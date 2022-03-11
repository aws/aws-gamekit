// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// Gtest
#include <gmock/gmock.h>
#include <gtest/gtest.h>

// GameKit
#include <aws/gamekit/authentication/exports.h>
#include <aws/gamekit/core/internal/platform_string.h>

#include "gamekit_game_saving_exports_tests.h"
#include "../core/mocks/mock_time_provider.h"


static const char* TEST_SLOT_NAME = "testSlot";
static const char* TEST_SLOT_NAME_2 = "testSlot2";
static const char* TEST_SLOT_NAME_3 = "testSlot3";
static const char* TEST_MALFORMED_SLOT_NAME = "<>^thi$_/sLot\\name is-#malf0rme:D";

static const int64_t TEST_MAX_METADATA_BYTES = 1883;

static const char* TEST_METADATA_LOCAL = "{'description':'level 1 complete','percentcomplete':0}";
static const char* TEST_METADATA_CLOUD = "{'description':'level 3 complete','percentcomplete':35}";

#ifdef _WIN32
static const char* TEST_FAKE_PATH = ".\\fakePath\\fakePath2\\FakeFile.txt";
static const char* TEST_EXPECTED_SAVED_SLOT_INFORMATION_FILEPATH = "..\\core\\test_data\\testFiles\\gameSavingTests\\ExpectedSavedSlotInformation.json";
static const char* TEST_INVALID_SAVED_SLOT_INFORMATION_FILEPATH = "..\\core\\test_data\\testFiles\\gameSavingTests\\InvalidSavedSlotInformation.json";
static const char* TEST_NULL_SAVED_SLOT_INFORMATION_FILEPATH = "..\\core\\test_data\\testFiles\\gameSavingTests\\NullSavedSlotInformation.json";
static const char* TEST_TEMP_FILEPATH = "..\\core\\test_data\\testFiles\\gameSavingTests\\TempFile";
#else
static const char* TEST_FAKE_PATH = "./fakePath/fakePath2/FakeFile.txt";
static const char* TEST_EXPECTED_SAVED_SLOT_INFORMATION_FILEPATH = "../core/test_data/testFiles/gameSavingTests/ExpectedSavedSlotInformation.json";
static const char* TEST_INVALID_SAVED_SLOT_INFORMATION_FILEPATH = "../core/test_data/testFiles/gameSavingTests/InvalidSavedSlotInformation.json";
static const char* TEST_NULL_SAVED_SLOT_INFORMATION_FILEPATH = "../core/test_data/testFiles/gameSavingTests/NullSavedSlotInformation.json";
static const char* TEST_TEMP_FILEPATH = "../core/test_data/testFiles/gameSavingTests/TempFile";
#endif

static const std::string APRIL_28 = "2021-04-28T16:18:23Z";
static const std::string OLD_DATE = "2000-01-01T00:00:00Z";
static const std::string APRIL_28_EPOCH = "1619626703000";
static const std::string APRIL_29_EPOCH = "1619713103000";
static const std::string OLD_DATE_EPOCH = "946684800000";

static const std::string TEST_LAST_MODIFIED_LOCAL = APRIL_28;
static const std::string TEST_LAST_MODIFIED_CLOUD = APRIL_28;
static const std::string TEST_LAST_SYNC = APRIL_28;
static const std::string TEST_LAST_SYNC_OLD_CLOUD_TIME = OLD_DATE;

static const int64_t TEST_SIZE_LOCAL = 42;
static const int64_t TEST_SIZE_CLOUD = 73586489;

static const std::string TEST_RESPONSE_METADATA_ENCODED = "eydkZXNjcmlwdGlvbic6J2xldmVsIDMgY29tcGxldGUnLCdwZXJjZW50Y29tcGxldGUnOjM1fQ==";
static const std::string TEST_RESPONSE_METADATA_2_ENCODED = "eydkZXNjcmlwdGlvbic6J2xldmVsIDQgY29tcGxldGUnLCdwZXJjZW50Y29tcGxldGUnOjUwfQ==";
static const std::string TEST_RESPONSE_METADATA_DECODED = "{'description':'level 3 complete','percentcomplete':35}";
static const std::string TEST_RESPONSE_METATADA_2_DECODED = "{'description':'level 4 complete','percentcomplete':50}";
static const std::string TEST_RESPONSE = "{\"meta\":{\"code\":\"200\",\"message\":\"OK\"},\"data\":{\"metadata\":\"" + TEST_RESPONSE_METADATA_ENCODED + "\",\"size\":\"73586489\",\"slot_name\":\"testSlot\",\"player_id\":\"testPlayer\",\"last_modified\":" + APRIL_28_EPOCH + "}}";
static const std::string TEST_RESPONSE_NO_ENTRY = "{\"meta\":{\"code\":\"200\",\"message\":\"OK\"},\"data\":{}}";
static const std::string TEST_RESPONSE_OLD_CLOUD_TIME = "{\"meta\":{\"code\":\"200\",\"message\":\"OK\"},\"data\":{\"metadata\":\"" + TEST_RESPONSE_METADATA_ENCODED + "\",\"size\":\"73586489\",\"slot_name\":\"testSlot\",\"player_id\":\"testPlayer\",\"last_modified\":" + OLD_DATE_EPOCH + "}}";
static const std::string TEST_RESPONSE_INVALID_JSON = "{ not valid json }";
static const std::string TEST_RESPONSE_MAX_SLOTS_EXCEEDED = "{\"meta\":{\"code\":\"400\",\"message\":\"Max Cloud Save Slots Exceeded\"},\"data\":{}}";
static const std::string TEST_RESPONSE_OTHER_BAD_REQUEST = "{\"meta\":{\"code\":\"400\",\"message\":\"Malformed Hash Size Mismatch\"},\"data\":{}}";
static const std::string TEST_RESPONSE_MULTIPLE_ENTRIES = "{\"meta\":{},\"data\":{\"slots_metadata\":[{\"metadata\":\"" + TEST_RESPONSE_METADATA_ENCODED + "\",\"size\":\"73586489\",\"slot_name\":\"testSlot\",\"player_id\":\"testPlayer\",\"last_modified\":" + APRIL_28_EPOCH + "}," \
"{\"metadata\":\"{'description':'level 4 complete','percentcomplete':50}\",\"size\":\"83986489\",\"slot_name\":\"testSlot2\",\"player_id\":\"testPlayer\",\"last_modified\":" + APRIL_29_EPOCH + "}]}}";
static const std::string TEST_RESPONSE_PAGE_1 = "{\"meta\":{},\"data\":{\"slots_metadata\":[{\"metadata\":\"" + TEST_RESPONSE_METADATA_ENCODED + "\",\"size\":\"73586489\",\"slot_name\":\"testSlot\",\"player_id\":\"testPlayer\",\"last_modified\":" + APRIL_28_EPOCH + "}]},\"paging\":{\"next_start_key\":{\"slot_name\":\"testSlot\"},\"paging_token\":\"foo\"}}";
static const std::string TEST_RESPONSE_PAGE_2 = "{\"meta\":{},\"data\":{\"slots_metadata\":[{\"metadata\":\"" + TEST_RESPONSE_METADATA_2_ENCODED + "\",\"size\":\"83986489\",\"slot_name\":\"testSlot2\",\"player_id\":\"testPlayer\",\"last_modified\":" + APRIL_29_EPOCH + "}]},\"paging\":{\"next_start_key\":{\"slot_name\":\"testSlot2\"},\"paging_token\":\"foo\"}}";
static const std::string TEST_RESPONSE_PAGE_LAST = "{\"meta\":{},\"data\":{\"slots_metadata\":[]}}";
static const std::string TEST_RESPONSE_PUT_URL = "{\"meta\":{\"code\":\"200\",\"message\":\"OK\"},\"data\":{\"url\":\"https://gamekit-dev-number-testGame-player-gamesaves.s3.amazonaws.com/testPlayer/testSlot?andSomeOtherStuff\"}}";
static const std::string TEST_RESPONSE_PUT_URL_EMPTY = "{\"meta\":{\"code\":\"200\",\"message\":\"OK\"},\"data\":{\"url\":\"\"}}";
static const std::string TEST_GENERATE_S3_PRESIGNED_URL_RESPONSE = "{\"meta\":{\"code\":\"200\",\"message\":\"OK\"},\"data\":{\"url\":\"testUrl\"}}";
static const std::string TEST_GENERATE_MALFORMED_S3_PRESIGNED_URL_RESPONSE = "{\"meta\":{\"code\":\"200\",\"message\":\"OK\"}}";
static const std::string TEST_SLOT_DOWNLOAD_RESPONSE{ '\x41', '\x42', '\x43', '\x44', '\x45', '\x46', '\x47', '\x48' }; // pretend we're a non-string response
static const int64_t TEST_SLOT_DOWNLOAD_RESPONSE_SIZE = 8;
static const Aws::String TEST_SHA_256_METADATA_HEADER = "x-amz-meta-hash";
static const Aws::String TEST_SLOT_DOWNLOAD_SHA_256 = "msIZfZJYJXsa6EY+QhTkzQpXi8FRfyQVkouRvkKD/Eg="; // base64 encoded SHA-256 of the s3 download response above

using namespace GameKit::Tests::GameSavingExports;
using namespace GameKit::GameSaving;
using namespace GameKit::Mocks;

using namespace testing;

void GameKitGameSavingExportsTestFixture::SetUp()
{
    ::testing::internal::CaptureStdout();
    TestLogger::Clear();

    local = ToAwsString(TEST_LAST_MODIFIED_LOCAL);
    cloud = ToAwsString(TEST_LAST_MODIFIED_CLOUD);
    last = ToAwsString(TEST_LAST_SYNC);

    testStack.Initialize();
}

void GameKitGameSavingExportsTestFixture::TearDown()
{
    std::string capturedStdout = ::testing::internal::GetCapturedStdout();

    testStack.Cleanup();

    remove(TEST_FAKE_PATH);
    remove(TEST_TEMP_FILEPATH);
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

void* GameKitGameSavingExportsTestFixture::CreateGameSavingInstance(const Slot* localSyncedSlots = nullptr, unsigned int slotCount, const char* const* slotInformationPaths, unsigned int slotInformationArraySize)
{
    sessionManager = static_cast<Authentication::GameKitSessionManager*>(GameKitSessionManagerInstanceCreate("../core/test_data/sampleplugin/instance/testgame/dev/awsGameKitClientConfig.yml", TestLogger::Log));
    sessionManager->SetToken(TokenType::IdToken, "test_token");

    void* instance;
    FileActions actions
    {
        writeCallback,
        readCallback,
        fileSizeCallback,
        nullptr,
        nullptr,
        nullptr
    };

    if (slotInformationArraySize == 0)
    {
        const char* paths;
        instance = GameKitGameSavingInstanceCreateWithSessionManager(sessionManager, TestLogger::Log, &paths, 0, actions);
    }
    else
    {
        instance = GameKitGameSavingInstanceCreateWithSessionManager(sessionManager, TestLogger::Log, slotInformationPaths, slotInformationArraySize, actions);
    }

    auto gameSaving = static_cast<GameSaving::GameSaving*>(instance);
    for (unsigned int i = 0; i < slotCount; i++)
    {
        gameSaving->AddLocalSlot(localSyncedSlots[i]);
    }
    return instance;
}

void GameKitGameSavingExportsTestFixture::SetMocks(void* instance)
{
    mockHttpClient = std::make_shared<MockHttpClient>();

    auto gameSaving = static_cast<GameSaving::GameSaving*>(instance);
    gameSaving->SetHttpClient(mockHttpClient);
}

bool GameKitGameSavingExportsTestFixture::HasSlot(const std::vector<GameSaving::CachedSlot>& slots, const char* slotName)
{
    for (const Slot& slot : slots)
    {
        if (strcmp(slot.slotName, slotName) == 0)
        {
            return true;
        }
    }

    return false;
}

CachedSlot GameKitGameSavingExportsTestFixture::GetSlot(const std::vector<GameSaving::CachedSlot>& slots, const char* slotName)
{
    for (const Slot& slot : slots)
    {
        if (strcmp(slot.slotName, slotName) == 0)
        {
            return slot;
        }
    }

    throw std::invalid_argument(std::string("No slot found with name: ") + slotName);
}

void GameKitGameSavingExportsTestFixture::AssertCallSucceeded(unsigned actualStatusCode, const Dispatcher& dispatcher, const Slot& expectedDispatcherSlot, unsigned expectedDispatcherSlotCount) const
{
    AssertCallResult(GameKit::GAMEKIT_SUCCESS, actualStatusCode, dispatcher, expectedDispatcherSlot, expectedDispatcherSlotCount);
}

void GameKitGameSavingExportsTestFixture::AssertCallFailed(unsigned int expectedStatusCode, unsigned int actualStatusCode, const Dispatcher& dispatcher, unsigned int expectedDispatcherSlotCount) const
{
    ASSERT_NE(GameKit::GAMEKIT_SUCCESS, expectedStatusCode);
    ASSERT_NE(GameKit::GAMEKIT_SUCCESS, actualStatusCode);

    const Slot emptySlot{};
    const Slot expectedSlot = emptySlot;
    AssertCallResult(expectedStatusCode, actualStatusCode, dispatcher, expectedSlot, expectedDispatcherSlotCount);

    ASSERT_EQ(nullptr, dispatcher.data);
    ASSERT_EQ(0, dispatcher.dataSize);
}

void GameKitGameSavingExportsTestFixture::AssertCallResult(unsigned expectedStatusCode, unsigned actualStatusCode, const Dispatcher& dispatcher, const Slot& expectedDispatcherSlot, unsigned expectedDispatcherSlotCount) const
{
    ASSERT_EQ(expectedStatusCode, actualStatusCode);

    ASSERT_EQ(1, dispatcher.callCount);
    ASSERT_EQ(expectedStatusCode, dispatcher.callStatus);
    ASSERT_EQ(expectedDispatcherSlotCount, dispatcher.slotCount);
    ASSERT_EQ(expectedDispatcherSlotCount, dispatcher.syncedSlots.size());
    AssertEqual(expectedDispatcherSlot, dispatcher.slot);
}

void GameKitGameSavingExportsTestFixture::AssertEqual(const Slot& expectedSlot, const Slot& actualSlot) const
{
    ASSERT_EQ(0, strcmp(expectedSlot.slotName, actualSlot.slotName));
    ASSERT_EQ(0, strcmp(expectedSlot.metadataLocal, actualSlot.metadataLocal));
    ASSERT_EQ(0, strcmp(expectedSlot.metadataCloud, actualSlot.metadataCloud));
    ASSERT_EQ(expectedSlot.sizeLocal, actualSlot.sizeLocal);
    ASSERT_EQ(expectedSlot.sizeCloud, actualSlot.sizeCloud);
    ASSERT_EQ(expectedSlot.lastModifiedLocal, actualSlot.lastModifiedLocal);
    ASSERT_EQ(expectedSlot.lastModifiedCloud, actualSlot.lastModifiedCloud);
    ASSERT_EQ(expectedSlot.lastSync, actualSlot.lastSync);
    ASSERT_EQ(expectedSlot.slotSyncStatus, actualSlot.slotSyncStatus);
}

void GameKitGameSavingExportsTestFixture::AssertSlotInfoEqual(const Slot& expectedSlot, const char* slotInfoFilePath) const
{
    std::string savedSlotInfo;
    GameKit::Utils::FileUtils::ReadFileIntoString(slotInfoFilePath, savedSlotInfo);
    CachedSlot actualSlot;
    actualSlot.FromJson(ToAwsString(savedSlotInfo));

    AssertEqual(expectedSlot, actualSlot);
}

void GameKitGameSavingExportsTestFixture::AssertIsEmpty(const Slot& actualSlot) const
{
    Slot emptySlot{};
    AssertEqual(emptySlot, actualSlot);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingInstanceCreate_WithLocalSlots_Success)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        TEST_METADATA_CLOUD,
        TEST_SIZE_LOCAL,
        TEST_SIZE_CLOUD,
        local.Millis(),
        cloud.Millis(),
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    // act
    const GAMEKIT_GAME_SAVING_INSTANCE_HANDLE gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);

    // assert
    ASSERT_NE(gameSavingInstance, nullptr);
    ASSERT_EQ(static_cast<GameKit::GameSaving::GameSaving*>(gameSavingInstance)->GetSyncedSlots().at(testSlot.slotName).slotName, testSlot.slotName);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingInstanceCreate_WithoutLocalSlots_Success)
{
    // act
    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(nullptr, 0);

    // assert
    ASSERT_NE(gameSavingInstance, nullptr);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingInstanceRelease_Success)
{
    // arrange
    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance();

    // act
    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingGetAllSlotSyncStatuses_success)
{
    // arrange
    std::vector<Slot> testSlots;
    testSlots.push_back({
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    });

    testSlots.push_back({
        TEST_SLOT_NAME_3,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    });

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(testSlots.data(), 2);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_MULTIPLE_ENTRIES);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    unsigned int response = GameKitGetAllSlotSyncStatuses(gameSavingInstance, &dispatcher, slotCallback, true, 0);

    // assert
    ASSERT_EQ(response, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatuses[0], GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(1, dispatcher.callCount);
    ASSERT_EQ(3, dispatcher.slotCounts[0]);
    ASSERT_EQ(3, dispatcher.syncedSlots.size());
    ASSERT_TRUE(HasSlot(dispatcher.syncedSlots, TEST_SLOT_NAME));
    ASSERT_TRUE(HasSlot(dispatcher.syncedSlots, TEST_SLOT_NAME_2));
    ASSERT_TRUE(HasSlot(dispatcher.syncedSlots, TEST_SLOT_NAME_3));
    ASSERT_TRUE(dispatcher.complete);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingGetAllSlotSyncStatuses_multi_page_single_call)
{
    // arrange
    std::vector<Slot> testSlots;
    testSlots.push_back({
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
        });

    testSlots.push_back({
        TEST_SLOT_NAME_3,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
        });

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(testSlots.data(), 2);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_PAGE_1);

    std::shared_ptr<FakeHttpResponse> testResponse2 = std::make_shared<FakeHttpResponse>();
    testResponse2->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse2->SetResponseBody(TEST_RESPONSE_PAGE_2);

    std::shared_ptr<FakeHttpResponse> testResponse3 = std::make_shared<FakeHttpResponse>();
    testResponse3->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse3->SetResponseBody(TEST_RESPONSE_PAGE_LAST);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse)).WillOnce(Return(testResponse2)).WillOnce(Return(testResponse3));

    Dispatcher dispatcher;

    // act
    unsigned int response = GameKitGetAllSlotSyncStatuses(gameSavingInstance, &dispatcher, slotCallback, true, 1);

    // assert
    ASSERT_EQ(response, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatuses[0], GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(1, dispatcher.callCount);
    ASSERT_EQ(3, dispatcher.slotCounts[0]);
    ASSERT_EQ(3, dispatcher.syncedSlots.size());
    ASSERT_TRUE(HasSlot(dispatcher.syncedSlots, TEST_SLOT_NAME));
    ASSERT_TRUE(HasSlot(dispatcher.syncedSlots, TEST_SLOT_NAME_2));
    ASSERT_TRUE(HasSlot(dispatcher.syncedSlots, TEST_SLOT_NAME_2));
    ASSERT_TRUE(dispatcher.complete);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingGetAllSlotSyncStatuses_multi_page_multi_call)
{
    // arrange
    std::vector<Slot> testSlots;
    testSlots.push_back({
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
        });

    testSlots.push_back({
        TEST_SLOT_NAME_3,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
        });

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(testSlots.data(), 2);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_PAGE_1);

    std::shared_ptr<FakeHttpResponse> testResponse2 = std::make_shared<FakeHttpResponse>();
    testResponse2->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse2->SetResponseBody(TEST_RESPONSE_PAGE_2);

    std::shared_ptr<FakeHttpResponse> testResponse3 = std::make_shared<FakeHttpResponse>();
    testResponse3->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse3->SetResponseBody(TEST_RESPONSE_PAGE_LAST);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse)).WillOnce(Return(testResponse2)).WillOnce(Return(testResponse3));

    Dispatcher dispatcher;

    // act
    unsigned int response = GameKitGetAllSlotSyncStatuses(gameSavingInstance, &dispatcher, slotCallback, false, 1);

    // assert
    ASSERT_EQ(response, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatuses[0], GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatuses[1], GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatuses[2], GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(3, dispatcher.callCount);
    ASSERT_EQ(1, dispatcher.slotCounts[0]);
    ASSERT_EQ(1, dispatcher.slotCounts[1]);
    ASSERT_EQ(1, dispatcher.slotCounts[2]);
    ASSERT_EQ(3, dispatcher.syncedSlots.size());
    ASSERT_TRUE(HasSlot(dispatcher.syncedSlots, TEST_SLOT_NAME));
    ASSERT_TRUE(HasSlot(dispatcher.syncedSlots, TEST_SLOT_NAME_2));
    ASSERT_TRUE(HasSlot(dispatcher.syncedSlots, TEST_SLOT_NAME_3));
    ASSERT_TRUE(dispatcher.complete);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingGetAllSlotSyncStatuses_missing_token)
{
    // arrange
    const SlotSyncStatus expectedSlotSyncStatus = SlotSyncStatus::SYNCED;

    std::vector<Slot> testSlots;
    const Slot slot1 = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        TEST_METADATA_LOCAL,
        TEST_SIZE_LOCAL,
        TEST_SIZE_LOCAL,
        local.Millis(),
        cloud.Millis(),
        last.Millis(),
        expectedSlotSyncStatus
    };
    testSlots.push_back(slot1);

    const Slot slot2 = {
        TEST_SLOT_NAME_2,
        TEST_METADATA_LOCAL,
        TEST_METADATA_LOCAL,
        TEST_SIZE_LOCAL,
        TEST_SIZE_LOCAL,
        local.Millis(),
        cloud.Millis(),
        last.Millis(),
        expectedSlotSyncStatus
    };
    testSlots.push_back(slot2);

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(testSlots.data(), 2);
    SetMocks(gameSavingInstance);
    sessionManager->DeleteToken(GameKit::TokenType::IdToken);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).Times(0);

    Dispatcher dispatcher;

    // act
    unsigned int response = GameKitGetAllSlotSyncStatuses(gameSavingInstance, &dispatcher, slotCallback, true, 0);

    // assert
    ASSERT_EQ(response, GameKit::GAMEKIT_ERROR_NO_ID_TOKEN);
    ASSERT_EQ(dispatcher.callStatus, GameKit::GAMEKIT_ERROR_NO_ID_TOKEN);
    ASSERT_EQ(dispatcher.callStatuses[0], GameKit::GAMEKIT_ERROR_NO_ID_TOKEN);
    ASSERT_EQ(1, dispatcher.callCount);
    ASSERT_EQ(0, dispatcher.slotCounts[0]);
    ASSERT_EQ(0, dispatcher.syncedSlots.size());
    ASSERT_TRUE(dispatcher.complete);

    // assert the synced slots are not modified
    std::unordered_map<std::string, CachedSlot> syncedSlots = static_cast<GameKit::GameSaving::GameSaving*>(gameSavingInstance)->GetSyncedSlots();
    AssertEqual(slot1, syncedSlots[TEST_SLOT_NAME]);
    AssertEqual(slot2, syncedSlots[TEST_SLOT_NAME_2]);
    ASSERT_EQ(expectedSlotSyncStatus, syncedSlots[TEST_SLOT_NAME].slotSyncStatus);
    ASSERT_EQ(expectedSlotSyncStatus, syncedSlots[TEST_SLOT_NAME_2].slotSyncStatus);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}
TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingGetSlotSyncStatus_success)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitGetSlotSyncStatus(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    // assert
    ASSERT_EQ(response, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(1, dispatcher.callCount);
    ASSERT_EQ(1, dispatcher.slotCount);
    ASSERT_EQ(1, dispatcher.syncedSlots.size());
    ASSERT_EQ(0, strcmp(dispatcher.slot.slotName.c_str(), dispatcher.syncedSlots[0].slotName.c_str()));
    ASSERT_EQ(0, strcmp(TEST_SLOT_NAME, dispatcher.slot.slotName.c_str()));
    ASSERT_EQ(0, strcmp(TEST_METADATA_LOCAL, dispatcher.slot.metadataLocal.c_str()));
    ASSERT_EQ(0, strcmp(TEST_METADATA_CLOUD, dispatcher.slot.metadataCloud.c_str()));
    ASSERT_EQ(TEST_SIZE_LOCAL, dispatcher.slot.sizeLocal);
    ASSERT_EQ(TEST_SIZE_CLOUD, dispatcher.slot.sizeCloud);
    ASSERT_EQ(local.Millis(), dispatcher.slot.lastModifiedLocal.Millis());
    ASSERT_EQ(cloud.Millis(), dispatcher.slot.lastModifiedCloud.Millis());
    ASSERT_EQ(last.Millis(), dispatcher.slot.lastSync.Millis());
    ASSERT_EQ(SlotSyncStatus::SYNCED, dispatcher.slot.slotSyncStatus);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingGetSlotSyncStatus_synced)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitGetSlotSyncStatus(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    // assert
    ASSERT_EQ(local.Millis(), dispatcher.slot.lastModifiedLocal.Millis());
    ASSERT_EQ(cloud.Millis(), dispatcher.slot.lastModifiedCloud.Millis());
    ASSERT_EQ(last.Millis(), dispatcher.slot.lastSync.Millis());
    ASSERT_EQ(SlotSyncStatus::SYNCED, dispatcher.slot.slotSyncStatus);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingGetSlotSyncStatus_should_upload_local)
{
    // arrange
    last = ToAwsString(TEST_LAST_SYNC_OLD_CLOUD_TIME);
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_OLD_CLOUD_TIME);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitGetSlotSyncStatus(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    // assert
    ASSERT_EQ(local, dispatcher.slot.lastModifiedLocal.Millis());
    ASSERT_EQ(last, dispatcher.slot.lastModifiedCloud.Millis());
    ASSERT_EQ(last, dispatcher.slot.lastSync.Millis());
    ASSERT_EQ(SlotSyncStatus::SHOULD_UPLOAD_LOCAL, dispatcher.slot.slotSyncStatus);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingGetSlotSyncStatus_should_download_cloud)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is update from the response
        0, // setting local to 0 to force it to be older then cloud
        0, // cloud time is updated from the response
        0, // last sync must be equal to local in this case, else it will indicate a conflict
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitGetSlotSyncStatus(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    // assert
    ASSERT_EQ(0, dispatcher.slot.lastModifiedLocal.Millis());
    ASSERT_EQ(cloud.Millis(), dispatcher.slot.lastModifiedCloud.Millis());
    ASSERT_EQ(0, dispatcher.slot.lastSync.Millis());
    ASSERT_EQ(SlotSyncStatus::SHOULD_DOWNLOAD_CLOUD, dispatcher.slot.slotSyncStatus);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingGetSlotSyncStatus_in_conflict)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is update from the response
        local.Millis(), // setting local to 0 to force it to be older then cloud
        0, // cloud time is updated from the response
        0, // if last sync is not equal to either local or cloud, this indicates a possible conflict
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitGetSlotSyncStatus(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    // assert
    ASSERT_EQ(local.Millis(), dispatcher.slot.lastModifiedLocal.Millis());
    ASSERT_EQ(cloud.Millis(), dispatcher.slot.lastModifiedCloud.Millis());
    ASSERT_EQ(0, dispatcher.slot.lastSync.Millis());
    ASSERT_EQ(SlotSyncStatus::IN_CONFLICT, dispatcher.slot.slotSyncStatus);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingGetSlotSyncStatus_missing_token)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);
    sessionManager->DeleteToken(GameKit::TokenType::IdToken);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).Times(0);

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitGetSlotSyncStatus(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_NO_ID_TOKEN, response, dispatcher);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingGetSlotSyncStatus_http_request_failed)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(500));

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitGetSlotSyncStatus(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_HTTP_REQUEST_FAILED, response, dispatcher);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingGetSlotSyncStatus_response_body_not_in_json_format)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_INVALID_JSON);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitGetSlotSyncStatus(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_PARSE_JSON_FAILED, response, dispatcher);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingGetSlotSyncStatus_missing_local_slot)
{
    // arrange
    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance();
    SetMocks(gameSavingInstance);

    static_cast<GameKit::GameSaving::GameSaving*>(gameSavingInstance)->ClearSyncedSlots();
    const unsigned int expectedSlotCount = 0;

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).Times(0);

    Dispatcher dispatcher;

    // act
    unsigned int response = GameKitGetSlotSyncStatus(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    GameKit::GameSaving::GameSaving* pGS = (GameKit::GameSaving::GameSaving*)gameSavingInstance;

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_GAME_SAVING_SLOT_NOT_FOUND, response, dispatcher, expectedSlotCount);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingGetSlotSyncStatus_malformed_slot_name)
{
    // arrange
    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance();
    SetMocks(gameSavingInstance);

    static_cast<GameKit::GameSaving::GameSaving*>(gameSavingInstance)->ClearSyncedSlots();
    const unsigned int expectedSlotCount = 0;

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).Times(0);

    Dispatcher dispatcher;

    // act
    unsigned int response = GameKitGetSlotSyncStatus(gameSavingInstance, &dispatcher, slotActionCallback, TEST_MALFORMED_SLOT_NAME);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_GAME_SAVING_MALFORMED_SLOT_NAME, response, dispatcher, expectedSlotCount);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingGetSlotSyncStatus_entry_not_found)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        0, // for a new entry that is not in the cloud, the last sync will not be set, ie last == cloud
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_NO_ENTRY);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitGetSlotSyncStatus(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    // assert
    ASSERT_EQ(response, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(local.Millis(), dispatcher.slot.lastModifiedLocal.Millis());
    ASSERT_EQ(0, dispatcher.slot.lastModifiedCloud.Millis());
    ASSERT_EQ(0, dispatcher.slot.lastSync.Millis());
    ASSERT_EQ(SlotSyncStatus::SHOULD_UPLOAD_LOCAL, dispatcher.slot.slotSyncStatus);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingSaveSlot_success)
{
    // arrange
    last = ToAwsString(TEST_LAST_SYNC_OLD_CLOUD_TIME);
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::string testBuffer = "I'm a test buffer";
    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        (uint8_t*)testBuffer.data(),
        (unsigned int)testBuffer.size(),
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_OLD_CLOUD_TIME);

    std::shared_ptr<FakeHttpResponse> testResponse2 = std::make_shared<FakeHttpResponse>();
    testResponse2->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse2->SetResponseBody(TEST_RESPONSE_PUT_URL);

    std::shared_ptr<FakeHttpResponse> testResponse3 = std::make_shared<FakeHttpResponse>();
    testResponse3->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(testResponse))
        .WillOnce(Return(testResponse2))
        .WillOnce(Return(testResponse3));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitSaveSlot(gameSavingInstance, &dispatcher, slotActionCallback, testModel);

    // assert
    ASSERT_EQ(response, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(1, dispatcher.slotCount);
    ASSERT_EQ(1, dispatcher.syncedSlots.size());
    ASSERT_EQ(dispatcher.slot.metadataCloud, dispatcher.slot.metadataLocal);
    ASSERT_EQ(dispatcher.slot.sizeCloud, dispatcher.slot.sizeLocal);
    ASSERT_EQ(dispatcher.slot.lastModifiedCloud, dispatcher.slot.lastModifiedLocal);
    ASSERT_EQ(dispatcher.slot.lastSync, dispatcher.slot.lastModifiedLocal);
    ASSERT_EQ(SlotSyncStatus::SYNCED, dispatcher.slot.slotSyncStatus);
    AssertSlotInfoEqual(dispatcher.slot, TEST_TEMP_FILEPATH);

    // teardown
    remove(TEST_TEMP_FILEPATH);
    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingSaveSlot_s3_upload_failed)
{
    // arrange
    last = ToAwsString(TEST_LAST_SYNC_OLD_CLOUD_TIME);
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::string testBuffer = "I'm a test buffer";

    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        (uint8_t*)testBuffer.data(),
        (unsigned int)testBuffer.size(),
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_OLD_CLOUD_TIME);

    std::shared_ptr<FakeHttpResponse> testResponse2 = std::make_shared<FakeHttpResponse>();
    testResponse2->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse2->SetResponseBody(TEST_RESPONSE_PUT_URL);

    std::shared_ptr<FakeHttpResponse> testResponse3 = std::make_shared<FakeHttpResponse>();
    testResponse3->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(403));

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse)).WillOnce(Return(testResponse2)).WillOnce(Return(testResponse3));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitSaveSlot(gameSavingInstance, &dispatcher, slotActionCallback, testModel);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_HTTP_REQUEST_FAILED, response, dispatcher);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingSaveSlot_missing_token)
{
    // arrange
    const unsigned int expectedSlotCount = 0;
    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(nullptr, expectedSlotCount);
    SetMocks(gameSavingInstance);
    sessionManager->DeleteToken(GameKit::TokenType::IdToken);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).Times(0);

    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        nullptr,
        0,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitSaveSlot(gameSavingInstance, &dispatcher, slotActionCallback, testModel);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_NO_ID_TOKEN, response, dispatcher, expectedSlotCount);
    ASSERT_FALSE(boost::filesystem::exists(TEST_TEMP_FILEPATH));

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingSaveSlot_slot_not_found)
{
    // arrange
    const unsigned int expectedSlotCount = 0;
    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(nullptr, expectedSlotCount);
    SetMocks(gameSavingInstance);

    std::string testBuffer = "I'm a test buffer";
    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        (uint8_t*)testBuffer.data(),
        (unsigned int)testBuffer.size(),
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_NO_ENTRY);

    std::shared_ptr<FakeHttpResponse> testResponse2 = std::make_shared<FakeHttpResponse>();
    testResponse2->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse2->SetResponseBody(TEST_RESPONSE_PUT_URL);

    std::shared_ptr<FakeHttpResponse> testResponse3 = std::make_shared<FakeHttpResponse>();
    testResponse3->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(testResponse))
        .WillOnce(Return(testResponse2))
        .WillOnce(Return(testResponse3));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitSaveSlot(gameSavingInstance, &dispatcher, slotActionCallback, testModel);

    // assert
    ASSERT_EQ(response, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(1, dispatcher.slotCount);
    ASSERT_EQ(1, dispatcher.syncedSlots.size());
    ASSERT_EQ(dispatcher.slot.metadataCloud, dispatcher.slot.metadataLocal);
    ASSERT_EQ(dispatcher.slot.sizeCloud, dispatcher.slot.sizeLocal);
    ASSERT_EQ(dispatcher.slot.lastModifiedCloud, dispatcher.slot.lastModifiedLocal);
    ASSERT_EQ(dispatcher.slot.lastSync, dispatcher.slot.lastModifiedLocal);
    ASSERT_EQ(SlotSyncStatus::SYNCED, dispatcher.slot.slotSyncStatus);
    AssertSlotInfoEqual(dispatcher.slot, TEST_TEMP_FILEPATH);

    // teardown
    remove(TEST_TEMP_FILEPATH);
    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingSaveSlot_malformed_slot_name)
{
    // arrange
    const unsigned int expectedSlotCount = 0;
    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(nullptr, expectedSlotCount);
    SetMocks(gameSavingInstance);

    const GameSavingModel testModel = {
        TEST_MALFORMED_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        nullptr,
        0,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitSaveSlot(gameSavingInstance, &dispatcher, slotActionCallback, testModel);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_GAME_SAVING_MALFORMED_SLOT_NAME, response, dispatcher, expectedSlotCount);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingSaveSlot_metadata_too_long)
{
    // arrange
    last = ToAwsString(TEST_LAST_SYNC_OLD_CLOUD_TIME);
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::vector<char> metadata(TEST_MAX_METADATA_BYTES + 1, 'a');
    metadata.push_back('\n');
    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        metadata.data(),
        0, // epoch time
        false, // override sync
        nullptr,
        0,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_OLD_CLOUD_TIME);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitSaveSlot(gameSavingInstance, &dispatcher, slotActionCallback, testModel);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_GAME_SAVING_EXCEEDED_MAX_SIZE, response, dispatcher);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingSaveSlot_max_slots_exceeded)
{
    // arrange
    last = ToAwsString(TEST_LAST_SYNC_OLD_CLOUD_TIME);
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::string testBuffer = "I'm a test buffer";
    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        (uint8_t*)testBuffer.data(),
        (unsigned int)testBuffer.size(),
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(400));
    testResponse->SetResponseBody(TEST_RESPONSE_MAX_SLOTS_EXCEEDED);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitSaveSlot(gameSavingInstance, &dispatcher, slotActionCallback, testModel);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_GAME_SAVING_MAX_CLOUD_SLOTS_EXCEEDED, response, dispatcher);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingSaveSlot_other_bad_request)
{
    // arrange
    last = ToAwsString(TEST_LAST_SYNC_OLD_CLOUD_TIME);
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::string testBuffer = "I'm a test buffer";
    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        (uint8_t*)testBuffer.data(),
        (unsigned int)testBuffer.size(),
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(400));
    testResponse->SetResponseBody(TEST_RESPONSE_OTHER_BAD_REQUEST);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitSaveSlot(gameSavingInstance, &dispatcher, slotActionCallback, testModel);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_HTTP_REQUEST_FAILED, response, dispatcher);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingSaveSlot_generatePresignedPutURL_lambda_call_failed)
{
    // arrange
    last = ToAwsString(TEST_LAST_SYNC_OLD_CLOUD_TIME);
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::string testBuffer = "I'm a test buffer";

    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        (uint8_t*)testBuffer.data(),
        (unsigned int)testBuffer.size(),
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_OLD_CLOUD_TIME);

    std::shared_ptr<FakeHttpResponse> testResponse2 = std::make_shared<FakeHttpResponse>();
    testResponse2->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse2->SetResponseBody(TEST_RESPONSE_PUT_URL_EMPTY);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse)).WillOnce(Return(testResponse2));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitSaveSlot(gameSavingInstance, &dispatcher, slotActionCallback, testModel);

    // assert
    ASSERT_NE(response, GameKit::GAMEKIT_SUCCESS);
    ASSERT_NE(dispatcher.callStatus, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(1, dispatcher.callCount);
    ASSERT_EQ(1, dispatcher.slotCount);
    AssertIsEmpty(dispatcher.slot);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingSaveSlot_url_not_correct)
{
    // arrange
    last = ToAwsString(TEST_LAST_SYNC_OLD_CLOUD_TIME);
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::string testBuffer = "I'm a test buffer";

    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        (uint8_t*)testBuffer.data(),
        (unsigned int)testBuffer.size(),
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_OLD_CLOUD_TIME);

    std::shared_ptr<FakeHttpResponse> testResponse2 = std::make_shared<FakeHttpResponse>();
    testResponse2->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(404));

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse)).WillOnce(Return(testResponse2));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitSaveSlot(gameSavingInstance, &dispatcher, slotActionCallback, testModel);

    // assert
    ASSERT_NE(response, GameKit::GAMEKIT_SUCCESS);
    ASSERT_NE(dispatcher.callStatus, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(1, dispatcher.callCount);
    ASSERT_EQ(1, dispatcher.slotCount);
    AssertIsEmpty(dispatcher.slot);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingSaveSlot_in_conflict)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        nullptr,
        0,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_OLD_CLOUD_TIME);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitSaveSlot(gameSavingInstance, &dispatcher, slotActionCallback, testModel);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_GAME_SAVING_SYNC_CONFLICT, response, dispatcher);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingSaveSlot_slot_already_synced)
{
    const int64_t lastModified = Aws::Utils::DateTime(ToAwsString(APRIL_28), Aws::Utils::DateFormat::ISO_8601).Millis();
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        lastModified,
        0, // cloud time is update from the response
        lastModified,
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::string testBuffer = "I'm a test buffer";
    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        lastModified, // epoch time
        false, // override sync
        (uint8_t*)testBuffer.data(),
        (unsigned int)testBuffer.size(),
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE); // has the same last_modified timestamp as the testSlot

    std::shared_ptr<FakeHttpResponse> testResponse2 = std::make_shared<FakeHttpResponse>();
    testResponse2->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse2->SetResponseBody(TEST_RESPONSE_PUT_URL);

    std::shared_ptr<FakeHttpResponse> testResponse3 = std::make_shared<FakeHttpResponse>();
    testResponse3->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(testResponse))
        .WillOnce(Return(testResponse2))
        .WillOnce(Return(testResponse3));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitSaveSlot(gameSavingInstance, &dispatcher, slotActionCallback, testModel);

    // assert
    ASSERT_EQ(response, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(1, dispatcher.slotCount);
    ASSERT_EQ(1, dispatcher.syncedSlots.size());
    ASSERT_EQ(dispatcher.slot.metadataCloud, dispatcher.slot.metadataLocal);
    ASSERT_EQ(dispatcher.slot.sizeCloud, dispatcher.slot.sizeLocal);
    ASSERT_EQ(dispatcher.slot.lastModifiedCloud, dispatcher.slot.lastModifiedLocal);
    ASSERT_EQ(dispatcher.slot.lastSync, dispatcher.slot.lastModifiedLocal);
    ASSERT_EQ(SlotSyncStatus::SYNCED, dispatcher.slot.slotSyncStatus);
    AssertSlotInfoEqual(dispatcher.slot, TEST_TEMP_FILEPATH);

    // teardown
    remove(TEST_TEMP_FILEPATH);
    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingSaveSlot_cloud_is_newer)
{
    // arrange
    // cloud > local == last for SlotSyncStatus::SHOULD_DOWNLOAD_CLOUD
    last = ToAwsString(TEST_LAST_SYNC_OLD_CLOUD_TIME);
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        last.Millis(), // local last modified
        0, // cloud time is update from the response
        last.Millis(), // local last sync
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        last.Millis(), // local last modified
        false, // override sync
        nullptr,
        0,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitSaveSlot(gameSavingInstance, &dispatcher, slotActionCallback, testModel);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_GAME_SAVING_CLOUD_SLOT_IS_NEWER, response, dispatcher);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingSaveSlot_override)
{
    // arrange
    // slot in conflict
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::string testBuffer = "I'm a test buffer";
    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        true, // override sync
        (uint8_t*)testBuffer.data(),
        (unsigned int)testBuffer.size(),
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_OLD_CLOUD_TIME);

    std::shared_ptr<FakeHttpResponse> testResponse2 = std::make_shared<FakeHttpResponse>();
    testResponse2->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse2->SetResponseBody(TEST_RESPONSE_PUT_URL);

    std::shared_ptr<FakeHttpResponse> testResponse3 = std::make_shared<FakeHttpResponse>();
    testResponse3->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(testResponse))
        .WillOnce(Return(testResponse2))
        .WillOnce(Return(testResponse3));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitSaveSlot(gameSavingInstance, &dispatcher, slotActionCallback, testModel);

    // assert
    ASSERT_EQ(response, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(1, dispatcher.slotCount);
    ASSERT_EQ(1, dispatcher.syncedSlots.size());
    ASSERT_EQ(dispatcher.slot.metadataCloud, dispatcher.slot.metadataLocal);
    ASSERT_EQ(dispatcher.slot.sizeCloud, dispatcher.slot.sizeLocal);
    ASSERT_EQ(dispatcher.slot.lastModifiedCloud, dispatcher.slot.lastModifiedLocal);
    ASSERT_EQ(dispatcher.slot.lastSync, dispatcher.slot.lastModifiedLocal);
    ASSERT_EQ(SlotSyncStatus::SYNCED, dispatcher.slot.slotSyncStatus);
    AssertSlotInfoEqual(dispatcher.slot, TEST_TEMP_FILEPATH);

    remove(TEST_TEMP_FILEPATH);
    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingLoadSlot_success)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is update from the response
        0, // setting local to 0 to force it to be older then cloud
        0, // cloud time is updated from the response
        0, // last sync must be equal to local in this case, else it will indicate a conflict
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> slotSyncStatusResponse = std::make_shared<FakeHttpResponse>();
    slotSyncStatusResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotSyncStatusResponse->SetResponseBody(TEST_RESPONSE);

    std::shared_ptr<FakeHttpResponse> slotS3PresignedUrlResponse = std::make_shared<FakeHttpResponse>();
    slotS3PresignedUrlResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotS3PresignedUrlResponse->SetResponseBody(TEST_GENERATE_S3_PRESIGNED_URL_RESPONSE);

    std::shared_ptr<FakeHttpResponse> slotDownloadResponse = std::make_shared<FakeHttpResponse>();
    slotDownloadResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotDownloadResponse->SetResponseBody(TEST_SLOT_DOWNLOAD_RESPONSE);
    slotDownloadResponse->AddHeader(TEST_SHA_256_METADATA_HEADER, TEST_SLOT_DOWNLOAD_SHA_256);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(slotSyncStatusResponse))
        .WillOnce(Return(slotS3PresignedUrlResponse))
        .WillOnce(Return(slotDownloadResponse));

    uint8_t data[TEST_SLOT_DOWNLOAD_RESPONSE_SIZE];
    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        data,
        TEST_SLOT_DOWNLOAD_RESPONSE_SIZE,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    Dispatcher dispatcher;
    // act
    const unsigned int response = GameKitLoadSlot(gameSavingInstance, &dispatcher, slotDataResponseCallback, testModel);

    // assert
    ASSERT_EQ(response, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(1, dispatcher.slotCount);
    ASSERT_EQ(1, dispatcher.syncedSlots.size());
    ASSERT_EQ(0, strcmp(dispatcher.slot.slotName.c_str(), dispatcher.syncedSlots[0].slotName.c_str()));
    ASSERT_EQ(0, strcmp(TEST_SLOT_NAME, dispatcher.slot.slotName.c_str()));
    ASSERT_EQ(0, strcmp(TEST_METADATA_CLOUD, dispatcher.slot.metadataLocal.c_str()));
    ASSERT_EQ(0, strcmp(TEST_METADATA_CLOUD, dispatcher.slot.metadataCloud.c_str()));
    ASSERT_EQ(TEST_SIZE_CLOUD, dispatcher.slot.sizeLocal);
    ASSERT_EQ(TEST_SIZE_CLOUD, dispatcher.slot.sizeCloud);
    ASSERT_EQ(cloud.Millis(), dispatcher.slot.lastModifiedLocal.Millis());
    ASSERT_EQ(cloud.Millis(), dispatcher.slot.lastModifiedCloud.Millis());
    ASSERT_EQ(cloud.Millis(), dispatcher.slot.lastSync.Millis());
    ASSERT_EQ(SlotSyncStatus::SYNCED, dispatcher.slot.slotSyncStatus);
    ASSERT_EQ(TEST_SLOT_DOWNLOAD_RESPONSE_SIZE, dispatcher.dataSize);
    ASSERT_EQ(data, dispatcher.data);
    uint8_t expectedData[TEST_SLOT_DOWNLOAD_RESPONSE_SIZE];
    memcpy((void*)expectedData, (void*)TEST_SLOT_DOWNLOAD_RESPONSE.c_str(), TEST_SLOT_DOWNLOAD_RESPONSE_SIZE);
    for (int i = 0; i < TEST_SLOT_DOWNLOAD_RESPONSE_SIZE; i++)
    {
        ASSERT_EQ(expectedData[i], data[i]);
    }

    AssertSlotInfoEqual(dispatcher.slot, TEST_TEMP_FILEPATH);
    remove(TEST_TEMP_FILEPATH);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingLoadSlot_invalid_sha)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is update from the response
        0, // setting local to 0 to force it to be older then cloud
        0, // cloud time is updated from the response
        0, // last sync must be equal to local in this case, else it will indicate a conflict
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> slotSyncStatusResponse = std::make_shared<FakeHttpResponse>();
    slotSyncStatusResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotSyncStatusResponse->SetResponseBody(TEST_RESPONSE);

    std::shared_ptr<FakeHttpResponse> slotS3PresignedUrlResponse = std::make_shared<FakeHttpResponse>();
    slotS3PresignedUrlResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotS3PresignedUrlResponse->SetResponseBody(TEST_GENERATE_S3_PRESIGNED_URL_RESPONSE);

    std::shared_ptr<FakeHttpResponse> slotDownloadResponse = std::make_shared<FakeHttpResponse>();
    slotDownloadResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotDownloadResponse->SetResponseBody(TEST_SLOT_DOWNLOAD_RESPONSE);
    slotDownloadResponse->AddHeader(TEST_SHA_256_METADATA_HEADER, "some malformed sha-256");

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(slotSyncStatusResponse))
        .WillOnce(Return(slotS3PresignedUrlResponse))
        .WillOnce(Return(slotDownloadResponse));

    uint8_t data[TEST_SLOT_DOWNLOAD_RESPONSE_SIZE];
    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        data,
        TEST_SLOT_DOWNLOAD_RESPONSE_SIZE,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    Dispatcher dispatcher;
    // act
    const unsigned int response = GameKitLoadSlot(gameSavingInstance, &dispatcher, slotDataResponseCallback, testModel);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_GAME_SAVING_SLOT_TAMPERED, response, dispatcher);
    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingLoadSlot_missing_sha)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is update from the response
        0, // setting local to 0 to force it to be older then cloud
        0, // cloud time is updated from the response
        0, // last sync must be equal to local in this case, else it will indicate a conflict
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> slotSyncStatusResponse = std::make_shared<FakeHttpResponse>();
    slotSyncStatusResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotSyncStatusResponse->SetResponseBody(TEST_RESPONSE);

    std::shared_ptr<FakeHttpResponse> slotS3PresignedUrlResponse = std::make_shared<FakeHttpResponse>();
    slotS3PresignedUrlResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotS3PresignedUrlResponse->SetResponseBody(TEST_GENERATE_S3_PRESIGNED_URL_RESPONSE);

    std::shared_ptr<FakeHttpResponse> slotDownloadResponse = std::make_shared<FakeHttpResponse>();
    slotDownloadResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotDownloadResponse->SetResponseBody(TEST_SLOT_DOWNLOAD_RESPONSE);
    // No sha-256 header returned

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(slotSyncStatusResponse))
        .WillOnce(Return(slotS3PresignedUrlResponse))
        .WillOnce(Return(slotDownloadResponse));

    uint8_t data[TEST_SLOT_DOWNLOAD_RESPONSE_SIZE];
    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        data,
        TEST_SLOT_DOWNLOAD_RESPONSE_SIZE,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    Dispatcher dispatcher;
    // act
    const unsigned int response = GameKitLoadSlot(gameSavingInstance, &dispatcher, slotDataResponseCallback, testModel);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_GAME_SAVING_MISSING_SHA, response, dispatcher);
    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingLoadSlot_invalid_lambda_response)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is update from the response
        0, // setting local to 0 to force it to be older then cloud
        0, // cloud time is updated from the response
        0, // last sync must be equal to local in this case, else it will indicate a conflict
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> slotSyncStatusResponse = std::make_shared<FakeHttpResponse>();
    slotSyncStatusResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotSyncStatusResponse->SetResponseBody(TEST_RESPONSE);

    std::shared_ptr<FakeHttpResponse> slotS3PresignedUrlResponse = std::make_shared<FakeHttpResponse>();
    slotS3PresignedUrlResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotS3PresignedUrlResponse->SetResponseBody(TEST_GENERATE_MALFORMED_S3_PRESIGNED_URL_RESPONSE);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(slotSyncStatusResponse))
        .WillOnce(Return(slotS3PresignedUrlResponse));

    uint8_t data[TEST_SLOT_DOWNLOAD_RESPONSE_SIZE];
    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        data,
        TEST_SLOT_DOWNLOAD_RESPONSE_SIZE,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    Dispatcher dispatcher;
    // act
    const unsigned int response = GameKitLoadSlot(gameSavingInstance, &dispatcher, slotDataResponseCallback, testModel);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_PARSE_JSON_FAILED, response, dispatcher);
    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingLoadSlot_success_overwrite)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is update from the response
        0, // setting local to 0 to force it to be older then cloud
        0, // cloud time is updated from the response
        0, // last sync must be equal to local in this case, else it will indicate a conflict
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> slotSyncStatusResponse = std::make_shared<FakeHttpResponse>();
    slotSyncStatusResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotSyncStatusResponse->SetResponseBody(TEST_RESPONSE);

    std::shared_ptr<FakeHttpResponse> slotS3PresignedUrlResponse = std::make_shared<FakeHttpResponse>();
    slotS3PresignedUrlResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotS3PresignedUrlResponse->SetResponseBody(TEST_GENERATE_S3_PRESIGNED_URL_RESPONSE);

    std::shared_ptr<FakeHttpResponse> slotDownloadResponse = std::make_shared<FakeHttpResponse>();
    slotDownloadResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotDownloadResponse->SetResponseBody(TEST_SLOT_DOWNLOAD_RESPONSE);
    slotDownloadResponse->AddHeader(TEST_SHA_256_METADATA_HEADER, TEST_SLOT_DOWNLOAD_SHA_256);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(slotSyncStatusResponse))
        .WillOnce(Return(slotS3PresignedUrlResponse))
        .WillOnce(Return(slotDownloadResponse));

    uint8_t data[TEST_SLOT_DOWNLOAD_RESPONSE_SIZE];
    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        true, // override sync
        data,
        TEST_SLOT_DOWNLOAD_RESPONSE_SIZE,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    Dispatcher dispatcher;
    // act
    const unsigned int response = GameKitLoadSlot(gameSavingInstance, &dispatcher, slotDataResponseCallback, testModel);

    // assert
    ASSERT_EQ(response, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(1, dispatcher.slotCount);
    ASSERT_EQ(1, dispatcher.syncedSlots.size());
    ASSERT_EQ(0, strcmp(dispatcher.slot.slotName.c_str(), dispatcher.syncedSlots[0].slotName.c_str()));
    ASSERT_EQ(0, strcmp(TEST_SLOT_NAME, dispatcher.slot.slotName.c_str()));
    ASSERT_EQ(0, strcmp(TEST_METADATA_CLOUD, dispatcher.slot.metadataLocal.c_str()));
    ASSERT_EQ(0, strcmp(TEST_METADATA_CLOUD, dispatcher.slot.metadataCloud.c_str()));
    ASSERT_EQ(TEST_SIZE_CLOUD, dispatcher.slot.sizeLocal);
    ASSERT_EQ(TEST_SIZE_CLOUD, dispatcher.slot.sizeCloud);
    ASSERT_EQ(cloud.Millis(), dispatcher.slot.lastModifiedLocal.Millis());
    ASSERT_EQ(cloud.Millis(), dispatcher.slot.lastModifiedCloud.Millis());
    ASSERT_EQ(cloud.Millis(), dispatcher.slot.lastSync.Millis());
    ASSERT_EQ(SlotSyncStatus::SYNCED, dispatcher.slot.slotSyncStatus);
    ASSERT_EQ(TEST_SLOT_DOWNLOAD_RESPONSE_SIZE, dispatcher.dataSize);
    ASSERT_EQ(data, dispatcher.data);
    uint8_t expectedData[TEST_SLOT_DOWNLOAD_RESPONSE_SIZE];
    memcpy((void*)expectedData, (void*)TEST_SLOT_DOWNLOAD_RESPONSE.c_str(), TEST_SLOT_DOWNLOAD_RESPONSE_SIZE);
    for (int i = 0; i < TEST_SLOT_DOWNLOAD_RESPONSE_SIZE; i++)
    {
        ASSERT_EQ(expectedData[i], data[i]);
    }

    AssertSlotInfoEqual(dispatcher.slot, TEST_TEMP_FILEPATH);
    remove(TEST_TEMP_FILEPATH);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingLoadSlot_missing_local_slot)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        0, // for a new entry that is not in the cloud, the last sync will not be set, ie last == cloud
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    static_cast<GameKit::GameSaving::GameSaving*>(gameSavingInstance)->ClearSyncedSlots();
    const unsigned int expectedSlotCount = 0;

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).Times(0);

    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        nullptr,
        0,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitLoadSlot(gameSavingInstance, &dispatcher, slotDataResponseCallback, testModel);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_GAME_SAVING_SLOT_NOT_FOUND, response, dispatcher, expectedSlotCount);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingLoadSlot_malformed_slot_name)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        0, // for a new entry that is not in the cloud, the last sync will not be set, ie last == cloud
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    static_cast<GameKit::GameSaving::GameSaving*>(gameSavingInstance)->ClearSyncedSlots();
    const unsigned int expectedSlotCount = 0;

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).Times(0);

    const GameSavingModel testModel = {
        TEST_MALFORMED_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        nullptr,
        0,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitLoadSlot(gameSavingInstance, &dispatcher, slotDataResponseCallback, testModel);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_GAME_SAVING_MALFORMED_SLOT_NAME, response, dispatcher, expectedSlotCount);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingLoadSlot_should_upload_local)
{
    // arrange
    last = ToAwsString(TEST_LAST_SYNC_OLD_CLOUD_TIME);
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_OLD_CLOUD_TIME);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        nullptr,
        0,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitLoadSlot(gameSavingInstance, &dispatcher, slotDataResponseCallback, testModel);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_GAME_SAVING_LOCAL_SLOT_IS_NEWER, response, dispatcher);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingLoadSlot_in_conflict)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is update from the response
        local.Millis(), // setting local to 0 to force it to be older then cloud
        0, // cloud time is updated from the response
        0, // if last sync is not equal to either local or cloud, this indicates a possible conflict
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        nullptr,
        0,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitLoadSlot(gameSavingInstance, &dispatcher, slotDataResponseCallback, testModel);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_GAME_SAVING_SYNC_CONFLICT, response, dispatcher);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingLoadSlot_already_synced)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> slotSyncStatusResponse = std::make_shared<FakeHttpResponse>();
    slotSyncStatusResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotSyncStatusResponse->SetResponseBody(TEST_RESPONSE);

    std::shared_ptr<FakeHttpResponse> slotS3PresignedUrlResponse = std::make_shared<FakeHttpResponse>();
    slotS3PresignedUrlResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotS3PresignedUrlResponse->SetResponseBody(TEST_GENERATE_S3_PRESIGNED_URL_RESPONSE);

    std::shared_ptr<FakeHttpResponse> slotDownloadResponse = std::make_shared<FakeHttpResponse>();
    slotDownloadResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotDownloadResponse->SetResponseBody(TEST_SLOT_DOWNLOAD_RESPONSE);
    slotDownloadResponse->AddHeader(TEST_SHA_256_METADATA_HEADER, TEST_SLOT_DOWNLOAD_SHA_256);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(slotSyncStatusResponse))
        .WillOnce(Return(slotS3PresignedUrlResponse))
        .WillOnce(Return(slotDownloadResponse));

    uint8_t data[TEST_SLOT_DOWNLOAD_RESPONSE_SIZE];
    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        data,
        TEST_SLOT_DOWNLOAD_RESPONSE_SIZE,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitLoadSlot(gameSavingInstance, &dispatcher, slotDataResponseCallback, testModel);

    // assert
    ASSERT_EQ(response, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GameKit::GAMEKIT_SUCCESS);
    ASSERT_EQ(1, dispatcher.slotCount);
    ASSERT_EQ(1, dispatcher.syncedSlots.size());
    ASSERT_EQ(0, strcmp(dispatcher.slot.slotName.c_str(), dispatcher.syncedSlots[0].slotName.c_str()));
    ASSERT_EQ(0, strcmp(TEST_SLOT_NAME, dispatcher.slot.slotName.c_str()));
    ASSERT_EQ(0, strcmp(TEST_METADATA_CLOUD, dispatcher.slot.metadataLocal.c_str()));
    ASSERT_EQ(0, strcmp(TEST_METADATA_CLOUD, dispatcher.slot.metadataCloud.c_str()));
    ASSERT_EQ(TEST_SIZE_CLOUD, dispatcher.slot.sizeLocal);
    ASSERT_EQ(TEST_SIZE_CLOUD, dispatcher.slot.sizeCloud);
    ASSERT_EQ(cloud.Millis(), dispatcher.slot.lastModifiedLocal.Millis());
    ASSERT_EQ(cloud.Millis(), dispatcher.slot.lastModifiedCloud.Millis());
    ASSERT_EQ(cloud.Millis(), dispatcher.slot.lastSync.Millis());
    ASSERT_EQ(SlotSyncStatus::SYNCED, dispatcher.slot.slotSyncStatus);

    AssertSlotInfoEqual(dispatcher.slot, TEST_TEMP_FILEPATH);
    remove(TEST_TEMP_FILEPATH);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingLoadSlot_buffer_too_small)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata is updated from the response
        TEST_SIZE_LOCAL,
        0, // cloud size is updated from the response
        local.Millis(),
        0, // cloud time is update from the response
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> slotSyncStatusResponse = std::make_shared<FakeHttpResponse>();
    slotSyncStatusResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotSyncStatusResponse->SetResponseBody(TEST_RESPONSE);

    std::shared_ptr<FakeHttpResponse> slotS3PresignedUrlResponse = std::make_shared<FakeHttpResponse>();
    slotS3PresignedUrlResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotS3PresignedUrlResponse->SetResponseBody(TEST_GENERATE_S3_PRESIGNED_URL_RESPONSE);

    std::shared_ptr<FakeHttpResponse> slotDownloadResponse = std::make_shared<FakeHttpResponse>();
    slotDownloadResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    slotDownloadResponse->SetResponseBody(TEST_SLOT_DOWNLOAD_RESPONSE);
    slotDownloadResponse->AddHeader(TEST_SHA_256_METADATA_HEADER, TEST_SLOT_DOWNLOAD_SHA_256);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(slotSyncStatusResponse))
        .WillOnce(Return(slotS3PresignedUrlResponse))
        .WillOnce(Return(slotDownloadResponse));

    uint8_t data[TEST_SLOT_DOWNLOAD_RESPONSE_SIZE - 1];
    const GameSavingModel testModel = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        0, // epoch time
        false, // override sync
        data,
        TEST_SLOT_DOWNLOAD_RESPONSE_SIZE - 1,
        TEST_TEMP_FILEPATH, // local slot info file path
    };

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitLoadSlot(gameSavingInstance, &dispatcher, slotDataResponseCallback, testModel);

    // assert
    ASSERT_EQ(response, GameKit::GAMEKIT_ERROR_GAME_SAVING_BUFFER_TOO_SMALL);
    ASSERT_EQ(dispatcher.callStatus, GameKit::GAMEKIT_ERROR_GAME_SAVING_BUFFER_TOO_SMALL);

    remove(TEST_TEMP_FILEPATH);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingDeleteCloudSlot_success)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        TEST_METADATA_LOCAL,
        TEST_SIZE_LOCAL,
        TEST_SIZE_LOCAL,
        local.Millis(),
        cloud.Millis(),
        last.Millis(),
        SlotSyncStatus::SYNCED
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_NO_ENTRY);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitDeleteSlot(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    // assert
    AssertCallSucceeded(response, dispatcher, testSlot, 0);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingDeleteCloudSlot_save_only_exists_locally)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        "", // cloud metadata has not been synced yet
        TEST_SIZE_LOCAL,
        0, // cloud size has not been synced yet
        local.Millis(),
        0, // cloud time has not been synced yet
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_NO_ENTRY);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitDeleteSlot(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    // assert
    AssertCallSucceeded(response, dispatcher, testSlot, 0);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingDeleteCloudSlot_multiple_local_slots)
{
    // arrange
    const Slot expectedDeletedSlot{
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        TEST_METADATA_LOCAL,
        TEST_SIZE_LOCAL,
        TEST_SIZE_LOCAL,
        local.Millis(),
        cloud.Millis(),
        last.Millis(),
        SlotSyncStatus::SYNCED
    };
    const Slot expectedRemainingSlot = {
        TEST_SLOT_NAME_3,
        TEST_METADATA_LOCAL,
        "", // cloud metadata has not been synced yet
        TEST_SIZE_LOCAL,
        0, // cloud size has not been synced yet
        local.Millis(),
        0, // cloud time has not been synced yet
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    std::vector<Slot> testSlots;
    testSlots.push_back(expectedDeletedSlot);
    testSlots.push_back(expectedRemainingSlot);

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(testSlots.data(), testSlots.size());
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_NO_ENTRY);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    const char* deletedSlotName = TEST_SLOT_NAME;

    // act
    const unsigned int response = GameKitDeleteSlot(gameSavingInstance, &dispatcher, slotActionCallback, deletedSlotName);

    // assert
    AssertCallSucceeded(response, dispatcher, expectedDeletedSlot, 1);
    ASSERT_FALSE(HasSlot(dispatcher.syncedSlots, deletedSlotName));
    ASSERT_TRUE(HasSlot(dispatcher.syncedSlots, TEST_SLOT_NAME_3));
    AssertEqual(expectedRemainingSlot, dispatcher.syncedSlots[0]);
    AssertEqual(expectedDeletedSlot, dispatcher.slot);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingDeleteCloudSlot_missing_local_slot)
{
    // arrange
    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance();
    SetMocks(gameSavingInstance);

    static_cast<GameKit::GameSaving::GameSaving*>(gameSavingInstance)->ClearSyncedSlots();
    const unsigned int expectedSlotCount = 0;

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_NO_ENTRY);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).Times(0);

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitDeleteSlot(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_GAME_SAVING_SLOT_NOT_FOUND, response, dispatcher, expectedSlotCount);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingDeleteCloudSlot_malformed_slot_name)
{
    // arrange
    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance();
    SetMocks(gameSavingInstance);

    static_cast<GameKit::GameSaving::GameSaving*>(gameSavingInstance)->ClearSyncedSlots();
    const unsigned int expectedSlotCount = 0;

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).Times(0);

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitDeleteSlot(gameSavingInstance, &dispatcher, slotActionCallback, TEST_MALFORMED_SLOT_NAME);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_GAME_SAVING_MALFORMED_SLOT_NAME, response, dispatcher, expectedSlotCount);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingDeleteCloudSlot_missing_token)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        TEST_METADATA_LOCAL,
        TEST_SIZE_LOCAL,
        TEST_SIZE_LOCAL,
        local.Millis(),
        cloud.Millis(),
        last.Millis(),
        SlotSyncStatus::SYNCED
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);
    sessionManager->DeleteToken(GameKit::TokenType::IdToken);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).Times(0);

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitDeleteSlot(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_NO_ID_TOKEN, response, dispatcher);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingDeleteCloudSlot_http_request_failed)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        TEST_METADATA_LOCAL,
        TEST_SIZE_LOCAL,
        TEST_SIZE_LOCAL,
        local.Millis(),
        cloud.Millis(),
        last.Millis(),
        SlotSyncStatus::SYNCED
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(500));

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitDeleteSlot(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_HTTP_REQUEST_FAILED, response, dispatcher);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingDeleteCloudSlot_response_body_not_in_json_format)
{
    // arrange
    Slot testSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        TEST_METADATA_LOCAL,
        TEST_SIZE_LOCAL,
        TEST_SIZE_LOCAL,
        local.Millis(),
        cloud.Millis(),
        last.Millis(),
        SlotSyncStatus::SYNCED
    };

    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&testSlot, 1);
    SetMocks(gameSavingInstance);

    std::shared_ptr<FakeHttpResponse> testResponse = std::make_shared<FakeHttpResponse>();
    testResponse->SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(200));
    testResponse->SetResponseBody(TEST_RESPONSE_INVALID_JSON);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _)).WillOnce(Return(testResponse));

    Dispatcher dispatcher;

    // act
    const unsigned int response = GameKitDeleteSlot(gameSavingInstance, &dispatcher, slotActionCallback, TEST_SLOT_NAME);

    // assert
    AssertCallFailed(GameKit::GAMEKIT_ERROR_PARSE_JSON_FAILED, response, dispatcher);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingLoadSlotInformation_success)
{
    // arrange
    const Slot expectedSlot = {
        TEST_SLOT_NAME,
        TEST_METADATA_LOCAL,
        TEST_METADATA_CLOUD,
        TEST_SIZE_LOCAL,
        TEST_SIZE_CLOUD,
        local.Millis(),
        cloud.Millis(),
        last.Millis(),
        SlotSyncStatus::UNKNOWN
    };

    Slot emptySlot;
    const char* path = TEST_EXPECTED_SAVED_SLOT_INFORMATION_FILEPATH;

    // act (loads expected slot)
    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&emptySlot, 0, &path, 1);

    auto gameSaving = static_cast<GameKit::GameSaving::GameSaving*>(gameSavingInstance);
    auto slots = gameSaving->GetSyncedSlots();

    // assert
    ASSERT_EQ(1, slots.size());
    AssertEqual(expectedSlot, slots[TEST_SLOT_NAME]);

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingLoadSlotInformation_bad_path_not_loaded)
{
    // arrange
    Slot emptySlot;
    const char* fakePath = TEST_FAKE_PATH;

    // act
    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&emptySlot, 0, &fakePath, 1);

    auto gameSaving = static_cast<GameKit::GameSaving::GameSaving*>(gameSavingInstance);
    auto slots = gameSaving->GetSyncedSlots();

    // assert
    ASSERT_EQ(0, slots.size());

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingLoadSlotInformation_json_parse_error)
{
    // arrange
    Slot emptySlot;
    const char* invalidSlotInfoPath = TEST_INVALID_SAVED_SLOT_INFORMATION_FILEPATH;

    // act
    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&emptySlot, 0, &invalidSlotInfoPath, 1);

    auto gameSaving = static_cast<GameKit::GameSaving::GameSaving*>(gameSavingInstance);
    auto slots = gameSaving->GetSyncedSlots();

    // assert
    ASSERT_EQ(0, slots.size());

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}

TEST_F(GameKitGameSavingExportsTestFixture, TestGameKitGameSavingLoadSlotInformation_null_json_values)
{
    // arrange
    Slot emptySlot;
    const char* nullSlotInfoPath = TEST_NULL_SAVED_SLOT_INFORMATION_FILEPATH;

    // act
    GAMEKIT_GAME_SAVING_INSTANCE_HANDLE const gameSavingInstance = CreateGameSavingInstance(&emptySlot, 0, &nullSlotInfoPath, 1);

    auto gameSaving = static_cast<GameKit::GameSaving::GameSaving*>(gameSavingInstance);
    auto slots = gameSaving->GetSyncedSlots();

    // assert
    ASSERT_EQ(0, slots.size());

    GameKitGameSavingInstanceRelease(gameSavingInstance);
}
