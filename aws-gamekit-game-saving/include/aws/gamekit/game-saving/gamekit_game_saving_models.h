// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// GameKit
#include <aws/gamekit/core/exports.h>

#define S3_PRESIGNED_URL_DEFAULT_TIME_TO_LIVE_SECONDS 120

extern "C"
{
    /**
     * @brief The recommended action your game should take in order to keep the local and cloud save file in sync.
     */
    enum class SlotSyncStatus : uint8_t {

        /**
         * @brief This status should not be possible.
         */
        UNKNOWN = 0,

        /**
         * @brief No action needed.
         *
         * @details The cloud file and local file are the same. They both have the same last modified timestamp.
         */
        SYNCED = 1,

        /**
         * @brief You should call GameKitLoadSlot() to download a newer version of this save from the cloud.
         *
         * @details Either the save file does not exist locally, or
         * the save file exists locally, the cloud file is newer, and the local file has previously been uploaded from this device.
         */
        SHOULD_DOWNLOAD_CLOUD = 2,

        /**
         * @brief You should call GameKitSaveSlot() to upload the local save file to the cloud.
         *
         * @details Either the save slot does not exist in the cloud, or
         * the save slot exists in the cloud, the local file is newer, and the last time the cloud save was updated was from this device.
         */
        SHOULD_UPLOAD_LOCAL = 3,

        /**
         * @brief You should ask the player to select which file they want to keep: the local file or the cloud file.
         *
         * @details The local file and the cloud file are different, and based on their last modified timestamps it is not clear which file should be kept.
         *
         * @details This may happen when a player plays on multiple devices, and especially when played in offline mode across multiple devices.
         */
        IN_CONFLICT = 4
    };

    /**
     * @brief Contains local and cloud information about a cached slot.
     *
     * @details This is also the data that gets written to the SaveInfo.json files.
     */
    struct Slot {
        Slot() {}
        Slot(const char* slotName,
             const char* metadataLocal,
             const char* metadataCloud,
             int64_t sizeLocal,
             int64_t sizeCloud,
             int64_t lastModifiedLocal,
             int64_t lastModifiedCloud,
             int64_t lastSync,
             SlotSyncStatus slotSyncStatus) :
                slotName(slotName),
                metadataLocal(metadataLocal),
                metadataCloud(metadataCloud),
                sizeLocal(sizeLocal),
                sizeCloud(sizeCloud),
                lastModifiedLocal(lastModifiedLocal),
                lastModifiedCloud(lastModifiedCloud),
                lastSync(lastSync),
                slotSyncStatus(slotSyncStatus) {}

        /**
         * @brief The slot name matching one of the cached slots.
         */
        const char* slotName = "";

        /**
         * @brief An arbitrary string you have associated with this save file locally.
         *
         * @details For example, this could be used to store information you want to display in the UI before you download the save file from the cloud,
         * such as a friendly display name, a user provided description, the total playtime, the percentage of the game completed, etc.
         *
         * @details The string can be in any format (ex: JSON), fully supporting UTF-8 compliant characters. It is limited to 1410 bytes.
         */
        const char* metadataLocal = "";

        /**
         * @brief An arbitrary string you have associated with the cloud save file.
         *
         * @details See Slot::metadataLocal for details.
         */
        const char* metadataCloud = "";

        /**
         * @brief The size of the local save file in bytes.
         */
        int64_t sizeLocal = 0;

        /**
         * @brief The size of the cloud save file in bytes.
         */
        int64_t sizeCloud = 0;

        /**
         * @brief The last time the local save file was modified in epoch milliseconds.
         */
        int64_t lastModifiedLocal = 0;

        /**
         * @brief The last time the cloud save file was modified in epoch milliseconds.
         */
        int64_t lastModifiedCloud = 0;

        /**
         * @brief The last time the local save file was uploaded from this device or downloaded to this device.
         *
         * @details This time will be equal to lastModifiedLocal after calling GameKitSaveSlot(), and equal to lastModifiedCloud after calling GameKitLoadSlot().
         */
        int64_t lastSync = 0;

        /**
         * @brief The recommended action your game should take in order to keep the local and cloud save file in sync.
         */
        SlotSyncStatus slotSyncStatus = SlotSyncStatus::UNKNOWN;
    };

    /**
     * @brief A struct containing the request parameters for both GameKitSaveSlot() and GameKitLoadSlot().
     *
     * @details All parameters are required, unless marked otherwise.
     */
    struct GameSavingModel
    {
        GameSavingModel() {}
        GameSavingModel(const char* slotName,
                        const char* metadata,
                        int64_t epochTime,
                        bool overrideSync,
                        uint8_t* data,
                        unsigned int dataSize,
                        const char* localSlotInformationFilePath) :
                            slotName(slotName),
                            metadata(metadata),
                            epochTime(epochTime),
                            overrideSync(overrideSync),
                            data(data),
                            dataSize(dataSize),
                            localSlotInformationFilePath(localSlotInformationFilePath) {}
        GameSavingModel(const char* slotName,
                        const char* metadata,
                        int64_t epochTime,
                        bool overrideSync,
                        uint8_t* data,
                        unsigned int dataSize,
                        const char* localSlotInformationFilePath,
                        unsigned int urlTimeToLive,
                        bool consistentRead) :
                            slotName(slotName),
                            metadata(metadata),
                            epochTime(epochTime),
                            overrideSync(overrideSync),
                            data(data),
                            dataSize(dataSize),
                            localSlotInformationFilePath(localSlotInformationFilePath),
                            urlTimeToLive(urlTimeToLive),
                            consistentRead(consistentRead) {}
        /**
         * (SaveSlot - Required) The name of the save slot to upload to the cloud. The name may be new, it does not have to exist in the cached slots.
         *
         * (LoadSlot - Required) The name of the save slot to download from the cloud. The name must exist in the cached slots.
         */
        const char* slotName = "";

        /**
         * (SaveSlot - Optional) An arbitrary string you want to associate with the save file.
         *
         * For example, this could be used to store information you want to display in the UI before you download the save file from the cloud,
         * such as a friendly display name, a user provided description, the total playtime, the percentage of the game completed, etc.
         *
         * The string can be in any format (ex: JSON), fully supporting UTF-8 compliant characters. It is limited to 1410 bytes.
         */
        const char* metadata = "";

        /**
         * (SaveSlot - Optional) The millisecond epoch time of when the local save file was last modified in UTC.
         *
         * Defaults to 0. If 0, will use the system's current timestamp as the EpochTime. The default is useful for
         * save files which only exist in memory (i.e. they aren't persisted on the device).
         */
        int64_t epochTime = 0;

        /**
         * (SaveSlot & LoadSlot - Optional) If set to true, this method will ignore the SlotSyncStatus and override the cloud/local data.
         *
         * Set this to true when you are resolving a sync conflict.
         */
        bool overrideSync = false;

        /**
         * (LoadSlot - Required) An array of unsigned bytes large enough to contain the save file after downloading from the cloud.
         *
         * We recommend determining how many bytes are needed by caching the Slot array
         * from the most recent Game Saving API call before calling GameKitLoadSlot(). From this cached array, you
         * can get the Slot::sizeCloud of the slot you are going to download. Note: the sizeCloud will be incorrect
         * if the cloud save has been updated from another device since the last time this device cached the
         * Slot array. In that case, call GameKitGetSlotSyncStatus() to get the accurate size.
         *
         * Alternative to caching, you can call GameKitGetSlotSyncStatus(slotName) to get the size of the cloud file.
         * However, this has extra latency compared to caching the results of the previous Game Saving API call.
         */
        uint8_t* data = nullptr;

        /**
         * The number of bytes in the `data` array.
         */
        unsigned int dataSize = 0;

        /**
         * (SaveSlot & LoadSlot - Required) The absolute path and filename for where to save the SaveInfo.json file.
         */
        const char* localSlotInformationFilePath = nullptr;

        /**
         * (SaveSlot & LoadSlot - Optional) The time to live in seconds for the generated pre-signed S3 urls used to upload/download the save file to/from the cloud. Defaults to 120 seconds.
         */
        unsigned int urlTimeToLive = S3_PRESIGNED_URL_DEFAULT_TIME_TO_LIVE_SECONDS;

        /**
         * (SaveSlot & LoadSlot - Optional) Whether to use "Consistent Read" when querying from DynamoDB. Defaults to true.
         */
        bool consistentRead = true;
    };

    /**
     * @brief A static callback function that will be invoked by GameKitGetAllSlotSyncStatuses() upon completion of the call (both for success or failure).
     *
     * @param dispatchReceiver The `receiver` pointer that was passed into the Game Saving API.
     * @param syncedSlots An array of cached slots. If `callStatus` is true, then this is the complete set of cached slots tracked by the Game Saving instance.
     * If false, then this is a subset of the cached slots. This subset will not be returned to the callback again until the final call (when `callStatus` is true).
     * If the call failed, then this array will be empty.
     * @param slotCount The number of slots in the `syncedSlots` array.
     * @param complete If true, then this is the final call of this response callback. If false, the callback will be invoked again with the next page of data.
     * @param callStatus A GameKit status code indicating the result of the API call. Status codes are defined in errors.h.
     * See the specific API's documentation for a list of possible status codes the API may return.
     */
    typedef void(*GameSavingResponseCallback)(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const Slot* syncedSlots, unsigned int slotCount, bool complete, unsigned int callStatus);

    /**
     * @brief A static callback function that will be invoked by a Game Saving API upon completion of the call (both for success or failure).
     *
     * @details This callback signature is used by Game Saving APIs which act on a single save slot.
     *
     * @param dispatchReceiver The `receiver` pointer that was passed into the Game Saving API.
     * @param syncedSlots An array containing a copy of the current set of cached slots.
     * @param slotCount The number of slots in the `syncedSlots` array.
     * @param slot A copy of the cached slot that was acted on by the API. If the call failed, this slot is empty and should not be used.
     * This slot might not be valid once this object leaves scope (i.e. once this callback function completes).
     * @param callStatus A GameKit status code indicating the result of the API call. Status codes are defined in errors.h.
     * See the specific API's documentation for a list of possible status codes the API may return.
     */
    typedef void(*GameSavingSlotActionResponseCallback)(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const Slot* syncedSlots, unsigned int slotCount, Slot slot, unsigned int callStatus);

    /**
     * @brief A static callback function that will be invoked by GameKitLoadSlot() upon completion of the call (both for success or failure).
     *
     * @param dispatchReceiver The `receiver` pointer that was passed into the Game Saving API.
     * @param syncedSlots An array containing a copy of the current set of cached slots.
     * @param slotCount The number of slots in the `syncedSlots` array.
     * @param slot A copy of the cached slot that was downloaded. If the call failed, this slot is empty and should not be used.
     * This slot might not be valid once this object leaves scope (i.e. once this callback function completes).
     * @param data An array of unsigned bytes containing the downloaded file, or a nullptr if the call failed.
     * @param dataSize The size of the `data` array in bytes.
     * @param callStatus A GameKit status code indicating the result of the API call. Status codes are defined in errors.h.
     * See the specific API's documentation for a list of possible status codes the API may return.
     */
    typedef void(*GameSavingDataResponseCallback)(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const Slot* syncedSlots, unsigned int slotCount, Slot slot, const uint8_t* data, unsigned int dataSize, unsigned int callStatus);

    /**
     * @brief Save a byte array to a file, overwriting the file if it already exists.
     *
     * @param dispatchReceiver The pointer stored in FileActions::fileWriteDispatchReceiver. The implementer of this function may use this pointer however they find suitable.
     * For example, to point to a class instance on which this callback function can invoke a file-writing instance method.
     * @param filePath The absolute or relative path of the file to write to.
     * @param data The data to write to the file.
     * @param size The length of the `data` array.
     * @return True if the data was successfully written to the file, false otherwise.
     */
    typedef bool(*FileWriteCallback)(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* filePath, const uint8_t* data, const unsigned int size);

    /**
     * @brief Load a file into a byte array.
     *
     * @param dispatchReceiver The pointer stored in FileActions::fileReadDispatchReceiver. The implementer of this function may use this pointer however they find suitable.
     * For example, to point to a class instance on which this callback function can invoke a file-reading instance method.
     * @param filePath The absolute or relative path of the file to read from.
     * @param data The array to store the loaded data in. Must be pre-allocated with enough space to store the entire contents of the file. The caller of
     * this function must call `delete[] data` when finished with the data to prevent a memory leak.
     * @param size The length of the `data` array.
     * @return True if the data was successfully read from the file, false otherwise.
     */
    typedef bool(*FileReadCallback)(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* filePath, uint8_t* data, unsigned int size);

    /**
     * @brief Return the size of the file in bytes, or 0 if the file does not exist.
     *
     * @param dispatchReceiver The pointer stored in FileActions::fileSizeDispatchReceiver. The implementer of this function may use this pointer however they find suitable.
     * For example, to point to a class instance on which this callback function can invoke a size-getting instance method.
     * @param filePath The absolute or relative path of the file to check.
     * @return The file size in bytes, or 0 if the file does not exist.
     */
    typedef unsigned int(*FileGetSizeCallback)(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* filePath);

    /**
     * @brief A bundle of callback functions that provide file I/O for the Game Saving library.
     */
    struct FileActions
    {
        /**
         * @brief A callback function the Game Saving library will call when it needs to write to a file.
         */
        FileWriteCallback fileWriteCallback;

        /**
         * @brief A callback function the Game Saving library will call when it needs to load a file.
         */
        FileReadCallback fileReadCallback;

        /**
         * @brief A callback function the Game Saving library will call when it needs to get the size of a file.
         */
        FileGetSizeCallback fileSizeCallback;

        /**
         * @brief This pointer will be passed into FileActions::fileWriteCallback() whenever it is invoked.
         *
         * @details The implementer of FileActions::fileWriteCallback() may use this pointer however they find suitable.
         * For example, to point to an instance of a class on which to invoke a file writing instance method.
         */
        DISPATCH_RECEIVER_HANDLE fileWriteDispatchReceiver;

        /**
         * @brief This pointer will be passed into FileActions::fileReadCallback() whenever it is invoked.
         *
         * @details The implementer of FileActions::fileReadCallback() may use this pointer however they find suitable.
         * For example, to point to an instance of a class on which to invoke a file reading instance method.
         */
        DISPATCH_RECEIVER_HANDLE fileReadDispatchReceiver;

        /**
         * @brief This pointer will be passed into FileActions::fileSizeCallback() whenever it is invoked.
         *
         * @details The implementer of FileActions::fileSizeCallback() may use this pointer however they find suitable.
         * For example, to point to an instance of a class on which to invoke a file size-getting instance method.
         */
        DISPATCH_RECEIVER_HANDLE fileSizeDispatchReceiver;
    };
}
