// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../core/test_common.h"
#include "../core/test_stack.h"
#include "../core/test_log.h"
#include "../core/mocks/fake_http_client.h"
#include "aws/gamekit/core/utils/file_utils.h"
#include "aws/gamekit/game-saving/exports.h"
#include "aws/gamekit/game-saving/gamekit_game_saving.h"

// Normally including boost::filesystem in a header is not permitted,
// but this is a test-only header that is never included externally.
#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>

namespace GameKit
{
    namespace Tests
    {
        namespace GameSavingExports
        {
            struct Dispatcher
            {
                std::vector<GameSaving::CachedSlot> syncedSlots;
                std::vector<unsigned int> slotCounts;

                unsigned int slotCount;
                GameSaving::CachedSlot slot;

                const uint8_t* data = nullptr;
                unsigned int dataSize = 0;

                bool complete = false;

                unsigned int callCount = 0;
                unsigned int callStatus = -1;
                std::vector<unsigned int> callStatuses;

                void CallbackHandler(const Slot* syncedSlots, unsigned int slotCount, bool complete, unsigned int callStatus)
                {
                    this->slotCounts.push_back(slotCount);

                    for (unsigned int i = 0; i < slotCount; ++i)
                    {
                        this->syncedSlots.push_back(syncedSlots[i]);
                    }

                    ++callCount;

                    this->complete = complete;
                    this->callStatus = callStatus;
                    this->callStatuses.push_back(callStatus);
                }

                void CallbackHandler(const Slot* syncedSlots, unsigned int slotCount, const Slot* slot, unsigned int callStatus)
                {
                    this->syncedSlots.clear();
                    this->slotCount = slotCount;
                    this->slot = *slot;

                    for (unsigned int i = 0; i < slotCount ; ++i)
                    {
                        this->syncedSlots.push_back(syncedSlots[i]);
                    }

                    ++callCount;
                    this->callStatus = callStatus;
                }

                void CallbackHandler(const Slot* syncedSlots, unsigned int slotCount, const Slot* slot, const uint8_t* data, unsigned int dataSize, unsigned int callStatus)
                {
                    this->syncedSlots.clear();
                    this->slotCount = slotCount;
                    this->slot = *slot;

                    for (unsigned int i = 0; i < slotCount; ++i)
                    {
                        this->syncedSlots.push_back(syncedSlots[i]);
                    }

                    this->data = data;
                    this->dataSize = dataSize;

                    ++callCount;
                    this->callStatus = callStatus;
                }
            };

            class GameKitGameSavingExportsTestFixture : public ::testing::Test
            {
            public:
                GameKitGameSavingExportsTestFixture() {};
                ~GameKitGameSavingExportsTestFixture() override {};

                void SetUp() override;

                void TearDown() override;

            protected:
                typedef TestLog<GameKitGameSavingExportsTestFixture> TestLogger;

                TestStackInitializer testStack;

                GameSavingResponseCallback slotCallback = [](DISPATCH_RECEIVER_HANDLE dispatchReceiver, const Slot* syncedSlots, unsigned int slotCount, bool complete, unsigned int callStatus)
                {
                    static_cast<Dispatcher*>(dispatchReceiver)->CallbackHandler(syncedSlots, slotCount, complete, callStatus);
                };

                GameSavingSlotActionResponseCallback slotActionCallback = [](DISPATCH_RECEIVER_HANDLE dispatchReceiver, const Slot* syncedSlots, unsigned int slotCount, const Slot* slot, unsigned int callStatus)
                {
                    static_cast<Dispatcher*>(dispatchReceiver)->CallbackHandler(syncedSlots, slotCount, slot, callStatus);
                };

                GameSavingDataResponseCallback slotDataResponseCallback = [](DISPATCH_RECEIVER_HANDLE dispatchReceiver, const Slot* syncedSlots, unsigned int slotCount, const Slot* slot, const uint8_t* data, unsigned int dataSize, unsigned int callStatus)
                {
                    static_cast<Dispatcher*>(dispatchReceiver)->CallbackHandler(syncedSlots, slotCount, slot, data, dataSize, callStatus);
                };

                FileWriteCallback writeCallback = [](DISPATCH_RECEIVER_HANDLE, const char* filePath, const uint8_t* data, const unsigned int size)
                {
                    std::string strData(reinterpret_cast<const char*>(data), size);
                    unsigned int status = GameKit::Utils::FileUtils::WriteStringToFile(strData, filePath, TestLogger::Log, "GameSaving::SaveMetadata() ");
                    return status == GameKit::GAMEKIT_SUCCESS;
                };

                FileReadCallback readCallback = [](DISPATCH_RECEIVER_HANDLE, const char* filePath, uint8_t* data, unsigned int size)
                {
                    std::string loadedString;
                    const auto readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString, TestLogger::Log, "GameSaving::LoadSlotInformation() ");
                    std::vector<uint8_t> dataVector(loadedString.begin(), loadedString.end());

                    memcpy(data, dataVector.data(), size);
                    return readStatus == GameKit::GAMEKIT_SUCCESS;
                };

                FileGetSizeCallback fileSizeCallback = [](DISPATCH_RECEIVER_HANDLE, const char* filePath)
                {
                    if (!boost::filesystem::exists(filePath))
                    {
                        return (unsigned int) 0;
                    }
                    return (unsigned int) boost::filesystem::file_size(filePath);
                };

                Authentication::GameKitSessionManager* sessionManager;
                std::shared_ptr<MockHttpClient> mockHttpClient;
                Aws::Utils::DateTime local;
                Aws::Utils::DateTime cloud;
                Aws::Utils::DateTime last;

                void* CreateGameSavingInstance(const Slot* localSyncedSlots, unsigned int slotCount=0, const char* const* slotInformationPaths=nullptr, unsigned int slotInformationArraySize=0);
                void SetMocks(void* instance);
                bool HasSlot(const std::vector<GameSaving::CachedSlot>& slots, const char* slotName);
                GameSaving::CachedSlot GetSlot(const std::vector<GameSaving::CachedSlot>& slots, const char* slotName);
                void AssertCallSucceeded(unsigned int actualStatusCode, const Dispatcher& dispatcher, const Slot& expectedDispatcherSlot, unsigned int expectedDispatcherSlotCount = 1) const;
                void AssertCallFailed(unsigned int expectedStatusCode, unsigned int actualStatusCode, const Dispatcher& dispatcher, unsigned int expectedDispatcherSlotCount = 1) const;
                void AssertCallResult(unsigned int expectedStatusCode, unsigned int actualStatusCode, const Dispatcher& dispatcher, const Slot& expectedDispatcherSlot, unsigned int expectedDispatcherSlotCount = 1) const;
                void AssertEqual(const Slot& expectedSlot, const Slot& actualSlot) const;
                void AssertSlotInfoEqual(const Slot& expectedSlot, const char* slotInfoFilePath) const;
                void AssertIsEmpty(const Slot& actualSlot) const;
            };
        }
    }
}
