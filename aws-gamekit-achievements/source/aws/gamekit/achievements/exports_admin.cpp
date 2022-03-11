// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// Standard library
#include <exception>
#include <string>

// GameKit
#include <aws/gamekit/achievements/exports_admin.h>
#include <aws/gamekit/achievements/gamekit_admin_achievements.h>
#include <aws/gamekit/core/feature_resources.h>
#include <aws/gamekit/core/gamekit_feature.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/awsclients/default_clients.h>

using namespace GameKit::Logger;
using namespace GameKit::Achievements;

GAMEKIT_ADMIN_ACHIEVEMENTS_INSTANCE_HANDLE GameKitAdminAchievementsInstanceCreateWithSessionManager(void* sessionManager, const char* cloudResourcesPath, const GameKit::AccountCredentials accountCredentials, GameKit::AccountInfo accountInfo, FuncLogCallback logCb)
{
    Logging::Log(logCb, Level::Info, "GameDevAdminAchievementsCreate");
    GameKit::Authentication::GameKitSessionManager* sessMgr = (GameKit::Authentication::GameKitSessionManager*)sessionManager;
    AdminAchievements* achievements = new AdminAchievements(logCb, sessMgr, std::string(cloudResourcesPath), accountInfo, accountCredentials);

    Aws::Client::ClientConfiguration clientConfig;
    GameKit::DefaultClients::SetDefaultClientConfiguration(sessMgr->GetClientSettings(), clientConfig);
    clientConfig.region = sessMgr->GetClientSettings()[GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_REGION].c_str();

    return (GameKit::GameKitFeature*)achievements;
}

unsigned int GameKitAdminListAchievements(GAMEKIT_ADMIN_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance, unsigned int pageSize, bool waitForAllPages, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback)
{
    return ((AdminAchievements*)((GameKit::GameKitFeature*)achievementsInstance))->ListAchievements(pageSize, waitForAllPages, dispatchReceiver, responseCallback);
}

unsigned int GameKitAdminAddAchievements(GAMEKIT_ADMIN_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance, const GameKit::Achievement* achievements, unsigned int batchSize)
{
    return ((AdminAchievements*)((GameKit::GameKitFeature*)achievementsInstance))->AddAchievements(achievements, batchSize);
}

unsigned int GameKitAdminDeleteAchievements(GAMEKIT_ADMIN_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance, const char* const* achievementIdentifiers, unsigned int batchSize)
{
    return ((AdminAchievements*)((GameKit::GameKitFeature*)achievementsInstance))->DeleteAchievements(achievementIdentifiers, batchSize);
}

unsigned int GameKitAdminCredentialsChanged(GAMEKIT_ADMIN_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance, const GameKit::AccountCredentials accountCredentials, const GameKit::AccountInfo accountInfo)
{
    return ((AdminAchievements*)((GameKit::GameKitFeature*)achievementsInstance))->ChangeCredentials(accountCredentials, accountInfo);
}

bool GameKitIsAchievementIdValid(const char* achievementId)
{
    // Valid ID is any combination of alphanumeric characters and underscores that doesn't begin or end with an underscore, length >= 2
    return std::regex_match(achievementId, std::regex("^[a-zA-Z0-9][a-zA-Z0-9_]*[a-zA-Z0-9]$"));
}

void GameKitAdminAchievementsInstanceRelease(GAMEKIT_ADMIN_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance)
{
    delete((AdminAchievements*)((GameKit::GameKitFeature*)achievementsInstance));
}
