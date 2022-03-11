// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <memory>
#include <string>
#include <unordered_set>

// AWS SDK
#include <aws/core/http/HttpClient.h>
#include <aws/core/utils/base64/Base64.h>
#include <aws/core/utils/crypto/Sha256.h>

// GameKit
#include <aws/gamekit/authentication/gamekit_session_manager.h>
#include <aws/gamekit/core/gamekit_feature.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/utils/current_time_provider.h>
#include <aws/gamekit/core/utils/file_utils.h>
#include <aws/gamekit/core/utils/validation_utils.h>
#include <aws/gamekit/game-saving/gamekit_game_saving_cached_slot.h>
#include <aws/gamekit/game-saving/gamekit_game_saving_caller.h>

// Workaround for conflict with user.h PAGE_SIZE macro when compiling for Android
#pragma push_macro("PAGE_SIZE")
#undef PAGE_SIZE

namespace GameKit
{
    class IGameSaving
    {
    public:
        IGameSaving() {};
        virtual ~IGameSaving() {};

        virtual void AddLocalSlots(const char* const* localSlotInformationFilePaths, unsigned int arraySize) = 0;
        virtual void SetFileActions(FileActions fileActions) = 0;
        virtual unsigned int GetAllSlotSyncStatuses(DISPATCH_RECEIVER_HANDLE receiver, GameSavingResponseCallback resultCb, bool waitForAllPages, unsigned int pageSize) = 0;
        virtual unsigned int GetSlotSyncStatus(DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, const char* slotName) = 0;
        virtual unsigned int DeleteSlot(DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, const char* slotName) = 0;
        virtual unsigned int SaveSlot(DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, GameSavingModel model) = 0;
        virtual unsigned int LoadSlot(DISPATCH_RECEIVER_HANDLE receiver, GameSavingDataResponseCallback resultCb, GameSavingModel model) = 0;
    };

    namespace GameSaving
    {
        /**
         * @brief See game-saving/exports.h for most of the documentation.
         */
        class GameSaving : GameKitFeature, IGameSaving
        {
        private:
            #pragma region Constants
            static const unsigned int MAX_PAGE_SIZE = 100;
            static const unsigned int MAX_METADATA_BYTES = 1410;
            static const std::string START_KEY;
            static const std::string PAGING_TOKEN;
            static const std::string PAGE_SIZE;
            static const std::string METADATA;
            static const std::string HASH;
            static const std::string TIME_TO_LIVE;
            static const std::string LAST_MODIFIED_EPOCH_TIME;
            static const std::string CONSISTENT_READ;

            static const Aws::String S3_SHA_256_METADATA_HEADER;
            static const Aws::String S3_SLOT_METADATA_HEADER;
            static const Aws::String S3_EPOCH_METADATA_HEADER;
            #pragma endregion

            Authentication::GameKitSessionManager* m_sessionManager;
            std::shared_ptr<Aws::Http::HttpClient> m_httpClient;
            std::shared_ptr<Utils::ICurrentTimeProvider> m_currentTimeProvider;
            std::unordered_map<std::string, CachedSlot> m_syncedSlots;
            std::mutex m_gameSavingMutex;
            Caller m_caller;

            FileWriteCallback m_fileWriteCallback;
            FileReadCallback m_fileReadCallback;
            FileGetSizeCallback m_fileSizeCallback;
            DISPATCH_RECEIVER_HANDLE m_fileWriteDispatchReceiver;
            DISPATCH_RECEIVER_HANDLE m_fileReadDispatchReceiver;
            DISPATCH_RECEIVER_HANDLE m_fileSizeDispatchReceiver;

            bool isPlayerLoggedIn(const std::string& methodName) const;
            unsigned int getSlotSyncStatusInternal(CachedSlot& slot);
            unsigned int validateSlotStatusForDownload(CachedSlot& slot, bool overrideSync) const;
            unsigned int getPresignedS3UrlForSlot(const char* slotName, unsigned int urlTtl, std::string& returnedS3Url) const;
            unsigned int downloadSlotFromS3(const std::string& presignedSlotDownloadUrl, std::shared_ptr<Aws::Http::HttpResponse>& returnedResponse) const;
            unsigned int addSlot(const std::string& slotName);

            /**
             * @brief Loads an array of slot information files to the local slot cache.
             *
             * @param localSlotInformationFilePaths Array of filepaths to slot information files.
             * @param arraySize Size of localSlotInformation array.
            */
            void loadSlotInformation(const char* const* localSlotInformationFilePaths, unsigned int arraySize);

            /**
             * @brief Utility that saves the information about a slot to a local location.
             *
             * @param slot object containing the slot's information to save locally.
             * @param filePath a null terminated array of characters containing the absolute or relative path and filename where the slot data will be saved.
             * For example: "foo.json", "..\\foo.json", or "C:\\Program Files\\foo.json".
             * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
             */
            unsigned int saveSlotInformation(const Slot& slot, const char* filePath);

            /**
             * @brief Called by the game to download a slots data from the cloud, or for resolving a SHOULD_DOWNLOAD_CLOUD sync status. Slot status should
             * already be updated before calling this method.
             *
             * @param model a struct containing slot information and the data buffer to download the save information into.
             * @param slot object containing the slot's local information
             * @param outActualSlotSize actual size of the slot downloaded
             * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
            */
            unsigned int downloadCloudSlot(GameSavingModel& model, CachedSlot& slot, unsigned int& outActualSlotSize);

            /**
             * @brief Called by the game when updating a current slot, creating a new slot, or resolving a SHOULD_UPLOAD_LOCAL sync status.
             *
             * @param model a struct containing slot information and the data buffer with the save information to upload.
             * @param slot object containing the slot's local information
             * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
            */
            unsigned int uploadLocalSlot(GameSavingModel& model, CachedSlot& slot);

            /**
             * @brief Utility that updates the slot's local information, then saves it to a file, and then gets the updated sync status for the slot.
             *
             * @param slot A reference to the local cached slot.
             * @param model Information about the new save file. The local slot's information will be copied from this model.
             * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
            */
            unsigned int updateLocalSlotStatus(CachedSlot& slot, const GameSavingModel& model);

            unsigned int invokeCallback(DISPATCH_RECEIVER_HANDLE receiver, GameSavingResponseCallback resultCb, unsigned int callStatus) const;
            unsigned int invokeCallback(DISPATCH_RECEIVER_HANDLE receiver, GameSavingResponseCallback resultCb, std::vector<Slot>& singlePageOfSlots) const;
            unsigned int invokeCallback(DISPATCH_RECEIVER_HANDLE receiver, GameSavingResponseCallback resultCb, bool waitForAllPages, std::unordered_set<std::string>& slotsFromCloud) const;
            unsigned int invokeCallback(DISPATCH_RECEIVER_HANDLE receiver, GameSavingResponseCallback resultCb, std::vector<Slot>& returnedSlotList, bool isFinalCall, unsigned int callStatus) const;
            unsigned int invokeCallback(DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, unsigned int callStatus) const;
            unsigned int invokeCallback(DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, unsigned int callStatus, const Slot& actedOnSlot) const;
            unsigned int invokeCallback(DISPATCH_RECEIVER_HANDLE receiver, GameSavingDataResponseCallback resultCb, unsigned int callStatus) const;
            unsigned int invokeCallback(DISPATCH_RECEIVER_HANDLE receiver, GameSavingDataResponseCallback resultCb, unsigned int callStatus, const Slot& slot, const uint8_t* data, unsigned int dataSize) const;

            static bool isValidCallback(DISPATCH_RECEIVER_HANDLE receiver, void* resultCb);
            static std::string getSha256(std::iostream& buffer);
            static void updateSlotFromJson(const JsonView& jsonBody, CachedSlot& returnedSlot);
            static void updateSlotSyncStatus(CachedSlot& returnedSlot);
            static void markSlotAsSyncedWithLocal(CachedSlot& returnedSlot);
            static void markSlotAsSyncedWithCloud(CachedSlot& returnedSlot);

            // illegal operators
            GameSaving(const GameSaving&) = delete;
            GameSaving& operator = (const GameSaving&) = delete;
        
        public:
            GameSaving(Authentication::GameKitSessionManager* sessionManager, FuncLogCallback logCb, const char* const* localSlotInformationFilePaths,
                unsigned int arraySize, FileActions fileActions);
            ~GameSaving() override;

            void AddLocalSlots(const char* const* localSlotInformationFilePaths, unsigned int arraySize) override;
            void SetFileActions(FileActions fileActions) override;
            unsigned int GetAllSlotSyncStatuses(DISPATCH_RECEIVER_HANDLE receiver, GameSavingResponseCallback resultCb, bool waitForAllPages, unsigned int pageSize) override;
            unsigned int GetSlotSyncStatus(DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, const char* slotName) override;
            unsigned int DeleteSlot(DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, const char* slotName) override;
            unsigned int SaveSlot(DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, GameSavingModel model) override;
            unsigned int LoadSlot(DISPATCH_RECEIVER_HANDLE receiver, GameSavingDataResponseCallback resultCb, GameSavingModel model) override;

            /**
             * @brief Getter that returns the cached hash of synced slots. Should be used for testing only.
             *
             * @return Hash of slot objects hashed by their slot name.
            */
            const std::unordered_map<std::string, CachedSlot>& GetSyncedSlots() const
            {
                return m_syncedSlots;
            }

            /**
             * @brief Clears the synced slot cache. Should be used for testing only.
            */
            void ClearSyncedSlots()
            {
                m_syncedSlots.clear();
            }

            /**
             * @brief Sets the Http client to use for this feature. Should be used for testing only.
             *
             * @param httpClient Shared pointer to an http client for this feature to use.
            */
            void SetHttpClient(std::shared_ptr<Aws::Http::HttpClient> httpClient)
            {
                m_httpClient = httpClient;
            }

            /**
             * @brief Sets the CurrentTimeProvider to use for this feature. Should be used for testing only.
             *
             * @param currentTimeProvider Shared pointer to an ICurrentTimeProvider for this feature to use.
            */
            void SetCurrentTimeProvider(std::shared_ptr<Utils::ICurrentTimeProvider> currentTimeProvider)
            {
                m_currentTimeProvider = currentTimeProvider;
            }

            /**
             * @brief Adds the given slot to the local slots. Should be used for testing only.
             *
             * @param slot The slot to be added to local slots.
            */
            void AddLocalSlot(const Slot& slot)
            {
                m_syncedSlots[slot.slotName] = slot;
            }
        };
    }
}
#pragma pop_macro("PAGE_SIZE")