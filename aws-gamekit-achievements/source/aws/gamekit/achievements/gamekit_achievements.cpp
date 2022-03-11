// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// AWS SDK
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/utils/StringUtils.h>

// GameKit
#include <aws/gamekit/achievements/gamekit_achievements.h>
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>
#include <aws/gamekit/core/utils/file_utils.h>

// Boost
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem.hpp>

using namespace Aws::Utils;
using namespace GameKit::Achievements;

#pragma region Constructors/Destructor
Achievements::Achievements(FuncLogCallback logCb, Authentication::GameKitSessionManager* sessionManager)
{
    m_logCb = logCb;
    m_sessionManager = sessionManager;

    GameKit::AwsApiInitializer::Initialize(m_logCb, this);

    static const long TIMEOUT = 5000;

    Aws::Client::ClientConfiguration clientConfig;
    GameKit::DefaultClients::SetDefaultClientConfiguration(m_sessionManager->GetClientSettings(), clientConfig);
    clientConfig.region = m_sessionManager->GetClientSettings()[GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_REGION].c_str();
    clientConfig.connectTimeoutMs = TIMEOUT;
    clientConfig.httpRequestTimeoutMs = TIMEOUT;
    clientConfig.requestTimeoutMs = TIMEOUT;
    m_httpClient = Aws::Http::CreateHttpClient(clientConfig);

    Logging::Log(m_logCb, Level::Info, "Achievements instantiated");
}

Achievements::~Achievements()
{
    GameKit::AwsApiInitializer::Shutdown(m_logCb, this);
}
#pragma endregion

#pragma region Public Methods
unsigned int Achievements::UpdateAchievementForPlayer(const char* achievementId, unsigned int incrementBy, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback)
{
    const std::string uri = m_sessionManager->GetClientSettings()[GameKit::ClientSettings::Achievements::SETTINGS_ACHIEVEMENTS_API_GATEWAY_BASE_URL] + "/" + achievementId + "/unlock";
    const std::string idToken = m_sessionManager->GetToken(GameKit::TokenType::IdToken);
    if (idToken.empty())
    {
        Logging::Log(m_logCb, Level::Info, "Achievements::UpdateAchievementForPlayer() No ID token in session.");
        return GAMEKIT_ERROR_NO_ID_TOKEN;
    }

    const std::shared_ptr<Aws::Http::HttpRequest> request = Aws::Http::CreateHttpRequest(Aws::String(uri), Aws::Http::HttpMethod::HTTP_POST, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
    request->SetAuthorization(Aws::String(idToken));

    Aws::Utils::Json::JsonValue body;
    body.WithInteger("increment_by", incrementBy);
    const Aws::String body_string = body.View().WriteCompact();

    std::shared_ptr<Aws::IOStream> bodyStream = Aws::MakeShared<Aws::StringStream>("UpdateAchievementBody", std::ios_base::in | std::ios_base::out);
    bodyStream->write(body_string.c_str(), body_string.length());

    request->SetContentType("application/json");
    request->AddContentBody(bodyStream);
    request->SetContentLength(StringUtils::to_string(body_string.length()));

    const std::shared_ptr<Aws::Http::HttpResponse> response = m_httpClient->MakeRequest(request);
    Aws::Utils::Json::JsonValue outJson;
    return processResponse(response, "Achievements::UpdateAchievementForPlayer()", dispatchReceiver, responseCallback, outJson);
}

unsigned int Achievements::GetAchievementForPlayer(const char* achievementId, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback)
{
    const std::string uri = m_sessionManager->GetClientSettings()[GameKit::ClientSettings::Achievements::SETTINGS_ACHIEVEMENTS_API_GATEWAY_BASE_URL] + "/" + achievementId;
    const std::string idToken = m_sessionManager->GetToken(GameKit::TokenType::IdToken);
    if (idToken.empty())
    {
        Logging::Log(m_logCb, Level::Info, "Achievements::GetAchievementForPlayer() No ID token in session.");
        return GAMEKIT_ERROR_NO_ID_TOKEN;
    }

    // Avoid hitting the ListAchievementsForPlayer() endpoint when empty achievement id is given.
    if (std::string(achievementId).empty())
    {
        Logging::Log(m_logCb, Level::Error, "Achievements::GetAchievementForPlayer() Achievement ID was empty, cannot retrieve.");
        return GAMEKIT_ERROR_ACHIEVEMENTS_INVALID_ID;
    }

    const std::shared_ptr<Aws::Http::HttpRequest> request = Aws::Http::CreateHttpRequest(Aws::String(uri), Aws::Http::HttpMethod::HTTP_GET, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
    request->SetAuthorization(ToAwsString(idToken));

    // TODO set use_consistent_read as queryStringParam after it's added as a parameter for this.

    const std::shared_ptr<Aws::Http::HttpResponse> response = m_httpClient->MakeRequest(request);
    Aws::Utils::Json::JsonValue outJson;
    return processResponse(response, "Achievements::GetAchievementForPlayer()", dispatchReceiver, responseCallback, outJson);
}

unsigned int Achievements::ListAchievementsForPlayer(unsigned int pageSize, bool waitForAllPages, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback)
{
    const std::string uri = m_sessionManager->GetClientSettings()[GameKit::ClientSettings::Achievements::SETTINGS_ACHIEVEMENTS_API_GATEWAY_BASE_URL];
    const std::string idToken = m_sessionManager->GetToken(GameKit::TokenType::IdToken);
    if (idToken.empty())
    {
        Logging::Log(m_logCb, Level::Info, "Achievements::ListAchievementsForPlayer() No ID token in session.");
        return GAMEKIT_ERROR_NO_ID_TOKEN;
    }

    Aws::String startKey = "";
    Aws::String pagingToken = "";
    unsigned int status = GameKit::GAMEKIT_SUCCESS;

    do
    {
        const std::shared_ptr<Aws::Http::HttpRequest> request = Aws::Http::CreateHttpRequest(Aws::String(uri), Aws::Http::HttpMethod::HTTP_GET, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
        request->SetAuthorization(ToAwsString(idToken));

        if (startKey != "")
        {
            request->AddQueryStringParameter("start_key", startKey);
            startKey = "";
            request->AddQueryStringParameter("paging_token", pagingToken);
            pagingToken = "";
        }
        request->AddQueryStringParameter("limit", StringUtils::to_string(pageSize));
        request->AddQueryStringParameter("wait_for_all_pages", StringUtils::to_string(waitForAllPages));

        const std::shared_ptr<Aws::Http::HttpResponse> response = m_httpClient->MakeRequest(request);
        Aws::Utils::Json::JsonValue value;
        status = processResponse(response, "Achievements::ListAchievementsForPlayer()", dispatchReceiver, responseCallback, value);
        if (status != GameKit::GAMEKIT_SUCCESS)
        {
            return status;
        }

        const Aws::Utils::Json::JsonView view = value.View();
        if (view.KeyExists("paging"))
        {
            const Aws::Utils::Json::JsonView pagingObj = view.GetObject("paging");
            if (pagingObj.KeyExists("next_start_key"))
            {
                startKey = pagingObj.GetObject("next_start_key").WriteCompact();
                if (!pagingObj.KeyExists("paging_token"))
                {
                    Logging::Log(m_logCb, Level::Error, "paging_token missing from response with next_start_key");
                    pagingToken = "";
                }
                else
                {
                    pagingToken = pagingObj.GetString("paging_token");
                }
            }
        }
    } while (startKey != "");

    return status;
}
#pragma endregion

#pragma region Private Methods
unsigned int Achievements::processResponse(const std::shared_ptr<Aws::Http::HttpResponse>& response, const std::string& originMethod,
    const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback, Aws::Utils::Json::JsonValue& jsonBody) const
{
    if (response->GetResponseCode() == Aws::Http::HttpResponseCode::NO_CONTENT)
    {
        return GAMEKIT_SUCCESS;
    }
    if (response->GetResponseCode() != Aws::Http::HttpResponseCode::OK)
    {
        const std::string errorMessage = "Error: " + originMethod + " returned with http response code : " + std::to_string(static_cast<int>(response->GetResponseCode()));
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_HTTP_REQUEST_FAILED;
    }

    Aws::IOStream& body = response->GetResponseBody();
    jsonBody = Aws::Utils::Json::JsonValue(body);

    if (!jsonBody.WasParseSuccessful())
    {
        const std::string errorMessage = "Error: " + originMethod + " response formatted incorrectly : " + ToStdString(jsonBody.GetErrorMessage());
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_PARSE_JSON_FAILED;
    }

    if (!(dispatchReceiver == nullptr) && !(responseCallback == nullptr))
    {
        const Aws::String output = jsonBody.View().WriteCompact();
        responseCallback(dispatchReceiver, output.c_str());
    }

    return GAMEKIT_SUCCESS;
}
#pragma endregion
