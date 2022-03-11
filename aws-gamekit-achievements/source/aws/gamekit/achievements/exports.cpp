// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// Standard library
#include <exception>
#include <string>

// GameKit
#include <aws/gamekit/achievements/exports.h>
#include <aws/gamekit/achievements/gamekit_achievements.h>
#include <aws/gamekit/core/feature_resources.h>
#include <aws/gamekit/core/gamekit_feature.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/awsclients/default_clients.h>

using namespace GameKit::Logger;
using namespace GameKit::Achievements;

GAMEKIT_ACHIEVEMENTS_INSTANCE_HANDLE GameKitAchievementsInstanceCreateWithSessionManager(void* sessionManager, FuncLogCallback logCb)
{
    Logging::Log(logCb, Level::Info, "GameDevAchievementsCreate");
    GameKit::Authentication::GameKitSessionManager* sessMgr = (GameKit::Authentication::GameKitSessionManager*)sessionManager;
    Achievements* achievements = new Achievements(logCb, sessMgr);

    Aws::Client::ClientConfiguration clientConfig;
    GameKit::DefaultClients::SetDefaultClientConfiguration(sessMgr->GetClientSettings(), clientConfig);
    clientConfig.region = sessMgr->GetClientSettings()[GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_REGION].c_str();

    return (GameKit::GameKitFeature*)achievements;
}

unsigned int GameKitListAchievements(GAMEKIT_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance, unsigned int pageSize, bool waitForAllPages, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback)
{
    return ((Achievements*)((GameKit::GameKitFeature*)achievementsInstance))->ListAchievementsForPlayer(pageSize, waitForAllPages, dispatchReceiver, responseCallback);
}

unsigned int GameKitUpdateAchievement(GAMEKIT_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance, const char* achievementIdentifier, unsigned int incrementBy, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback)
{
    return ((Achievements*)((GameKit::GameKitFeature*)achievementsInstance))->UpdateAchievementForPlayer(achievementIdentifier, incrementBy, dispatchReceiver, responseCallback);
}

unsigned int GameKitGetAchievement(GAMEKIT_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance, const char* achievementIdentifier,
    const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback)
{
    return ((Achievements*)((GameKit::GameKitFeature*)achievementsInstance))->GetAchievementForPlayer(achievementIdentifier, dispatchReceiver, responseCallback);
}

GAMEKIT_API unsigned int GameKitGetAchievementIconsBaseUrl(GAMEKIT_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback)
{
    auto url = ((Achievements*)((GameKit::GameKitFeature*)achievementsInstance))->GetSessionManager()->GetClientSettings()[GameKit::ClientSettings::Achievements::SETTINGS_ACHIEVEMENTS_ICONS_BASE_URL];
    responseCallback(dispatchReceiver, url.append("/").c_str());
    return GameKit::GAMEKIT_SUCCESS;
}

void GameKitAchievementsInstanceRelease(GAMEKIT_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance)
{
    delete((Achievements*)((GameKit::GameKitFeature*)achievementsInstance));
}
