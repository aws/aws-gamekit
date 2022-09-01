// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

/** @file
 * @brief The C interface for the Game Saving library.
 *
 * The Game Saving library provides APIs for storing game save files in the cloud and synchronizing them with local devices.
 *
 * ## Singleton
 * The Game Saving library is designed to be used as a singleton. During the life of your program you should only
 * create once instance of the Game Saving class through GameKitGameSavingInstanceCreateWithSessionManager().
 *
 * It's okay to create and initialize another instance of the Game Saving class if your singleton instance is destroyed.
 * Just make sure to initialize the new instance as described below.
 *
 * ## Initialization
 * The Game Saving library must be initialized exactly once by calling GameKitGameSavingInstanceCreateWithSessionManager() and optionally GameKitSetFileActions().
 * This initialization must be done before calling any other Game Saving APIs, and should only be done once per instance of your singleton.
 *
 * After the library is initialized, each time a user logs in GameKitClearSyncedSlots(), GameKitAddLocalSlots(), and GameKitGetAllSlotSyncStatuses() must be
 * called in that order to ensure all local and cloud slots are up to date.
 *
 * - GameKitGameSavingInstanceCreateWithSessionManager() creates an instance of the Game Saving class which you need to pass into every other Game Saving API.
 * - GameKitClearSyncedSlots() is called to ensure that any previous user's slots are cleared out properly. If preferred, this method can also be called when a user logs out. 
 * - GameKitAddLocalSlots() is optional if you already provided `localSlotInformationFilePaths` to the previous method. Either way, provding these paths
 *   this ensures Game Saving knows about local saves on the device that exist from previous times the game was played.
 * - GameKitGetAllSlotSyncStatuses() ensures Game Saving has the latest information about the cloud saves, knows which local saves are synchronized
 *   with the cloud, and which saves should be uploaded, downloaded, or need manual conflict resolution.
 *
 * ## Offline Mode
 * If your game is being played without internet, you must still call GameKitSaveSlot() and GameKitDeleteSlot() each time you would normally call these methods.
 * Otherwise, there is a risk that the progress made while playing offline will be overwritten the next time the game is played on this device with
 * an internet connection if a newer save has since been uploaded from another device.
 *
 * ## Save Slots
 * Save files that are uploaded/downloaded/tracked through this API are each associated with a named "save slot" for the player.
 *
 * When you deploy the Game Saving feature, you can configure the maximum number of cloud saves slots to provide each player. This limit can
 * prevent malicious players from storing too much data in the cloud. You can change this limit by doing another deployment of the Game Saving feature.
 *
 * ## Slot Information
 * The local and cloud attributes for a save slot are collectively known as "slot information" and are stored in the Slot struct.
 *
 * ## Cached Slots
 * This library maintains a cache of slot information for all slots it interacts with (both locally and in the cloud).
 * The cached slots are updated on every API call, and are also returned in the delegate of most API calls.
 *
 * ## SaveInfo.json Files
 * This library creates "SaveInfo.json" files on the device every time save files are uploaded/downloaded through the GameKitSaveSlot() and GameKitLoadSlot() APIs.
 *
 * The exact filenames and locations are provided by you. We highly recommended you store the SaveInfo.json files alongside their corresponding
 * save file to help developers and curious players to understand these files go together.
 *
 * The SaveInfo.json files are loaded during game startup either by passing the filepaths into GameKitGameSavingInstanceCreateWithSessionManager(), or by calling
 * GameKitAddLocalSlots() afterwards. This informs the library about any save files that exist on the device from previous game sessions.
 */

#pragma once
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/exports.h>
#include <aws/gamekit/core/enums.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/game-saving/gamekit_game_saving_models.h>

extern "C"
{
    /**
     * @brief A pointer to a GameSaving instance created with GameKitGameSavingInstanceCreateWithSessionManager().
     */
    typedef void* GAMEKIT_GAME_SAVING_INSTANCE_HANDLE;

    /**
     * @brief Create an instance of the GameSaving class, which can be used to access the other Game Saving APIs.
     *
     * @details Make sure to call GameKitGameSavingInstanceRelease() to destroy the returned object when finished with it, otherwise you'll have a memory leak.
     *
     * @param sessionManager Pointer to a SessionManager instance created with GameKitSessionManagerInstanceCreate().
     * @param logCb A callback function which the GameSaving instance can use to log information and errors.
     * @param localSlotInformationFilePaths Array of file paths for all of the player's SaveInfo.json files on the device. These paths are chosen by you when calling GameKitSaveSlot() and GameKitLoadSlot().
     * @param arraySize The number of filepaths in `localSlotInformationFilePaths`.
     * @param fileActions A struct of callbacks defining how to perform file I/O actions for the running platform.
     * @return A pointer to the new GameSaving instance.
    */
    GAMEKIT_API GAMEKIT_GAME_SAVING_INSTANCE_HANDLE GameKitGameSavingInstanceCreateWithSessionManager(
        void* sessionManager,
        FuncLogCallback logCb,
        const char* const* localSlotInformationFilePaths,
        unsigned int arraySize,
        FileActions fileActions);

    /**
     * @brief Load slot information for all of the player's local saves on the device.
     *
     * @details If the list of SaveInfo.json files was not provided to GameKitGameSavingInstanceCreateWithSessionManager(),
     * then this is the next method you should call before calling any other APIs on the Game Saving library. See the
     * file level documentation for more details on initialization.
     *
     * @details This method loads the SaveInfo.json files that were created on the device during previous game sessions when calling GameKitSaveSlot() and GameKitLoadSlot().
     * This overwrites any cached slots in memory which have the same slot name as the slots loaded from the SaveInfo.json files.
     *
     * @param gameSavingInstance A pointer to a GameSaving instance created with GameKitGameSavingInstanceCreateWithSessionManager().
     * @param localSlotInformationFilePaths Array of file paths for all of the player's SaveInfo.json files on the device. These paths are chosen by you when calling GameKitSaveSlot() and GameKitLoadSlot().
     * @param arraySize The number of filepaths in `localSlotInformationFilePaths`.
     */
    GAMEKIT_API void GameKitAddLocalSlots(GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance, const char* const* localSlotInformationFilePaths, unsigned int arraySize);

    /**
     * @brief Clears slot information for all of the feature's cached save slots. 
     *
     * @details This method should be called as soon as a user is logged out or before GameKitAddLocalSlots() is called after a user logs in..
     * Calling this method before calling GameKitAddLocalSlots() for a new user will ensure that a previous users cached slots will
     * not be present for the currently logged in user.
     *
     * @param gameSavingInstance A pointer to a GameSaving instance created with GameKitGameSavingInstanceCreateWithSessionManager().
     */
    GAMEKIT_API void GameKitClearSyncedSlots(GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance);

    /**
     * @brief Change the file I/O callbacks used by this library.
     *
     * @details If you didn't provide a set of FileActions to GameKitGameSavingInstanceCreateWithSessionManager(),
     * then this is the next method you should call before calling any other APIs on the Game Saving library
     * (even before GameKitAddLocalSlots() and GameKitClearSyncedSlots()). See the file level documentation for more details on initialization.
     *
     * @param gameSavingInstance A pointer to a GameSaving instance created with GameKitGameSavingInstanceCreateWithSessionManager().
     * @param fileActions A struct of callbacks defining how to perform file I/O actions for the running platform.
     */
    GAMEKIT_API void GameKitSetFileActions(GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance, FileActions fileActions);

    /**
     * @brief Get a complete and updated view of the player's save slots (both local and cloud).
     *
     * @details After calling this method, you should inspect the `syncedSlots` Slot array provided to the callback function and
     * take the recommended syncing action according to each slot's Slot::slotSyncStatus.
     *
     * @details Call this method during initialization (see class level documentation) and any time you suspect the cloud saves may have
     * been updated from another device.
     *
     * @details This method adds cached slots for all cloud saves not currently on the device, updates all cached slots with accurate cloud attributes,
     * and marks the Slot::slotSyncStatus member of all cached slots with the recommended syncing action you should take.
     *
     * @param gameSavingInstance A pointer to a GameSaving instance created with GameKitGameSavingInstanceCreateWithSessionManager().
     * @param receiver (Optional) This pointer will be passed to the callback function as the `dispatchReceiver`.
     * @param resultCb The callback function to invoke and return data to when the method has finished.
     * @param waitForAllPages If true, the `resultCb` will not be invoked until this method fully completes. If false, it will be invoked after each page of slots are updated.
     * @param pageSize If waitForAllPages is false, then this is the number of slots to return during each invocation of the callback. Otherwise this parameter is ignored.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (identity/exports.h) before calling this method.
     * - GAMEKIT_ERROR_HTTP_REQUEST_FAILED: The backend HTTP request failed. Check the logs to see what the HTTP response code was.
     * - GAMEKIT_ERROR_PARSE_JSON_FAILED: The backend returned a malformed JSON payload. This should not happen. If it does, it indicates there is a bug in the backend code.
     * - GAMEKIT_ERROR_SETTINGS_MISSING: One or more settings required for calling the backend are missing and the backend wasn't called. Verify the feature is deployed and the config is correct.
     */
    GAMEKIT_API unsigned int GameKitGetAllSlotSyncStatuses(
        GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance,
        DISPATCH_RECEIVER_HANDLE receiver,
        GameSavingResponseCallback resultCb,
        bool waitForAllPages,
        unsigned int pageSize);

    /**
     * @brief Get an updated view and recommended syncing action for the player's specific save slot.
     *
     * @details This method updates the specific save slot's cloud attributes and marks the Slot::slotSyncStatus member with the recommended syncing action you should take.
     *
     * @param gameSavingInstance A pointer to a GameSaving instance created with GameKitGameSavingInstanceCreateWithSessionManager().
     * @param receiver (Optional) This pointer will be passed to the callback function as the `dispatchReceiver`.
     * @param resultCb The callback function to invoke and return data to when the method has finished.
     * @param slotName The name of the cached slot to update.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (identity/exports.h) before calling this method.
     * - GAMEKIT_ERROR_GAME_SAVING_MALFORMED_SLOT_NAME: The provided slot name is malformed. Check the logs to see what the required format is.
     * - GAMEKIT_ERROR_GAME_SAVING_SLOT_NOT_FOUND: The provided slot name was not found in the cached slots. This either means you have a typo in the slot name,
     *                                             or the slot only exists in the cloud and you need to call GameKitGetAllSlotSyncStatuses() first before calling this method.
     * - GAMEKIT_ERROR_HTTP_REQUEST_FAILED: The backend HTTP request failed. Check the logs to see what the HTTP response code was.
     * - GAMEKIT_ERROR_PARSE_JSON_FAILED: The backend returned a malformed JSON payload. This should not happen. If it does, it indicates there is a bug in the backend code.
     * - GAMEKIT_ERROR_SETTINGS_MISSING: One or more settings required for calling the backend are missing and the backend wasn't called. Verify the feature is deployed and the config is correct.
     */
    GAMEKIT_API unsigned int GameKitGetSlotSyncStatus(
        GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance,
        DISPATCH_RECEIVER_HANDLE receiver,
        GameSavingSlotActionResponseCallback resultCb,
        const char* slotName);

    /**
     * @brief Delete the player's cloud save slot and remove it from the cached slots.
     *
     * @details No local files are deleted from the device. Data is only deleted from the cloud and from memory (the cached slot).
     *
     * @details After calling GameKitDeleteSlot(), you'll probably want to delete the local save file and corresponding SaveInfo.json file from the device.
     * If you keep the SaveInfo.json file, then next time the game boots up this library will recommend re-uploading the save file to the cloud when
     * you call GameKitGetAllSlotSyncStatuses() or GameKitGetSlotSyncStatus().
     *
     * @details If your game is being played without internet, you must still call this method and delete the SaveInfo.json file as normal to avoid the risk
     * of having the offline progress be overwritten when internet connectivity is restored. See the "Offline Mode" section in the file level documentation for more details.
     *
     * @param gameSavingInstance A pointer to a GameSaving instance created with GameKitGameSavingInstanceCreateWithSessionManager().
     * @param receiver (Optional) This pointer will be passed to the callback function as the `dispatchReceiver`.
     * @param resultCb The callback function to invoke and return data to when the method has finished.
     * @param slotName The name of the cached slot to delete.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (identity/exports.h) before calling this method.
     * - GAMEKIT_ERROR_GAME_SAVING_MALFORMED_SLOT_NAME: The provided slot name is malformed. Check the logs to see what the required format is.
     * - GAMEKIT_ERROR_GAME_SAVING_SLOT_NOT_FOUND: The provided slot name was not found in the cached slots. This either means you have a typo in the slot name,
     *                                             or the slot only exists in the cloud and you need to call GameKitGetAllSlotSyncStatuses() first before calling this method.
     * - GAMEKIT_ERROR_HTTP_REQUEST_FAILED: The backend HTTP request failed. Check the logs to see what the HTTP response code was.
     * - GAMEKIT_ERROR_PARSE_JSON_FAILED: The backend returned a malformed JSON payload. This should not happen. If it does, it indicates there is a bug in the backend code.
     * - GAMEKIT_ERROR_SETTINGS_MISSING: One or more settings required for calling the backend are missing and the backend wasn't called. Verify the feature is deployed and the config is correct.
     */
    GAMEKIT_API unsigned int GameKitDeleteSlot(
        GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance,
        DISPATCH_RECEIVER_HANDLE receiver,
        GameSavingSlotActionResponseCallback resultCb,
        const char* slotName);

    /**
     * @brief Upload a data buffer to the cloud, overwriting the player's cloud slot if it already exists.
     *
     * @details Also write the slot's information to a SaveInfo.json file on the device, and add the slot to the cached slots if it doesn't already exist.
     * This SaveInfo.json file should be passed into GameKitGameSavingInstanceCreateWithSessionManager() or GameKitAddLocalSlots() when you initialize the
     * Game Saving library in the future.
     *
     * @details If your game is being played without internet, you must still call this method as normal to avoid the risk of having the offline progress be
     * overwritten when internet connectivity is restored. See the "Offline Mode" section in the file level documentation for more details.
     *
     * @param gameSavingInstance A pointer to a GameSaving instance created with GameKitGameSavingInstanceCreateWithSessionManager().
     * @param receiver (Optional) This pointer will be passed to the callback function as the `dispatchReceiver`.
     * @param resultCb The callback function to invoke and return data to when the method has finished.
     * @param model A struct containing all required fields for saving local data to the cloud.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (identity/exports.h) before calling this method.
     * - GAMEKIT_ERROR_GAME_SAVING_MALFORMED_SLOT_NAME: The provided slot name is malformed. Check the logs to see what the required format is.
     * - GAMEKIT_ERROR_FILE_WRITE_FAILED: The SaveInfo.json file was unable to be written to the device. If using the default file I/O callbacks,
     *                                    check the logs to see the root cause. If the platform is not supported by the default file I/O callbacks,
     *                                    use GameKitSetFileActions() to provide your own callbacks. See GameKitSetFileActions() for more details.
     * - GAMEKIT_ERROR_GAME_SAVING_MAX_CLOUD_SLOTS_EXCEEDED: The upload was cancelled because it would have caused the player to exceed their "maximum cloud save slots limit". This limit
     *                                                       was configured when you deployed the Game Saving feature and can be changed by doing another deployment.
     * - GAMEKIT_ERROR_GAME_SAVING_EXCEEDED_MAX_SIZE: The `metadata` member of your `model` object is too large. Please see the documentation on GameSavingModel::metadata for details.
     * - GAMEKIT_ERROR_GAME_SAVING_SYNC_CONFLICT: The upload was cancelled to prevent overwriting the player's progress. This most likely indicates the player has played on multiple
     *                                            devices without having their progress properly synced with the cloud at the start and end of their play sessions. We recommend you inform
     *                                            the player of this conflict and present them with a choice - keep the cloud save or keep the local save. Then call GameKitSaveSlot() or
     *                                            GameKitLoadSlot() with overrideSync=true to override the cloud/local file.
     * - GAMEKIT_ERROR_GAME_SAVING_CLOUD_SLOT_IS_NEWER: The upload was cancelled because the cloud save file is newer than the file you attempted to upload. Treat this like a
     *                                                  GAMEKIT_ERROR_GAME_SAVING_SYNC_CONFLICT because the local and cloud save might have non-overlapping game progress.
     * - GAMEKIT_ERROR_HTTP_REQUEST_FAILED: The backend HTTP request failed. Check the logs to see what the HTTP response code was.
     * - GAMEKIT_ERROR_PARSE_JSON_FAILED: The backend returned a malformed JSON payload. This should not happen. If it does, it indicates there is a bug in the backend code.
     * - GAMEKIT_ERROR_SETTINGS_MISSING: One or more settings required for calling the backend are missing and the backend wasn't called. Verify the feature is deployed and the config is correct.
     */
    GAMEKIT_API unsigned int GameKitSaveSlot(
        GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance,
        DISPATCH_RECEIVER_HANDLE receiver,
        GameSavingSlotActionResponseCallback resultCb,
        GameSavingModel model);

    /**
     * @brief Download the player's cloud slot into a local data buffer.
     *
     * @details Also write the slot's information to a SaveInfo.json file on the device.
     * This SaveInfo.json file should be passed into AddLocalSlots() when you initialize the Game Saving library in the future.
     *
     * @param gameSavingInstance A pointer to a GameSaving instance created with GameKitGameSavingInstanceCreateWithSessionManager().
     * @param receiver (Optional) This pointer will be passed to the callback function as the `dispatchReceiver`.
     * @param resultCb The callback function to invoke and return data to when the method has finished.
     * @param model A struct containing all required fields for loading data from the cloud.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (identity/exports.h) before calling this method.
     * - GAMEKIT_ERROR_GAME_SAVING_MALFORMED_SLOT_NAME: The provided slot name is malformed. Check the logs to see what the required format is.
     * - GAMEKIT_ERROR_GAME_SAVING_SLOT_NOT_FOUND: The provided slot name was not found in the cached slots. This either means you have a typo in the slot name,
     *                                             or the slot only exists in the cloud and you need to call GameKitGetAllSlotSyncStatuses() first before calling this method.
     * - GAMEKIT_ERROR_FILE_WRITE_FAILED: The SaveInfo.json file was unable to be written to the device. If using the default file I/O callbacks,
     *                                    check the logs to see the root cause. If the platform is not supported by the default file I/O callbacks,
     *                                    use GameKitSetFileActions() to provide your own callbacks. GameKitSee SetFileActions() for more details.
     * - GAMEKIT_ERROR_GAME_SAVING_SYNC_CONFLICT: The download was cancelled to prevent overwriting the player's progress. This most likely indicates the player has played on multiple
     *                                            devices without having their progress properly synced with the cloud at the start and end of their play sessions. We recommend you inform
     *                                            the player of this conflict and present them with a choice - keep the cloud save or keep the local save. Then call GameKitSaveSlot() or
     *                                            GameKitLoadSlot() with OverrideSync=true to override the cloud/local file.
     * - GAMEKIT_ERROR_GAME_SAVING_LOCAL_SLOT_IS_NEWER: The download was cancelled because the local save file is newer than the cloud file you attempted to download. Treat this like a
     *                                                  GAMEKIT_ERROR_GAME_SAVING_SYNC_CONFLICT because the local and cloud save might have non-overlapping game progress.
     * - GAMEKIT_ERROR_GAME_SAVING_SLOT_UNKNOWN_SYNC_STATUS: The download was cancelled because the sync status could not be determined. Treat this like a GAMEKIT_ERROR_GAME_SAVING_SYNC_CONFLICT.
     * - GAMEKIT_ERROR_GAME_SAVING_MISSING_SHA: The S3 file is missing a SHA-256 metadata attribute and therefore the validity of the file could not be determined. This should not happen. If it does,
     *                                          this indicates there is a bug in the backend code.
     * - GAMEKIT_ERROR_GAME_SAVING_SLOT_TAMPERED: The SHA-256 hash of the downloaded file does not match the SHA-256 hash of the original file that was uploaded to S3. This indicates the downloaded
     *                                            file was corrupted or tampered with. You should try downloading again to rule out the possibility of random corruption.
     * - GAMEKIT_ERROR_GAME_SAVING_BUFFER_TOO_SMALL: The data buffer you provided in the Request object is not large enough to hold the downloaded S3 file. This likely means a newer version of the
     *                                               cloud file was uploaded from another device since the last time you called GameKitGetAllSlotSyncStatuses() or GameKitGetSlotSyncStatus() on this
     *                                               device. To resolve, call GameKitGetSlotSyncStatus() to get the up-to-date size of the cloud file.
     * - GAMEKIT_ERROR_HTTP_REQUEST_FAILED: The backend HTTP request failed. Check the logs to see what the HTTP response code was.
     * - GAMEKIT_ERROR_PARSE_JSON_FAILED: The backend returned a malformed JSON payload. This should not happen. If it does, it indicates there is a bug in the backend code.
     * - GAMEKIT_ERROR_SETTINGS_MISSING: One or more settings required for calling the backend are missing and the backend wasn't called. Verify the feature is deployed and the config is correct.
     */
    GAMEKIT_API unsigned int GameKitLoadSlot(
        GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance,
        DISPATCH_RECEIVER_HANDLE receiver,
        GameSavingDataResponseCallback resultCb,
        GameSavingModel model);

    /**
     * @brief Destroy the passed in GameSaving instance.
     *
     * @details Make sure to destroy every GameSaving instance when finished with it in order to prevent a memory leak.
     *
     * @param gameSavingInstance A pointer to a GameSaving instance created with GameKitGameSavingInstanceCreateWithSessionManager().
    */
    GAMEKIT_API void GameKitGameSavingInstanceRelease(GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance);
}
