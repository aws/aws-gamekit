// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/core/gamekit_settings.h>
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>
#include <aws/gamekit/core/utils/file_utils.h>

using namespace GameKit;
using namespace GameKit::Logger;
using namespace Aws::Auth;
using namespace Aws::Config;

#pragma region Public Methods
GameKitSettings::GameKitSettings(const std::string& gamekitRoot, const std::string& pluginVersion, const std::string& shortGameName, const std::string& currentEnvironment, FuncLogCallback logCallback) :
    m_gamekitRootPath(gamekitRoot), m_gamekitPluginVersion(pluginVersion), m_shortGameName(shortGameName), m_currentEnvironment(currentEnvironment), m_logCb(logCallback)
{
    Logging::Log(m_logCb, Level::Info, "GameKitSettings instantiated");

    // For the settings file, "not found" is a warning and not an error, because it never exists on the first run.
    std::string gamekitSettingsFile = GetSettingsFilePath();
    if (boost::filesystem::exists(gamekitSettingsFile))
    {
        const unsigned int returnCode = Utils::FileUtils::ReadFileAsYAML(gamekitSettingsFile, m_gamekitYamlSettings, m_logCb, "Plugin settings: ");
        if (returnCode == GAMEKIT_SUCCESS)
        {
            std::string msg = std::string("Plugin settings file loaded from ").append(gamekitSettingsFile);
            Logging::Log(m_logCb, Level::Info, msg.c_str());
        }
        // else ReadFileAsYAML has already logged an error message
    }
    else
    {
        std::string msg = std::string("Plugin settings file not found at ").append(gamekitSettingsFile);
        Logging::Log(m_logCb, Level::Warning, msg.c_str());
    }
}

GameKitSettings::~GameKitSettings()
{ }

void GameKitSettings::SetGameName(const std::string& gameName)
{
    m_gamekitYamlSettings[GAMEKIT_SETTINGS_GAME_KEY][GAMEKIT_SETTINGS_GAME_NAME] = gameName;
}

void GameKitSettings::SetLastUsedRegion(const std::string& region)
{
    m_gamekitYamlSettings[GAMEKIT_SETTINGS_LAST_USED_REGION] = region;
}

void GameKitSettings::SetLastUsedEnvironment(const std::string& envCode)
{
    m_gamekitYamlSettings[GAMEKIT_SETTINGS_LAST_USED_ENVIRONMENT][GAMEKIT_SETTINGS_LAST_USED_ENVIRONMENT_CODE] = envCode;
}

void GameKitSettings::AddCustomEnvironment(const std::string& envCode, const std::string& envDescription)
{
    m_gamekitYamlSettings[GAMEKIT_SETTINGS_ENVIRONMENTS_KEY][envCode][GAMEKIT_SETTINGS_ENVIRONMENT_DESCRIPTION] = envDescription;
}

void GameKitSettings::DeleteCustomEnvironment(const std::string& envCode)
{
    m_gamekitYamlSettings[GAMEKIT_SETTINGS_ENVIRONMENTS_KEY].remove(envCode);
}

void GameKitSettings::ActivateFeature(FeatureType featureType)
{
    m_gamekitYamlSettings[this->m_currentEnvironment][GAMEKIT_SETTINGS_FEATURES_KEY][GameKit::GetFeatureTypeString(featureType)][GAMEKIT_SETTINGS_FEATURE_ACTIVE] = true;
}

void GameKitSettings::DeactivateFeature(FeatureType featureType)
{
    m_gamekitYamlSettings[this->m_currentEnvironment][GAMEKIT_SETTINGS_FEATURES_KEY][GameKit::GetFeatureTypeString(featureType)][GAMEKIT_SETTINGS_FEATURE_ACTIVE] = false;
}

void GameKitSettings::SetFeatureVariables(FeatureType featureType, const std::map<std::string, std::string>& vars)
{
    YAML::Node featureVars = m_gamekitYamlSettings[this->m_currentEnvironment][GAMEKIT_SETTINGS_FEATURES_KEY][GameKit::GetFeatureTypeString(featureType)][GAMEKIT_SETTINGS_FEATURE_VARS];
    for (auto const& entry : vars)
    {
        featureVars[entry.first] = entry.second;
    }
}

void GameKitSettings::DeleteFeatureVariable(FeatureType featureType, std::string varName)
{
    m_gamekitYamlSettings[this->m_currentEnvironment][GAMEKIT_SETTINGS_FEATURES_KEY][GameKit::GetFeatureTypeString(featureType)][GAMEKIT_SETTINGS_FEATURE_VARS].remove(varName);
}

unsigned int GameKitSettings::SaveSettings()
{
    m_gamekitYamlSettings[GAMEKIT_SETTINGS_GAME_KEY][GAMEKIT_SETTINGS_SHORT_GAME_NAME] = m_shortGameName;
    m_gamekitYamlSettings[GAMEKIT_SETTINGS_VERSION_KEY] = m_gamekitPluginVersion;

    const unsigned int resultCode = Utils::FileUtils::WriteYAMLToFile(m_gamekitYamlSettings, GetSettingsFilePath(), Configuration::DO_NOT_EDIT, m_logCb, "Plugin settings: ");
    if (resultCode != GAMEKIT_SUCCESS)
    {
        return GAMEKIT_ERROR_SETTINGS_FILE_SAVE_FAILED;
    }

    std::string msg = std::string("Plugin settings saved to ").append(this->GetSettingsFilePath());
    Logging::Log(m_logCb, Level::Info, msg.c_str());
    return GAMEKIT_SUCCESS;
}

std::string GameKitSettings::GetGameName() const
{
    return m_gamekitYamlSettings[GAMEKIT_SETTINGS_GAME_KEY][GAMEKIT_SETTINGS_GAME_NAME].Scalar();
}

std::string GameKitSettings::GetLastUsedRegion() const
{
    try
    {
        return m_gamekitYamlSettings[GAMEKIT_SETTINGS_LAST_USED_REGION].Scalar();
    }
    catch (const YAML::InvalidNode&)
    {
        return "us-east-1";
    }
}

std::string GameKitSettings::GetLastUsedEnvironment() const
{
    try
    {
        return m_gamekitYamlSettings[GAMEKIT_SETTINGS_LAST_USED_ENVIRONMENT][GAMEKIT_SETTINGS_LAST_USED_ENVIRONMENT_CODE].Scalar();
    }
    catch (const YAML::InvalidNode&)
    {
        return "dev";
    }
}

std::map<std::string, std::string> GameKitSettings::GetCustomEnvironments() const
{
    std::map<std::string, std::string> envs;
    for (auto const& entry : m_gamekitYamlSettings[GAMEKIT_SETTINGS_ENVIRONMENTS_KEY])
    {
        envs.insert({ entry.first.as<std::string>(), entry.second[GAMEKIT_SETTINGS_ENVIRONMENT_DESCRIPTION].Scalar() });
    }

    return envs;
}

std::string GameKitSettings::GetCustomEnvironmentDescription(const std::string& envCode) const
{
    if (!m_gamekitYamlSettings[GAMEKIT_SETTINGS_ENVIRONMENTS_KEY][envCode])
    {
        return std::string();
    }

    return m_gamekitYamlSettings[GAMEKIT_SETTINGS_ENVIRONMENTS_KEY][envCode][GAMEKIT_SETTINGS_ENVIRONMENT_DESCRIPTION].Scalar();
}

bool GameKitSettings::IsFeatureActive(FeatureType featureType) const
{
    return m_gamekitYamlSettings[this->m_currentEnvironment][GAMEKIT_SETTINGS_FEATURES_KEY][GameKit::GetFeatureTypeString(featureType)][GAMEKIT_SETTINGS_FEATURE_ACTIVE].as<bool>();
}

std::map<std::string, std::string> GameKitSettings::GetFeatureVariables(FeatureType featureType) const
{
    std::map<std::string, std::string> vars;
    if (!m_gamekitYamlSettings[this->m_currentEnvironment] || !m_gamekitYamlSettings[this->m_currentEnvironment][GAMEKIT_SETTINGS_FEATURES_KEY] || !m_gamekitYamlSettings[this->m_currentEnvironment][GAMEKIT_SETTINGS_FEATURES_KEY][GameKit::GetFeatureTypeString(featureType)])
    {
        return vars;
    }

    for (auto const& entry : m_gamekitYamlSettings[this->m_currentEnvironment][GAMEKIT_SETTINGS_FEATURES_KEY][GameKit::GetFeatureTypeString(featureType)][GAMEKIT_SETTINGS_FEATURE_VARS])
    {
        vars.insert({ entry.first.as<std::string>(), entry.second.as<std::string>() });
    }

    return vars;
}

std::string GameKitSettings::GetFeatureVariable(FeatureType featureType, std::string varName) const
{

    if (!m_gamekitYamlSettings[this->m_currentEnvironment])
    {
        return std::string();
    }

    if (!m_gamekitYamlSettings[this->m_currentEnvironment][GAMEKIT_SETTINGS_FEATURES_KEY])
    {
        return std::string();
    }

    if (!m_gamekitYamlSettings[this->m_currentEnvironment][GAMEKIT_SETTINGS_FEATURES_KEY][GameKit::GetFeatureTypeString(featureType)][GAMEKIT_SETTINGS_FEATURE_VARS][varName])
    {
        return std::string();
    }

    return m_gamekitYamlSettings[this->m_currentEnvironment][GAMEKIT_SETTINGS_FEATURES_KEY][GameKit::GetFeatureTypeString(featureType)][GAMEKIT_SETTINGS_FEATURE_VARS][varName].Scalar();
}

void GameKitSettings::Reload()
{
    YAML::Node reloaded;
    const unsigned int result = Utils::FileUtils::ReadFileAsYAML(GetSettingsFilePath(), reloaded, m_logCb, "Plugin settings: ");
    if (result == GAMEKIT_SUCCESS)
    {
        m_gamekitYamlSettings.reset(reloaded);
        std::string msg = std::string("Reloaded plugin settings from ").append(this->GetSettingsFilePath());
        Logging::Log(m_logCb, Level::Warning, msg.c_str());
    }
    // else ReadFileAsYAML has already logged the error
}

std::string GameKitSettings::GetSettingsFilePath() const
{
    return m_gamekitRootPath + "/" + m_shortGameName + "/" + GAMEKIT_SETTINGS_FILE;
}

unsigned int GameKitSettings::SaveAwsCredentials(const std::string& profileName, const std::string& accessKey, const std::string& secretKey, FuncLogCallback logCb) const 
{
    const std::string credentialsFileLocation = ToStdString(ProfileConfigFileAWSCredentialsProvider::GetCredentialsProfileFilename());

    AWSConfigFileProfileConfigLoader configLoader = AWSConfigFileProfileConfigLoader(ToAwsString(credentialsFileLocation));

    AWSProfileConfigLoader::ProfilesContainer profiles;

    std::ifstream credentialsFile = std::ifstream(credentialsFileLocation);

    if (credentialsFile && credentialsFile.peek() != std::ifstream::traits_type::eof())
    {
        // If the file exists but there is an issue loading the credentials file, log and return an error. Otherwise, load the profiles from the file and continue
        if (!configLoader.Load())
        {
            const std::string errorMessage = "Failed to load Aws credentials at " + credentialsFileLocation;
            Logging::Log(logCb, Level::Error, errorMessage.c_str());

            return GAMEKIT_ERROR_CREDENTIALS_FILE_MALFORMED;
        }

        profiles = configLoader.GetProfiles();
    }

    Aws::String profileNameAwsStr = ToAwsString(profileName);
    if (profiles.find(profileNameAwsStr) != profiles.end())
    {
        const std::string infoMessage = "Credential profile:" + profileName + " already exists, updating access and secret";
        Logging::Log(logCb, Level::Info, infoMessage.c_str());

        AWSCredentials credentials = profiles[profileNameAwsStr].GetCredentials();
        credentials.SetAWSAccessKeyId(ToAwsString(accessKey));
        credentials.SetAWSSecretKey(ToAwsString(secretKey));
        profiles[profileNameAwsStr].SetCredentials(credentials);
    }
    else
    {
        const AWSCredentials credentials = AWSCredentials(ToAwsString(accessKey), ToAwsString(secretKey));
        Profile newProfile;
        newProfile.SetName(profileNameAwsStr);
        newProfile.SetCredentials(credentials);
        profiles[profileNameAwsStr] = newProfile;
    }

    return persistAwsProfiles(configLoader, credentialsFileLocation, profiles, logCb);
}

unsigned int GameKitSettings::SetAwsAccessKey(const std::string& profileName, const std::string& newAccessKey, FuncLogCallback logCb) const
{
    const std::string credentialsFileLocation = ToStdString(ProfileConfigFileAWSCredentialsProvider::GetCredentialsProfileFilename());

    AWSConfigFileProfileConfigLoader configLoader = AWSConfigFileProfileConfigLoader(ToAwsString(credentialsFileLocation));

    AWSCredentials credentials;
    AWSProfileConfigLoader::ProfilesContainer profiles;

    const unsigned int result = readAwsCredentials(profileName, configLoader, credentialsFileLocation, credentials, profiles, logCb);

    if (result != GAMEKIT_SUCCESS)
    {
        return result;
    }

    credentials.SetAWSAccessKeyId(ToAwsString(newAccessKey));
    profiles[ToAwsString(profileName)].SetCredentials(credentials);

    return persistAwsProfiles(configLoader, credentialsFileLocation, profiles, logCb);
}

unsigned int GameKitSettings::SetAwsSecretKey(const std::string& profileName, const std::string& newSecretKey, FuncLogCallback logCb) const
{
    const std::string credentialsFileLocation = ToStdString(ProfileConfigFileAWSCredentialsProvider::GetCredentialsProfileFilename());

    AWSConfigFileProfileConfigLoader configLoader = AWSConfigFileProfileConfigLoader(ToAwsString(credentialsFileLocation));

    AWSCredentials credentials;
    AWSProfileConfigLoader::ProfilesContainer profiles;

    const unsigned int result = readAwsCredentials(profileName, configLoader, credentialsFileLocation, credentials, profiles, logCb);

    if (result != GAMEKIT_SUCCESS)
    {
        return result;
    }

    credentials.SetAWSSecretKey(ToAwsString(newSecretKey));
    profiles[ToAwsString(profileName)].SetCredentials(credentials);

    return persistAwsProfiles(configLoader, credentialsFileLocation, profiles, logCb);
}

unsigned int GameKitSettings::GetAwsProfile(const std::string& profileName, DISPATCH_RECEIVER_HANDLE receiver, FuncAwsProfileResponseCallback responseCallback, FuncLogCallback logCb) const
{
    const std::string credentialsFileLocation = ToStdString(ProfileConfigFileAWSCredentialsProvider::GetCredentialsProfileFilename());

    AWSConfigFileProfileConfigLoader configLoader = AWSConfigFileProfileConfigLoader(ToAwsString(credentialsFileLocation));

    AWSCredentials credentials;
    AWSProfileConfigLoader::ProfilesContainer profiles;

    const unsigned int result = readAwsCredentials(profileName, configLoader, credentialsFileLocation, credentials, profiles, logCb);

    if (result != GAMEKIT_SUCCESS)
    {
        return result;
    }

    responseCallback(receiver, credentials.GetAWSAccessKeyId().c_str(), credentials.GetAWSSecretKey().c_str());

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitSettings::readAwsCredentials(const std::string& profileName, AWSConfigFileProfileConfigLoader& configLoader, const std::string& credentialsFileLocation, AWSCredentials& credentials, AWSProfileConfigLoader::ProfilesContainer& profiles, FuncLogCallback logCb) const
{
    std::ifstream credentialsFile = std::ifstream(credentialsFileLocation);

    if (!credentialsFile || credentialsFile.peek() == std::ifstream::traits_type::eof())
    {
        return GAMEKIT_ERROR_CREDENTIALS_FILE_NOT_FOUND;
    }

    if (!configLoader.Load())
    {
        const std::string errorMessage = "Failed to load Aws credentials at " + credentialsFileLocation;
        Logging::Log(logCb, Level::Error, errorMessage.c_str());

        return GAMEKIT_ERROR_CREDENTIALS_FILE_MALFORMED;
    }

    profiles = configLoader.GetProfiles();

    if (profiles.find(ToAwsString(profileName)) == profiles.end())
    {
        const std::string errorMessage = "Credential profile" + profileName + " does not exist";
        Logging::Log(logCb, Level::Error, errorMessage.c_str());

        return GAMEKIT_ERROR_CREDENTIALS_NOT_FOUND;
    }

    credentials = profiles[ToAwsString(profileName)].GetCredentials();

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitSettings::persistAwsProfiles(AWSConfigFileProfileConfigLoader& configLoader, const std::string& credentialsFileLocation, const AWSProfileConfigLoader::ProfilesContainer& profiles, FuncLogCallback logCb) const
{
    if (!configLoader.PersistProfiles(profiles))
    {
        const std::string errorMessage = "Failed to save Aws credentials to " + credentialsFileLocation;
        Logging::Log(logCb, Level::Error, errorMessage.c_str());

        return GAMEKIT_ERROR_CREDENTIALS_FILE_SAVE_FAILED;
    }

    return GAMEKIT_SUCCESS;
}
#pragma endregion
