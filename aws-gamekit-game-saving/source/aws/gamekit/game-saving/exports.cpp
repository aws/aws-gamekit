// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// GameKit
#include <aws/gamekit/game-saving/exports.h>
#include <aws/gamekit/game-saving/gamekit_game_saving.h>

using namespace GameKit::Logger;
using namespace GameKit::GameSaving;

GAMEKIT_GAME_SAVING_INSTANCE_HANDLE GameKitGameSavingInstanceCreateWithSessionManager(void* sessionManager, FuncLogCallback logCb, const char* const* localSlotInformationFilePaths,
    unsigned int arraySize, FileActions fileActions)
{
    Logging::Log(logCb, Level::Info, "GameDevGameSavingCreate");

    auto const sessMgr = static_cast<GameKit::Authentication::GameKitSessionManager*>(sessionManager);
    auto const gameSaving = new GameSaving(sessMgr, logCb, localSlotInformationFilePaths, arraySize, fileActions);

    return gameSaving;
}

void GameKitAddLocalSlots(GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance, const char* const* localSlotInformationFilePaths, unsigned int arraySize)
{
    return static_cast<GameSaving*>(gameSavingInstance)->AddLocalSlots(localSlotInformationFilePaths, arraySize);
}

void GameKitSetFileActions(GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance, FileActions fileActions)
{
    return static_cast<GameSaving*>(gameSavingInstance)->SetFileActions(fileActions);
}

unsigned int GameKitGetAllSlotSyncStatuses(
    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance,
    DISPATCH_RECEIVER_HANDLE receiver,
    GameSavingResponseCallback resultCb,
    bool waitForAllPages,
    unsigned int pageSize)
{
    return static_cast<GameSaving*>(gameSavingInstance)->GetAllSlotSyncStatuses(receiver, resultCb, waitForAllPages, pageSize);
}

unsigned int GameKitGetSlotSyncStatus(GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance, DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, const char* slotName)
{
    return static_cast<GameSaving*>(gameSavingInstance)->GetSlotSyncStatus(receiver, resultCb, slotName);
}

unsigned int GameKitDeleteSlot(GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance, DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, const char* slotName)
{
    return static_cast<GameSaving*>(gameSavingInstance)->DeleteSlot(receiver, resultCb, slotName);
}

unsigned int GameKitSaveSlot(GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance, DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, GameSavingModel model)
{
    return static_cast<GameSaving*>(gameSavingInstance)->SaveSlot(receiver, resultCb, model);
}

unsigned int GameKitLoadSlot(GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance, DISPATCH_RECEIVER_HANDLE receiver, GameSavingDataResponseCallback resultCb, GameSavingModel model)
{
    return static_cast<GameSaving*>(gameSavingInstance)->LoadSlot(receiver, resultCb, model);
}

void GameKitGameSavingInstanceRelease(GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance)
{
    delete static_cast<GameSaving*>(gameSavingInstance);
}
