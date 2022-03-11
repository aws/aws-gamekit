// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// AWS SDK
#include <aws/core/http/HttpClientFactory.h>

// GameKit
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/game-saving/gamekit_game_saving_caller.h>

using namespace GameKit::Logger;

// max number of retries when a retryable error is returned by the http client
static const long RETRIES = 10;

// scaling factor between retries: delay = (1 << number of attempts) * SCALING_FACTOR
static const long SCALING_FACTOR = 25;

#pragma region Public Methods
void GameKit::GameSaving::Caller::Initialize(Authentication::GameKitSessionManager* sessionManager, FuncLogCallback logCb, std::shared_ptr<Aws::Http::HttpClient>* httpClientPointer)
{
    m_sessionManager = sessionManager;
    m_logCb = logCb;
    m_httpClient = httpClientPointer;
}

unsigned int GameKit::GameSaving::Caller::CallApiGateway(
    const std::string& uri,
    Aws::Http::HttpMethod method,
    const std::string& currentFunctionName,
    Aws::Utils::Json::JsonValue& returnedJsonValue,
    const CallerParams& queryStringParams,
    const CallerParams& headerParams) const
{
    const std::string idToken = m_sessionManager->GetToken(GameKit::TokenType::IdToken);
    if (idToken.empty())
    {
        const std::string message = "GameSaving::" + currentFunctionName + "() No ID token in session.";
        Logger::Logging::Log(m_logCb, Level::Info, message.c_str());
        return GAMEKIT_ERROR_NO_ID_TOKEN;
    }

    const std::shared_ptr<Aws::Http::HttpRequest> request = CreateHttpRequest(ToAwsString(uri), method, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
    request->SetAwsAuthorization(Aws::String(idToken));

    // add any query string params
    for (auto param : queryStringParams)
    {
        request->AddQueryStringParameter(param.first.c_str(), ToAwsString(param.second));
    }

    // add any header params
    for (auto param : headerParams)
    {
        request->SetHeaderValue(param.first.c_str(), ToAwsString(param.second));
    }

    // attempt to make the http request
    std::shared_ptr<Aws::Http::HttpResponse> response;
    for (int tries = 0; tries < RETRIES; ++tries)
    {
        response = (*m_httpClient)->MakeRequest(request);

        // if the request failed for any reason other than the request was not made (results from a cold lambda) then do not retry
        if (response->GetResponseCode() != Aws::Http::HttpResponseCode::REQUEST_NOT_MADE)
        {
            break;
        }

        // increase the delay between each retry
        int delay = (1 << tries) * SCALING_FACTOR;

        const std::string message = "GameSaving::" + currentFunctionName + "() - http request was not made, retrying call after " + std::to_string(delay) + " ms";
        Logger::Logging::Log(m_logCb, Level::Info, message.c_str());

        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
    if (response->GetResponseCode() == Aws::Http::HttpResponseCode::NO_CONTENT)
    {
        return GAMEKIT_SUCCESS;
    }

    // Error case for exceeding max cloud slots on save
    if (response->GetResponseCode() == Aws::Http::HttpResponseCode::BAD_REQUEST)
    {
        Aws::IOStream& body = response->GetResponseBody();
        returnedJsonValue = Aws::Utils::Json::JsonValue(body);
        if (returnedJsonValue.WasParseSuccessful())
        {
            const Aws::Utils::Json::JsonView view = returnedJsonValue.View();
            std::string responseStatusMessage = view.KeyExists(RESPONSE_BODY_KEY_META) ? ToStdString(view.GetObject(RESPONSE_BODY_KEY_META).GetString(RESPONSE_BODY_KEY_META_MESSAGE)) : "";
            const std::string errorMessage = "Error: GameSaving::" + currentFunctionName + "() returned with http response code : " + 
                std::to_string(static_cast<int>(response->GetResponseCode())) + ", message: " + responseStatusMessage;
            Logger::Logging::Log(m_logCb, Level::Error, errorMessage.c_str());

            if (GetResponseStatusFromString(responseStatusMessage) == ResponseStatus::MAX_CLOUD_SAVE_SLOTS_EXCEEDED)
            {
                return GAMEKIT_ERROR_GAME_SAVING_MAX_CLOUD_SLOTS_EXCEEDED;
            }

            return GAMEKIT_ERROR_HTTP_REQUEST_FAILED;
        }
    }

    if (response->GetResponseCode() != Aws::Http::HttpResponseCode::OK)
    {
        const std::string errorMessage = "Error: GameSaving::" + currentFunctionName + "() returned with http response code : " + std::to_string(static_cast<int>(response->GetResponseCode()));
        Logger::Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_HTTP_REQUEST_FAILED;
    }

    Aws::IOStream& body = response->GetResponseBody();
    returnedJsonValue = Aws::Utils::Json::JsonValue(body);

    if (!returnedJsonValue.WasParseSuccessful())
    {
        const std::string errorMessage = "Error: GameSaving::" + currentFunctionName + "() response formatted incorrectly : " + ToStdString(returnedJsonValue.GetErrorMessage());
        Logger::Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_PARSE_JSON_FAILED;
    }

    return GAMEKIT_SUCCESS;
}
#pragma endregion
