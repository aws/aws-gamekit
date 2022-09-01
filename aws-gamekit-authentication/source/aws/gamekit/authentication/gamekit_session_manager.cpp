// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0


// GameKit
#include <aws/gamekit/authentication/gamekit_session_manager.h>
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/core/utils/file_utils.h>
#include <aws/gamekit/core/utils/timestamp_ticker.h>

// yaml-cpp
#include <yaml-cpp/yaml.h>

using namespace GameKit::Authentication;

namespace CognitoModel = Aws::CognitoIdentityProvider::Model;

#pragma region Constructors/Destructor
GameKitSessionManager::GameKitSessionManager(const std::string& clientConfigFile, FuncLogCallback logCallback)
    :m_logCb(logCallback)
{
    m_awsClientsInitializedInternally = false;
    m_tokenRefresher = nullptr;
    m_cognitoClient = nullptr;
    m_clientSettings = std::make_shared<std::map<std::string, std::string>>();

    AwsApiInitializer::Initialize(m_logCb, this);

    if (!clientConfigFile.empty())
    {
        loadConfigFile(clientConfigFile);
    }

    InitializeDefaultAwsClients();
}

GameKitSessionManager::~GameKitSessionManager()
{
    m_clientSettings = nullptr;
    if (m_awsClientsInitializedInternally && m_cognitoClient != nullptr)
    {
        delete(m_cognitoClient);
        m_cognitoClient = nullptr;
    }

    AwsApiInitializer::Shutdown(m_logCb, this);

    if (m_tokenRefresher != nullptr)
    {
        m_tokenRefresher->Stop();
        m_tokenRefresher = nullptr;
    }

    m_logCb = nullptr;
}
#pragma endregion

#pragma region Public Methods
void GameKitSessionManager::InitializeDefaultAwsClients()
{
    // if region setting is not loaded or Cognito client is already set, return
    if (m_clientSettings->operator[](GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_REGION).empty() || m_cognitoClient != nullptr)
    {
        return;
    }

    m_awsClientsInitializedInternally = true;
    m_cognitoClient = DefaultClients::GetDefaultCognitoIdentityProviderClient(DefaultClients::GetDefaultClientConfigurationWithRegion(
        GetClientSettings(),
        ClientSettings::Authentication::SETTINGS_IDENTITY_REGION));
}

void GameKitSessionManager::SetToken(TokenType tokenType, const std::string& value)
{
    const std::lock_guard<std::mutex> lock(m_sessionTokensMutex);
    m_sessionTokens[(size_t)tokenType] = value;
}

std::string GameKitSessionManager::GetToken(TokenType tokenType)
{
    const std::lock_guard<std::mutex> lock(m_sessionTokensMutex);
    return m_sessionTokens[(size_t)tokenType];
}

void GameKitSessionManager::DeleteToken(TokenType tokenType)
{
    const std::lock_guard<std::mutex> lock(m_sessionTokensMutex);
    m_sessionTokens[(size_t)tokenType] = "";
}

void GameKitSessionManager::SetSessionExpiration(int expirationInSeconds)
{
    const std::lock_guard<std::mutex> lock(m_sessionTokensMutex);
    if (!m_sessionTokens[(size_t)TokenType::RefreshToken].empty())
    {
        // Execute refresh N minutes before token expires, or halfway to expiration if it is very soon
        int interval = std::max<int>(expirationInSeconds - DEFAULT_REFRESH_SECONDS_BEFORE_EXPIRATION,
            expirationInSeconds / 2);

        m_tokenRefresher = Aws::MakeShared<Utils::TimestampTicker>("tokenRefresher", interval, std::bind(&GameKitSessionManager::executeTokenRefresh, this), m_logCb);

        std::stringstream buffer;
        buffer << "GameKitSessionManager::SetSessionExpiration(): Next token refresh in " << interval << " seconds.";
        Logger::Logging::Log(m_logCb, Logger::Level::Info, buffer.str().c_str(), this);

        m_tokenRefresher->Start();
    }
}

bool GameKitSessionManager::AreSettingsLoaded(FeatureType featureType) const
{
    switch (featureType)
    {
    case FeatureType::Identity:
        return m_clientSettings->count(GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_REGION) &&
            m_clientSettings->count(GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_API_GATEWAY_BASE_URL) &&
            m_clientSettings->count(GameKit::ClientSettings::Authentication::SETTINGS_USER_POOL_CLIENT_ID);
    case FeatureType::UserGameplayData:
        return m_clientSettings->count(GameKit::ClientSettings::UserGameplayData::SETTINGS_USER_GAMEPLAY_DATA_API_GATEWAY_BASE_URL);
    case FeatureType::Achievements:
        return m_clientSettings->count(GameKit::ClientSettings::Achievements::SETTINGS_ACHIEVEMENTS_API_GATEWAY_BASE_URL);
    case FeatureType::GameStateCloudSaving:
        return m_clientSettings->count(GameKit::ClientSettings::GameSaving::SETTINGS_GAME_SAVING_BASE_URL);
    default:
        return false;
    }
}

std::map<std::string, std::string> GameKitSessionManager::GetClientSettings() const
{
    return *m_clientSettings;
}

void GameKitSessionManager::ReloadConfigFile(const std::string& clientConfigFile)
{
    Logger::Logging::Log(m_logCb, Logger::Level::Info, "GameKitSessionManager::ReloadConfigFile()");

    // reload config
    if (!clientConfigFile.empty())
    {
        loadConfigFile(clientConfigFile);

        // load Cognito client if it's not already set
        InitializeDefaultAwsClients();
    }
    // new game, env, or non existent path, unload previous settings
    else
    {
        m_clientSettings->clear();
    }
}

void GameKitSessionManager::ReloadConfigFromFileContents(const std::string& clientConfigFileContents)
{
    Logger::Logging::Log(m_logCb, Logger::Level::Info, "GameKitSessionManager::ReloadConfigFromFileContents()");
    if (clientConfigFileContents.size() == 0)
    {
        m_clientSettings->clear();
    }
    else
    {
        loadConfigContents(clientConfigFileContents);
        InitializeDefaultAwsClients();
    }
}
#pragma endregion

#pragma region Private Methods
void GameKitSessionManager::loadConfigFile(const std::string& clientConfigFile) const
{
    m_clientSettings->clear();
    YAML::Node paramsYml;
    Utils::FileUtils::ReadFileAsYAML(clientConfigFile, paramsYml, m_logCb, "GameKitSessionManager: ");
    for (YAML::const_iterator it = paramsYml.begin(); it != paramsYml.end(); ++it)
    {
        m_clientSettings->insert({ it->first.as<std::string>(), it->second.as<std::string>() });
    }
}

void GameKitSessionManager::loadConfigContents(const std::string& clientConfigFileContents) const
{
    m_clientSettings->clear();
    YAML::Node paramsYml;
    Utils::FileUtils::ReadFileContentsAsYAML(clientConfigFileContents, paramsYml, m_logCb, "GameKitSessionManager: ");
    for (YAML::const_iterator it = paramsYml.begin(); it != paramsYml.end(); ++it)
    {
        m_clientSettings->insert({ it->first.as<std::string>(), it->second.as<std::string>() });
    }
}

void GameKitSessionManager::executeTokenRefresh()
{
    Logger::Logging::Log(m_logCb, Logger::Level::Info, "GameKitSessionManager::executeTokenRefresh()");
    if (GetToken(TokenType::RefreshToken).empty())
    {
        Logger::Logging::Log(m_logCb, Logger::Level::Info, "SessionManager::executeTokenRefresh: No refresh token present, stopping token refresh loop.");
        m_tokenRefresher->AbortLoop();
        return;
    }

    auto request = CognitoModel::InitiateAuthRequest()
        .WithClientId(m_clientSettings->operator[](GameKit::ClientSettings::Authentication::SETTINGS_USER_POOL_CLIENT_ID).c_str())
        .WithAuthFlow(CognitoModel::AuthFlowType::REFRESH_TOKEN)
        .AddAuthParameters("REFRESH_TOKEN", ToAwsString(GetToken(TokenType::RefreshToken)));

    auto outcome = m_cognitoClient->InitiateAuth(request);

    unsigned int retryAttempt = 1;
    while(!outcome.IsSuccess() && retryAttempt <= MAX_REFRESH_RETRY_ATTEMPTS)
    {
        auto error = outcome.GetError();
        auto errorMessage = "Error: SessionManager::executeTokenRefresh: " + error.GetExceptionName() + ": " + error.GetMessage();
        Logger::Logging::Log(m_logCb, Logger::Level::Error, errorMessage.c_str());

        const std::string retryMessage = "SessionManager::executeTokenRefresh: Retry attempt " + std::to_string(retryAttempt) + "/" + std::to_string(MAX_REFRESH_RETRY_ATTEMPTS);
        Logger::Logging::Log(m_logCb, Logger::Level::Info, retryMessage.c_str());

        // This must be called within a thread from m_tokenRefresher so it doesn't block.
        std::chrono::seconds sleepDuration{retryAttempt*retryAttempt};
        std::this_thread::sleep_for(sleepDuration);

        outcome = m_cognitoClient->InitiateAuth(request);
        retryAttempt += 1;
    }
    if (!outcome.IsSuccess())
    {
        Logger::Logging::Log(m_logCb, Logger::Level::Error, "Error: SessionManager::executeTokenRefresh: Failed, will no longer retry.");
        m_tokenRefresher->AbortLoop();
        return;
    }

    Aws::String accessToken = outcome.GetResult().GetAuthenticationResult().GetAccessToken();
    Aws::String idToken = outcome.GetResult().GetAuthenticationResult().GetIdToken();
    int expiresIn = outcome.GetResult().GetAuthenticationResult().GetExpiresIn();

    SetToken(TokenType::AccessToken, ToStdString(accessToken));
    SetToken(TokenType::IdToken, ToStdString(idToken));

    // Execute refresh N minutes before token expires, or halfway to expiration if it is very soon
    int interval = std::max<int>(expiresIn - DEFAULT_REFRESH_SECONDS_BEFORE_EXPIRATION, expiresIn / 2);
    std::stringstream buffer;
    buffer << "SessionManager::executeTokenRefresh: Next token refresh in " << interval << " seconds.";
    Logger::Logging::Log(m_logCb, Logger::Level::Info, buffer.str().c_str(), this);

    m_tokenRefresher->RescheduleLoop(interval);
}
#pragma endregion
