// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Lib
#include <fstream>
#include <iostream>
#include <map>
#include <string>

// AWS SDK
#include <aws/core/auth/AWSCredentialsProvider.h>

// GameKit
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/exports.h>
#include <aws/gamekit/core/model/account_info.h>
#include <aws/gamekit/core/model/config_consts.h>
#include <aws/gamekit/core/errors.h>
#include <aws/gamekit/core/logging.h>

// yaml-cpp
#include <yaml-cpp/yaml.h>

namespace GameKit
{
    static const std::string GAMEKIT_SETTINGS_FILE = "saveInfo.yml";
    static const std::string GAMEKIT_SETTINGS_VERSION_KEY = "gamekitPluginVersion";
    static const std::string GAMEKIT_SETTINGS_GAME_KEY = "game";
    static const std::string GAMEKIT_SETTINGS_GAME_NAME = "name";
    static const std::string GAMEKIT_SETTINGS_SHORT_GAME_NAME = "short_name";
    static const std::string GAMEKIT_SETTINGS_LAST_USED_REGION = "lastUsedRegion";
    static const std::string GAMEKIT_SETTINGS_LAST_USED_ENVIRONMENT = "lastUsedEnvironment";
    static const std::string GAMEKIT_SETTINGS_LAST_USED_ENVIRONMENT_CODE = "code";
    static const std::string GAMEKIT_SETTINGS_ENVIRONMENTS_KEY = "customEnvironments";
    static const std::string GAMEKIT_SETTINGS_ENVIRONMENT_DESCRIPTION = "description";
    static const std::string GAMEKIT_SETTINGS_FEATURES_KEY = "features";
    static const std::string GAMEKIT_SETTINGS_FEATURE_ACTIVE = "active";
    static const std::string GAMEKIT_SETTINGS_FEATURE_VARS = "vars";

    /**
     * GameKitSettings offers read/write access to the "saveInfo.yml" GAMEKIT Settings file.
     *
     * The settings file contains information such as:
     * - Game name
     * - Custom deployment environments (ex: "Gamma")
     * - List of activated/deactivated features
     * - Feature-specific variables (ex: "isFacebookLoginEnabled" for Identity)
     *
     * GameKitSettings should not be called from a build at runtime. GameKitSettings can create files and alter critical AWS settings which should not be done from a built version of a game.
     *
     * The file is stored at "GAMEKIT_ROOT/shortGameName/saveInfo.yml".
     *
     * The file is read/written through usage of the plugin UI. For example, when a feature is activated,
     * when feature variables are filled in, or when a custom deployment environment is added or removed.
     *
     * The file is loaded during the constructor, and can be reloaded by calling Reload().
     * Call SaveSettings() to write the settings to back to disk after making modifications with any "Set", "Add/Delete", "Activate/Deactivate" methods.
     */
    class GAMEKIT_API GameKitSettings
    {
    private:
        std::string m_gamekitPluginVersion;
        std::string m_gamekitRootPath;
        std::string m_shortGameName;
        std::string m_currentEnvironment;
        YAML::Node m_gamekitYamlSettings;
        FuncLogCallback m_logCb;

        static unsigned int readAwsCredentials(const std::string& profileName, Aws::Config::AWSConfigFileProfileConfigLoader& configLoader, const std::string& credentialsFileLocation, Aws::Auth::AWSCredentials& credentials, Aws::Config::AWSProfileConfigLoader::ProfilesContainer& profiles, FuncLogCallback logCb);
        static unsigned int persistAwsProfiles(Aws::Config::AWSConfigFileProfileConfigLoader& configLoader, const std::string& credentialsFileLocation, const Aws::Config::AWSProfileConfigLoader::ProfilesContainer& profiles, FuncLogCallback logCb);
    public:
        GameKitSettings(const std::string& gamekitRoot, const std::string& pluginVersion, const std::string& shortGameName, const std::string& currentEnvironment, FuncLogCallback logCallback);
        ~GameKitSettings();
        void SetGameName(const std::string& gameName);
        void SetLastUsedRegion(const std::string& region);
        void SetLastUsedEnvironment(const std::string& gameName);
        void AddCustomEnvironment(const std::string& envCode, const std::string& envDescription);
        void DeleteCustomEnvironment(const std::string& envCode);
        void ActivateFeature(FeatureType featureType);
        void DeactivateFeature(FeatureType featureType);
        void SetFeatureVariables(FeatureType featureType, const std::map<std::string, std::string>& vars);
        void DeleteFeatureVariable(FeatureType featureType, std::string varName);
        unsigned int SaveSettings();
        unsigned int PopulateAndSave(const std::string& gameName, const std::string& envCode, const std::string& region);
        std::string GetGameName() const;
        std::string GetLastUsedRegion() const;
        std::string GetLastUsedEnvironment() const;
        std::map<std::string, std::string> GetCustomEnvironments() const;
        std::string GetCustomEnvironmentDescription(const std::string& envCode) const;
        bool IsFeatureActive(FeatureType featureType) const;
        std::map<std::string, std::string> GetFeatureVariables(FeatureType featureType) const;
        std::string GetFeatureVariable(FeatureType featureType, std::string varName) const;
        std::string GetSettingsFilePath() const;
        void Reload();

        static unsigned int SaveAwsCredentials(const std::string& profileName, const std::string& accessKey, const std::string& secretKey, FuncLogCallback logCb);
        static bool AwsProfileExists(const std::string& profileName);
        static unsigned int SetAwsAccessKey(const std::string& profileName, const std::string& newAccessKey, FuncLogCallback logCb);
        static unsigned int SetAwsSecretKey(const std::string& profileName, const std::string& newSecretKey, FuncLogCallback logCb);
        static unsigned int GetAwsProfile(const std::string& profileName, DISPATCH_RECEIVER_HANDLE receiver, FuncAwsProfileResponseCallback responseCallback, FuncLogCallback logCb);
    };
}
