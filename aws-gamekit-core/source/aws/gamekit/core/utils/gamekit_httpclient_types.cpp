// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0


// AWS SDK
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/utils/StringUtils.h>

// GameKit
#include <aws/gamekit/core/awsclients/api_initializer.h>
#include <aws/gamekit/core/awsclients/default_clients.h>
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/core/utils/gamekit_httpclient.h>
#include <aws/gamekit/core/utils/gamekit_httpclient_types.h>

// Boost
#include <boost/crc.hpp>

#define MAX_STR_READ_LEN 1024

using namespace GameKit::Utils::HttpClient;
using namespace GameKit::Utils::Serialization;

#pragma region Serialization Public Methods
#if __ANDROID__
template <>
std::ostream& GameKit::Utils::Serialization::BinWrite<Aws::String>(std::ostream& os, const Aws::String& s)
{
    size_t length = s.length();
    os.write((char*)&length, sizeof(size_t));
    os.write(s.c_str(), s.length());

    return os;
}
#endif

template <>
std::ostream& GameKit::Utils::Serialization::BinWrite<std::string>(std::ostream& os, const std::string& s)
{
    size_t length = s.length();
    os.write((char*)&length, sizeof(size_t));
    os.write(s.c_str(), s.length());

    return os;
}

template <>
std::ostream& GameKit::Utils::Serialization::BinWrite<char*>(std::ostream& os, char* const& s)
{
    size_t length = strnlen(s, MAX_STR_READ_LEN);
    if (length == MAX_STR_READ_LEN)
    {
        // input string is longer than expected, set failbit
        os.setstate(std::ios::failbit);

        return os;
    }

    os.write((char*)&length, sizeof(size_t));
    os.write(s, length);

    return os;
}

#if __ANDROID__
template <>
std::istream& GameKit::Utils::Serialization::BinRead<Aws::String>(std::istream& is, Aws::String& s)
{
    size_t length = 0;
    is.read((char*)&length, sizeof(size_t));
    s.resize(length);
    is.read((char*)s.c_str(), length);

    return is;
}
#endif

template <>
std::istream& GameKit::Utils::Serialization::BinRead<std::string>(std::istream& is, std::string& s)
{
    size_t length = 0;
    is.read((char*)&length, sizeof(size_t));
    s.resize(length);
    is.read((char*)s.c_str(), length);

    return is;
}

unsigned int GameKit::Utils::Serialization::GetCRC(const std::string& s)
{
    boost::crc_32_type crc;
    crc.process_bytes(s.data(), s.size());

    return crc.checksum();
}

unsigned int GameKit::Utils::Serialization::GetCRC(const char* s, size_t length)
{
    boost::crc_32_type crc;
    crc.process_bytes(s, length);

    return crc.checksum();
}
#pragma endregion

#pragma region HttpClient Public Methods

GameKit::Utils::HttpClient::IOperation::IOperation(unsigned int maxAttempts,
    bool discard,
    std::shared_ptr<Aws::Http::HttpRequest> request,
    const Aws::Http::HttpResponseCode expectedCode,
    std::chrono::milliseconds timestamp)
    : Attempts(0),
    MaxAttempts(maxAttempts),
    Discard(discard),
    Request(request),
    ExpectedSuccessCode(expectedCode),
    CallbackContext(nullptr),
    SuccessCallback(nullptr),
    FailureCallback(nullptr),
    Timestamp(timestamp)
{}

bool GameKit::Utils::HttpClient::TrySerializeRequestBinary(std::ostream& os, const std::shared_ptr<Aws::Http::HttpRequest> request, FuncLogCallback logCb)
{
    try
    {
        BinWrite(os, request->GetURIString(false));
        BinWrite(os, request->GetMethod());

        auto queryStringParams = request->GetQueryStringParameters();
        BinWrite(os, queryStringParams.size());
        for (const auto& param : queryStringParams)
        {
            BinWrite(os, param.first);
            BinWrite(os, param.second);
        }

        auto headers = request->GetHeaders();
        BinWrite(os, headers.size());
        for (const auto& header : headers)
        {
            BinWrite(os, header.first);

            if (Aws::Utils::StringUtils::CaselessCompare(header.first.c_str(), "authorization"))
            {
                // Overwrite authorization value
                BinWrite(os, "~");
            }
            else
            {
                BinWrite(os, header.second);
            }
        }

        bool hasContent = request->HasContentLength();
        BinWrite(os, hasContent);
        if (hasContent)
        {
            BinWrite(os, request->GetContentType());
            BinWrite(os, request->GetContentLength());
            std::stringstream bodyStream = std::stringstream();
            
#if __ANDROID__
            // Rewind content body buffer before serializing
            request->GetContentBody()->clear();
            request->GetContentBody()->seekg(0);
#endif

            bodyStream << request->GetContentBody()->rdbuf();
            std::string body = bodyStream.str();
            BinWrite(os, body);
            BinWrite(os, GetCRC(body.c_str(), body.length()));

            // Rewind content body buffer
            request->GetContentBody()->clear();
            request->GetContentBody()->seekg(0);
        }

        return true;
    }
    catch (const std::ios_base::failure& failure)
    {
        std::string message = "Could not serialize HttpRequest, " + std::string(failure.what());
        Logging::Log(logCb, Level::Error, message.c_str());

        throw;
    }

    return false;
}

bool GameKit::Utils::HttpClient::TryDeserializeRequestBinary(std::istream& is, std::shared_ptr<Aws::Http::HttpRequest>& outRequest, FuncLogCallback logCb)
{
    std::string uri;
    Aws::Http::HttpMethod method;
    size_t queryStringParamCount;
    size_t headerCount;

    bool hasContent;
    std::string contentType;
    std::string contentLengthStr;
    std::string contentBody;

    long contentLength;
    unsigned int bodyCrc;

    try
    {
        BinRead(is, uri);
        BinRead(is, method);

        outRequest = Aws::Http::CreateHttpRequest(Aws::String(uri), method, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

        BinRead(is, queryStringParamCount);

        std::string key;
        std::string value;
        for (size_t i = 0; i < queryStringParamCount; ++i)
        {
            BinRead(is, key);
            BinRead(is, value);

            outRequest->AddQueryStringParameter(key.c_str(), ToAwsString(value));
        }

        BinRead(is, headerCount);
        for (size_t i = 0; i < headerCount; ++i)
        {
            BinRead(is, key);
            BinRead(is, value);

            outRequest->SetHeaderValue(key.c_str(), ToAwsString(value));
        }

        BinRead(is, hasContent);
        if (hasContent)
        {
            BinRead(is, contentType);
            BinRead(is, contentLengthStr);
            BinRead(is, contentBody);

            // verify content length matches
            contentLength = std::stol(contentLengthStr);
            if (contentBody.size() != contentLength)
            {
                Logging::Log(logCb, Level::Error, "Could not deserialize HttpRequest, content length mismatch");
                return false;
            }

            // verify body crc matches
            BinRead(is, bodyCrc);
            unsigned int computedCrc = GetCRC(contentBody);
            if (computedCrc != bodyCrc)
            {
                Logging::Log(logCb, Level::Error, "Could not deserialize HttpRequest, content CRC mismatch");
                return false;
            }

            // if body is Json, verify it can be parsed
            if (Aws::Utils::StringUtils::CaselessCompare(contentType.c_str(), "application/json"))
            {
                Aws::Utils::Json::JsonValue bodyObject(ToAwsString(contentBody));
                if (!bodyObject.WasParseSuccessful())
                {
                    std::string message = "Could not deserialize HttpRequest, content is not valid Json: " + std::string(bodyObject.GetErrorMessage());
                    Logging::Log(logCb, Level::Error, message.c_str());
                    return false;
                }
            }

            outRequest->SetContentType(ToAwsString(contentType));
            outRequest->SetContentLength(ToAwsString(contentLengthStr));
            std::shared_ptr<Aws::IOStream> bodyStream = Aws::MakeShared<Aws::StringStream>("RequestBody");
            *bodyStream << contentBody;
            outRequest->AddContentBody(bodyStream);
        }

        return true;
    }
    catch (const std::ios_base::failure& failure)
    {
        std::string message = "Could not deserialize HttpRequest, " + std::string(failure.what());
        Logging::Log(logCb, Level::Error, message.c_str());

        throw;
    }

    return false;
}

std::string GameKit::Utils::HttpClient::RequestResultTypeToString(RequestResultType resultType)
{
    switch (resultType)
    {
    case RequestResultType::RequestMadeSuccess:
        return "RequestMadeSuccess";
    case RequestResultType::RequestMadeFailure:
        return "RequestMadeFailure";
    case RequestResultType::RequestDropped:
        return "RequestDropped";
    case RequestResultType::RequestEnqueued:
        return "RequestEnqueued";
    case RequestResultType::RequestAttemptedAndEnqueued:
        return "RequestAttemptedAndEnqueued";

    default:
        return "Unknown result type";
        break;
    }
}
#pragma endregion

#pragma region RequestResult Public Methods
std::string RequestResult::ToString() const
{
    std::stringstream ss;
    ss << "RequestResult: " << RequestResultTypeToString(ResultType) << ", Response code: " <<
        (Response != nullptr ? std::to_string((int)Response->GetResponseCode()) : "null");

    return ss.str();
}

unsigned int RequestResult::ToErrorCode() const
{
    switch (ResultType)
    {
    case RequestResultType::RequestMadeSuccess:
        return GameKit::GAMEKIT_SUCCESS;
    case RequestResultType::RequestMadeFailure:
        return GameKit::GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_FAILED;
    case RequestResultType::RequestDropped:
        return GameKit::GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_DROPPED;
    case RequestResultType::RequestEnqueued:
        return GameKit::GAMEKIT_WARNING_USER_GAMEPLAY_DATA_API_CALL_ENQUEUED;
    case RequestResultType::RequestAttemptedAndEnqueued:
        return GameKit::GAMEKIT_WARNING_USER_GAMEPLAY_DATA_API_CALL_ENQUEUED;

    default:
        return GameKit::GAMEKIT_ERROR_GENERAL;
        break;
    }
}
#pragma endregion

#pragma region ExponentialBackoffStrategy Public Methods
ExponentialBackoffStrategy::ExponentialBackoffStrategy(unsigned int maxAttempts, FuncLogCallback logCb) : counter(0), currentStep(0), retryThreshold(0), maxAttempts(maxAttempts), logCb(logCb)
{}

ExponentialBackoffStrategy::~ExponentialBackoffStrategy()
{}

void ExponentialBackoffStrategy::IncreaseCounter()
{
    currentStep++;
    retryThreshold = pow(2, currentStep);
    retryThreshold = (std::rand() % retryThreshold) + 1;

    std::string message = "ExponentialBackoffStrategy step " + std::to_string(currentStep) + ", retry threshold " + std::to_string(retryThreshold);
    Logging::Log(logCb, Level::Verbose, message.c_str());
}

bool ExponentialBackoffStrategy::ShouldRetry()
{
    counter++;

    std::string message = "ExponentialBackoffStrategy counter " + std::to_string(counter);
    Logging::Log(logCb, Level::Verbose, message.c_str());

    return counter >= retryThreshold || counter >= maxAttempts;
}

void ExponentialBackoffStrategy::Reset()
{
    counter = 0;
    currentStep = 0;
    retryThreshold = 0;
}
#pragma endregion
