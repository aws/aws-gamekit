// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// AWS SDK
#include <aws/core/utils/StringUtils.h>
#include <aws/core/http/HttpClientFactory.h>

// GameKit
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/identity/facebook_identity_provider.h>

using namespace Aws::Utils;
using namespace GameKit;
using namespace GameKit::Logger;

GameKit::Identity::FacebookIdentityProvider::FacebookIdentityProvider(std::map<std::string, std::string>& clientSettings, FuncLogCallback logCb)
    :m_clientSettings(clientSettings), m_logCb(logCb)
{
    GameKit::AwsApiInitializer::Initialize(m_logCb, this);
    static const long TIMEOUT = 5000;
    Aws::Client::ClientConfiguration clientConfig;

    GameKit::DefaultClients::SetDefaultClientConfiguration(clientSettings, clientConfig);
    // Extend timeouts to account for cold lambda starts
    clientConfig.connectTimeoutMs = TIMEOUT;
    clientConfig.httpRequestTimeoutMs = TIMEOUT;
    clientConfig.requestTimeoutMs = TIMEOUT;
    m_httpClient = Aws::Http::CreateHttpClient(clientConfig);
}

GameKit::Identity::FacebookIdentityProvider::FacebookIdentityProvider(std::map<std::string, std::string>& clientSettings, const std::shared_ptr<Aws::Http::HttpClient> httpClient, FuncLogCallback logCb)
    :m_clientSettings(clientSettings), m_logCb(logCb)
{
    GameKit::AwsApiInitializer::Initialize(m_logCb, this);
    m_httpClient = httpClient;
}

GameKit::Identity::FacebookIdentityProvider::~FacebookIdentityProvider()
{
    GameKit::AwsApiInitializer::Shutdown(m_logCb, this);
}

GameKit::Identity::LoginUrlResponseInternal GameKit::Identity::FacebookIdentityProvider::GetLoginUrl()
{
    std::string requestId = Aws::String(Aws::Utils::UUID::RandomUUID()).c_str();
    boost::algorithm::to_lower(requestId);
    std::string payload = "{\"request_id\": \"" + requestId + "\"}";
    std::shared_ptr <Aws::Http::HttpResponse> resp = this->makeRequest("/fbloginurl", Aws::Http::HttpMethod::HTTP_POST, payload);
    Aws::Http::HttpResponseCode respCode = resp->GetResponseCode();
    unsigned int gamekitStatus = respCode == Aws::Http::HttpResponseCode::OK ? GameKit::GAMEKIT_SUCCESS : GameKit::GAMEKIT_ERROR_HTTP_REQUEST_FAILED;
    if (gamekitStatus != GameKit::GAMEKIT_SUCCESS)
    {
        std::string msg = "FacebookIdentityProvider::GetLoginUrl() unsuccessful http request, returned with code: " + std::to_string(static_cast<int>(respCode));
        Logging::Log(m_logCb, Level::Error, msg.c_str());
    }

    Aws::StringStream respBody;
    respBody << resp->GetResponseBody().rdbuf();

    return LoginUrlResponseInternal{ gamekitStatus, requestId, ToStdString(respBody.str()) };
}

unsigned int GameKit::Identity::FacebookIdentityProvider::PollForCompletion(const std::string& requestId, int timeout, std::string& encryptedLocation)
{
    std::time_t startTime = std::time(nullptr);
    std::shared_ptr <Aws::Http::HttpResponse> resp;
    Aws::Http::HttpResponseCode respCode;

    while(true)
    {
        std::string payload = "{\"request_id\": \"" + requestId + "\"}";
        resp = this->makeRequest("/fblogincheck", Aws::Http::HttpMethod::HTTP_POST, payload);
        respCode = resp->GetResponseCode();

        if (respCode != Aws::Http::HttpResponseCode::NOT_FOUND)
        {
            break;
        }

        // sleep for 5 seconds before checking again
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // current time is past timeout, break out of the loop
        std::time_t currentTime = std::time(nullptr);
        if (currentTime - startTime > timeout)
        {
            Logging::Log(m_logCb, Level::Error, "FacebookIdentityProvider::PollForCompletion() timed out waiting for Facebook login completion.");
            return GameKit::GAMEKIT_ERROR_REQUEST_TIMED_OUT;
        }
    }

    if (respCode != Aws::Http::HttpResponseCode::OK)
    {
        const std::string errorMessage = "FacebookIdentityProvider::PollForCompletion() returned with http response code : " +
                std::to_string(static_cast<int>(respCode)) + ", message: Http request to get encrypted location failed";
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        encryptedLocation = "";
        return  GameKit::GAMEKIT_ERROR_HTTP_REQUEST_FAILED;
    }

    Aws::StringStream respBody;
    respBody << resp->GetResponseBody().rdbuf();
    encryptedLocation = respBody.str();

    if (encryptedLocation == "Retrieved")
    {
        Logging::Log(m_logCb, Level::Warning, "FacebookIdentityProvider::PollForCompletion() encrypted location already retrieved.");
        encryptedLocation = "";
    }
    return GameKit::GAMEKIT_SUCCESS;
}

unsigned int GameKit::Identity::FacebookIdentityProvider::RetrieveTokens(const std::string& location, std::string& tokens)
{
    std::string payload = location;
    auto resp = this->makeRequest("/fbtokens", Aws::Http::HttpMethod::HTTP_POST, payload);
    if (resp->GetResponseCode() != Aws::Http::HttpResponseCode::OK)
    {
        const std::string errorMessage = "FacebookIdentityProvider::RetrieveTokens() returned with http response code : " +
                std::to_string(static_cast<int>(resp->GetResponseCode())) + ", message: Http request to get Facebook Token failed" ;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return  GameKit::GAMEKIT_ERROR_HTTP_REQUEST_FAILED;
    }
    Aws::StringStream respBody;
    respBody << resp->GetResponseBody().rdbuf();

    tokens = respBody.str();
    if (tokens == "Retrieved")
    {
        Logging::Log(m_logCb, Level::Warning, "FacebookIdentityProvider::RetrieveTokens() encrypted tokens already retrieved.");
        tokens = "";
    }

    return GameKit::GAMEKIT_SUCCESS;
}

std::shared_ptr <Aws::Http::HttpResponse> GameKit::Identity::FacebookIdentityProvider::makeRequest(const std::string& path, Aws::Http::HttpMethod method, const std::string& payload)
{
    std::string baseUrl = m_clientSettings[ClientSettings::Authentication::SETTINGS_IDENTITY_API_GATEWAY_BASE_URL];
    std::shared_ptr<Aws::IOStream> body = Aws::MakeShared<Aws::StringStream>("requestPayload");
    body->write(payload.c_str(), payload.size());

    Aws::String fullUrl = ToAwsString(baseUrl);
    fullUrl.append(path.c_str());
    std::shared_ptr<Aws::Http::HttpRequest> request = Aws::Http::CreateHttpRequest(fullUrl, method, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
    request->AddContentBody(body);
    request->SetContentLength(StringUtils::to_string(payload.size()));
    request->SetContentType("application/json");

    std::shared_ptr<Aws::Http::HttpResponse> response = m_httpClient->MakeRequest(request);
    return response;
}
