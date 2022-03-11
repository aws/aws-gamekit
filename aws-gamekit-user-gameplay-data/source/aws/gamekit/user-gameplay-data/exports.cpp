// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// GameKit
#include <aws/gamekit/core/feature_resources.h>
#include <aws/gamekit/core/awsclients/default_clients.h>
#include <aws/gamekit/user-gameplay-data/exports.h>
#include <aws/gamekit/user-gameplay-data/gamekit_user_gameplay_data.h>

using namespace GameKit::Logger;
using namespace GameKit::UserGameplayData;

GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE GameKitUserGameplayDataInstanceCreateWithSessionManager(void* sessionManager, FuncLogCallback logCb)
{
    Logging::Log(logCb, Level::Info, "UserGameplayData Instance Create with default settings.");
    GameKit::Authentication::GameKitSessionManager* sessMgr = (GameKit::Authentication::GameKitSessionManager*)sessionManager;
    UserGameplayData* gameplayData = new UserGameplayData(sessMgr, logCb);

    return (GameKit::GameKitFeature*)gameplayData;
}

void GameKitSetUserGameplayDataClientSettings(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, GameKit::UserGameplayDataClientSettings settings)
{
    ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->SetClientSettings(settings);
}

//Create
unsigned int GameKitAddUserGameplayData(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, GameKit::UserGameplayDataBundle userGameplayDataBundle, DISPATCH_RECEIVER_HANDLE unprocessedItemsReceiver, FuncBundleResponseCallback unprocessedItemsCallback)
{
    return ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->AddUserGameplayData(userGameplayDataBundle, unprocessedItemsReceiver, unprocessedItemsCallback);
}

//Read
unsigned int GameKitListUserGameplayDataBundles(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, DISPATCH_RECEIVER_HANDLE receiver, FuncListGameplayDataBundlesResponseCallback responseCallback)
{
    return ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->ListUserGameplayDataBundles(receiver, responseCallback);
}

unsigned int GameKitGetUserGameplayDataBundle(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, char* bundleName, DISPATCH_RECEIVER_HANDLE receiver, FuncBundleResponseCallback responseCallback)
{
    return ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->GetUserGameplayDataBundle(bundleName, receiver, responseCallback);
}

unsigned int GameKitGetUserGameplayDataBundleItem(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, GameKit::UserGameplayDataBundleItem userGameplayDataBundleItem, DISPATCH_RECEIVER_HANDLE receiver, FuncBundleItemResponseCallback responseCallback)
{
    return ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->GetUserGameplayDataBundleItem(userGameplayDataBundleItem, receiver, responseCallback);
}

//Update
unsigned int GameKitUpdateUserGameplayDataBundleItem(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, GameKit::UserGameplayDataBundleItemValue userGameplayDataBundleItemValue)
{
    return ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->UpdateUserGameplayDataBundleItem(userGameplayDataBundleItemValue);
}

//Destroy
unsigned int GameKitDeleteAllUserGameplayData(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance)
{
    return ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->DeleteAllUserGameplayData();
}

unsigned int GameKitDeleteUserGameplayDataBundle(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, char* bundleName)
{
    return ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->DeleteUserGameplayDataBundle(bundleName);
}

unsigned int GameKitDeleteUserGameplayDataBundleItems(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, GameKit::UserGameplayDataDeleteItemsRequest deleteItemsRequest)
{
    return ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->DeleteUserGameplayDataBundleItems(deleteItemsRequest);
}

void GameKitUserGameplayDataInstanceRelease(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance)
{
    delete((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance));
}

void GameKitUserGameplayDataStartRetryBackgroundThread(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance)
{
    ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->StartRetryBackgroundThread();
}

void GameKitUserGameplayDataStopRetryBackgroundThread(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance)
{
    ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->StopRetryBackgroundThread();
}

void GameKitUserGameplayDataSetNetworkChangeCallback(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, NETWORK_STATE_RECEIVER_HANDLE receiverHandle, NetworkStatusChangeCallback statusChangeCallback)
{
    ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->SetNetworkChangeCallback(receiverHandle, statusChangeCallback);
}

void GameKitUserGameplayDataSetCacheProcessedCallback(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, CACHE_PROCESSED_RECEIVER_HANDLE receiverHandle, CacheProcessedCallback cacheProcessedCallback)
{
    ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->SetCacheProcessedCallback(receiverHandle, cacheProcessedCallback);
}

void GameKitUserGameplayDataDropAllCachedEvents(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance)
{
    ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->DropAllCachedEvents();
}

unsigned int GameKitUserGameplayDataPersistApiCallsToCache(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, const char* offlineCacheFile)
{
    return ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->PersistApiCallsToCache(offlineCacheFile);
}

unsigned int GameKitUserGameplayDataLoadApiCallsFromCache(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, const char* offlineCacheFile)
{
    return ((UserGameplayData*)((GameKit::GameKitFeature*)userGameplayDataInstance))->LoadApiCallsFromCache(offlineCacheFile);
}
