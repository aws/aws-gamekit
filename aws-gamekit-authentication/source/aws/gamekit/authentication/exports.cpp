// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/authentication/exports.h>
#include <aws/gamekit/authentication/gamekit_session_manager.h>

using namespace GameKit::Logger;

GAMEKIT_API GAMEKIT_SESSIONMANAGER_INSTANCE_HANDLE GameKitSessionManagerInstanceCreate(const char* clientConfigFile, FuncLogCallback logCb)
{
    Logging::Log(logCb, Level::Info, "GameKitSessionManagerInstanceCreate");
    if (clientConfigFile == nullptr)
    {
        clientConfigFile = "";
    }

    GameKit::Authentication::GameKitSessionManager* sessionManager = new GameKit::Authentication::GameKitSessionManager(clientConfigFile, logCb);
    return sessionManager;
}

GAMEKIT_API bool GameKitSessionManagerAreSettingsLoaded(GAMEKIT_SESSIONMANAGER_INSTANCE_HANDLE sessionManagerInstance, GameKit::FeatureType featureType)
{
    return ((GameKit::Authentication::GameKitSessionManager*)sessionManagerInstance)->AreSettingsLoaded(featureType);
}

GAMEKIT_API void GameKitSessionManagerReloadConfigFile(GAMEKIT_SESSIONMANAGER_INSTANCE_HANDLE sessionManagerInstance, const char* clientConfigFile)
{
    ((GameKit::Authentication::GameKitSessionManager*)sessionManagerInstance)->ReloadConfigFile(clientConfigFile);
}

GAMEKIT_API void GameKitSessionManagerReloadConfigContents(GAMEKIT_SESSIONMANAGER_INSTANCE_HANDLE sessionManagerInstance, const char* clientConfigFileContents)
{
    std::string fileContents(clientConfigFileContents);    
    ((GameKit::Authentication::GameKitSessionManager*)sessionManagerInstance)->ReloadConfigFromFileContents(fileContents);
}

GAMEKIT_API void GameKitSessionManagerSetToken(GAMEKIT_SESSIONMANAGER_INSTANCE_HANDLE sessionManagerInstance, GameKit::TokenType tokenType, const char* value)
{
    ((GameKit::Authentication::GameKitSessionManager*)sessionManagerInstance)->SetToken(tokenType, value);
}

GAMEKIT_API void GameKitSessionManagerInstanceRelease(GAMEKIT_SESSIONMANAGER_INSTANCE_HANDLE sessionManagerInstance)
{
    delete((GameKit::Authentication::GameKitSessionManager*)sessionManagerInstance);
}
