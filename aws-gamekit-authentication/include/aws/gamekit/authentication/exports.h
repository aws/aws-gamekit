// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

/** @file
 * @brief The C interface for the Authentication library.
 */

#pragma once
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/enums.h>
#include <aws/gamekit/core/logging.h>

 /**
  * @brief GameKitSessionManager instance handle created by calling #GameKitSessionManagerInstanceCreate()
 */
typedef void* GAMEKIT_SESSIONMANAGER_INSTANCE_HANDLE;

extern "C"
{
    /**
     * @brief Create a GameKitSessionManager instance, which can be used to access the SessionManager API.
     *
     * @details Make sure to call GameKitSessionManagerInstanceRelease() to destroy the returned object when finished with it.
     *
     * @param clientConfigFile (Optional, can be a nullptr or empty string) Relative filepath to the generated file "awsGameKitClientConfig.yml".
     * The config file is generated by GAMEKIT each time a feature is deployed or re-deployed, and has settings for each GAMEKIT feature you've deployed.
     * @param logCb Callback function for logging information and errors.
     * @return Pointer to the new GameKitSessionManager instance.
     */
    GAMEKIT_API GAMEKIT_SESSIONMANAGER_INSTANCE_HANDLE GameKitSessionManagerInstanceCreate(const char* clientConfigFile, FuncLogCallback logCb);

    /**
     * @brief Check if the settings are loaded for the feature.
     *
     * @detailed These settings are found in file "awsGameKitClientConfig.yml" which is generated by GAMEKIT each time you deploy or re-deploy a feature.
     * The file is loaded by calling either GameKitSessionManagerInstanceCreate() or GameKitSessionManagerReloadConfigFile().
     *
     * @param sessionManagerInstance Pointer to GameKitSessionManager instance created with GameKitSessionManagerInstanceCreate().
     * @param featureType The feature to check.
     * @return True if the settings for the feature are loaded, false otherwise.
    */
    GAMEKIT_API bool GameKitSessionManagerAreSettingsLoaded(GAMEKIT_SESSIONMANAGER_INSTANCE_HANDLE sessionManagerInstance, GameKit::FeatureType featureType);

    /**
     * @brief Replace any loaded client settings with new settings from the provided file.
     *
     * @param sessionManagerInstance Pointer to GameKitSessionManager instance created with GameKitSessionManagerInstanceCreate().
     * @param clientConfigFile Relative filepath to the generated file "awsGameKitClientConfig.yml".
     * The config file is generated by GAMEKIT each time a feature is deployed or re-deployed, and has settings for each GAMEKIT feature you've deployed.
     */
    GAMEKIT_API void GameKitSessionManagerReloadConfigFile(GAMEKIT_SESSIONMANAGER_INSTANCE_HANDLE sessionManagerInstance, const char* clientConfigFile);

    /**
     * @brief Replace any loaded client settings with new settings from the provided file.
     *
     * @param sessionManagerInstance Pointer to GameKitSessionManager instance created with GameKitSessionManagerInstanceCreate().
     * @param clientConfigFileContents Contents of "awsGameKitClientConfig.yml".
     * The config file is generated by GAMEKIT each time a feature is deployed or re-deployed, and has settings for each GAMEKIT feature you've deployed.
     */
    GAMEKIT_API void GameKitSessionManagerReloadConfigContents(GAMEKIT_SESSIONMANAGER_INSTANCE_HANDLE sessionManagerInstance, const char* clientConfigFileContents);

    /**
     * @brief Sets a token's value.
     * @param sessionManagerInstance Pointer to GameKitSessionManager instance created with GameKitSessionManagerInstanceCreate().
     * @param tokenType The type of token to set.
     * @param value The value of the token.
    */
    GAMEKIT_API void GameKitSessionManagerSetToken(GAMEKIT_SESSIONMANAGER_INSTANCE_HANDLE sessionManagerInstance, GameKit::TokenType tokenType, const char* value);

    /**
     * @brief Destroy the provided GameKitSessionManager instance.
     *
     * @param sessionManagerInstance Pointer to GameKitSessionManager instance created with GameKitSessionManagerInstanceCreate().
     */
    GAMEKIT_API void GameKitSessionManagerInstanceRelease(GAMEKIT_SESSIONMANAGER_INSTANCE_HANDLE sessionManagerInstance);
}
