#include <iostream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "gamekit_achievements_exports_tests.h"

#include "aws/gamekit/authentication/exports.h"
#include "aws/gamekit/core/logging.h"
#include "aws/gamekit/core/awsclients/api_initializer.h"

using namespace GameKit::Tests::AchievementsExports;
using namespace testing;

#define CLIENT_CONFIG_FILE "../core/test_data/sampleplugin/instance/testgame/dev/awsGameKitClientConfig.yml"

void DispatchCallback(DISPATCH_RECEIVER_HANDLE receiver, const char* message)
{
    ((Dispatcher*) receiver)->CallbackHandler(message);
}

void GameKit::Tests::AchievementsExports::Dispatcher::CallbackHandler(const char* message)
{
    this->message = message;
}

GameKitAchievementsExportsTestFixture::GameKitAchievementsExportsTestFixture()
{}

GameKitAchievementsExportsTestFixture::~GameKitAchievementsExportsTestFixture()
{}

void GameKitAchievementsExportsTestFixture::SetUp()
{
    testStackInitializer.Initialize();
}

void GameKitAchievementsExportsTestFixture::TearDown()
{
    if (testSessionManager != nullptr)
    {
        GameKitSessionManagerInstanceRelease(testSessionManager);
        testSessionManager = nullptr;
    }

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));

    testStackInitializer.CleanupAndLog<TestLogger>();
    TestExecutionUtils::AbortOnFailureIfEnabled();
}

void* GameKitAchievementsExportsTestFixture::createAdminAchievementsInstance(bool setToken=true)
{
    testSessionManager = GameKitSessionManagerInstanceCreate(CLIENT_CONFIG_FILE, TestLogger::Log);
    if (setToken)
    {
        (static_cast<GameKit::Authentication::GameKitSessionManager *>(testSessionManager))->SetToken(TokenType::IdToken, "test_token");
    }

    return GameKitAchievementsInstanceCreateWithSessionManager(testSessionManager, TestLogger::Log);
}

void GameKitAchievementsExportsTestFixture::setAchievementsMocks(void* instance)
{
    GameKit::Achievements::Achievements* achievementsInstance = static_cast<GameKit::Achievements::Achievements*>(instance);
    this->mockHttpClient = std::make_shared<MockHttpClient>();
    achievementsInstance->SetHttpClient(this->mockHttpClient);
}

TEST_F(GameKitAchievementsExportsTestFixture, TestGameKitAchievementsInstanceCreate_Success)
{
    // act
    GameKit::GameKitFeature* achievementsInstance = (GameKit::GameKitFeature*)createAdminAchievementsInstance();

    // assert
    ASSERT_NE(achievementsInstance, nullptr);

    GameKitAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAchievementsExportsTestFixture, TestGameKitAchievementsInstanceRelease_Success)
{
    // arrange
    void* achievementsInstance = createAdminAchievementsInstance();

    // act
    GameKitAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAchievementsExportsTestFixture, TestGameKitAchievementsInstanceRelease_SessionManager_Persists)
{
    // arrange
    testSessionManager = GameKitSessionManagerInstanceCreate(CLIENT_CONFIG_FILE, TestLogger::Log);

    void* achievementsInstance = GameKitAchievementsInstanceCreateWithSessionManager(testSessionManager, TestLogger::Log);

    // act
    GameKitAchievementsInstanceRelease(achievementsInstance);

    // assert
    ASSERT_NE(nullptr, testSessionManager);
}

TEST_F(GameKitAchievementsExportsTestFixture, TestGameKitAchievementsGetAchievement_Success)
{
    // arrange
    void* achievementsInstance = createAdminAchievementsInstance();
    setAchievementsMocks(achievementsInstance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(200));
    response->SetResponseBody("{}");

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
       .WillOnce(Return(response));

    auto dispatcher = Dispatcher();

    // act
    auto result = GameKitGetAchievement(achievementsInstance, "fake_achievement_id", dispatcher.get(), DispatchCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);

    GameKitAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAchievementsExportsTestFixture, TestGameKitAchievementsGetAchievement_NoToken)
{
    // arrange
    void* achievementsInstance = createAdminAchievementsInstance(false);
    setAchievementsMocks(achievementsInstance);

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
        .Times(0);

    // act
    auto dispatcher = Dispatcher();
    auto result = GameKitGetAchievement(achievementsInstance, "fake_achievement_id", dispatcher.get(), DispatchCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_NO_ID_TOKEN);

    GameKitAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAchievementsExportsTestFixture, TestGameKitAchievementsGetAchievement_NoAchievementId)
{
    // arrange
    void* achievementsInstance = createAdminAchievementsInstance();
    setAchievementsMocks(achievementsInstance);

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
        .Times(0);

    // act
    auto dispatcher = Dispatcher();
    auto result = GameKitGetAchievement(achievementsInstance, "", dispatcher.get(), DispatchCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_ACHIEVEMENTS_INVALID_ID);

    GameKitAchievementsInstanceRelease(achievementsInstance);
}

// Testing for this error on other methods doesn't increase coverage
TEST_F(GameKitAchievementsExportsTestFixture, TestGameKitAchievementsGetAchievement_HttpError)
{
    // arrange
    void* achievementsInstance = createAdminAchievementsInstance();
    setAchievementsMocks(achievementsInstance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(500));
    response->SetResponseBody("{}");

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(response));

    auto dispatcher = Dispatcher();

    // act
    auto result = GameKitGetAchievement(achievementsInstance, "fake_achievement_id", dispatcher.get(), DispatchCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_HTTP_REQUEST_FAILED);

    GameKitAchievementsInstanceRelease(achievementsInstance);
}

// Testing for this error on other methods doesn't increase coverage
TEST_F(GameKitAchievementsExportsTestFixture, TestGameKitAchievementsGetAchievement_JsonError)
{
    // arrange
    void* achievementsInstance = createAdminAchievementsInstance();
    setAchievementsMocks(achievementsInstance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(200));
    response->SetResponseBody("{\"body\":}");

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(response));

    auto dispatcher = Dispatcher();

    // act
    auto result = GameKitGetAchievement(achievementsInstance, "fake_achievement_id", dispatcher.get(), DispatchCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_PARSE_JSON_FAILED);

    GameKitAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAchievementsExportsTestFixture, TestGameKitAchievementsUpdateAchievement_Success)
{
    // arrange
    void* achievementsInstance = createAdminAchievementsInstance();
    setAchievementsMocks(achievementsInstance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(200));
    response->SetResponseBody("{}");

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(response));

    auto dispatcher = Dispatcher();

    // act
    auto result = GameKitUpdateAchievement(achievementsInstance, "fake_achievement_id", 10, dispatcher.get(), DispatchCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);

    GameKitAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAchievementsExportsTestFixture, TestGameKitAchievementsUpdateAchievement_NoToken)
{
    // arrange
    void* achievementsInstance = createAdminAchievementsInstance(false);
    setAchievementsMocks(achievementsInstance);

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
        .Times(0);

    auto dispatcher = Dispatcher();

    // act
    auto result = GameKitUpdateAchievement(achievementsInstance, "fake_achievement_id", 2, dispatcher.get(), DispatchCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_NO_ID_TOKEN);

    GameKitAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAchievementsExportsTestFixture, TestGameKitAchievementsListAchievements_Success)
{
    // arrange
    void* achievementsInstance = createAdminAchievementsInstance();
    setAchievementsMocks(achievementsInstance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(200));
    response->SetResponseBody("{}");

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(response));

    auto dispatcher = Dispatcher();

    // act
    auto result = GameKitListAchievements(achievementsInstance, 100, false, dispatcher.get(), DispatchCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);

    GameKitAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAchievementsExportsTestFixture, TestGameKitAchievementsListAchievements_PaginatedSuccess)
{
    // arrange
    void* achievementsInstance = createAdminAchievementsInstance();
    setAchievementsMocks(achievementsInstance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(200));
    response->SetResponseBody("{\"paging\": {\"next_start_key\": {\"achievement_id\": \"key\"}, \"paging_token\": \"foo\"}}");

    std::shared_ptr<FakeHttpResponse> secondResponse = std::make_shared<FakeHttpResponse>();
    secondResponse->SetResponseCode(Aws::Http::HttpResponseCode(200));
    secondResponse->SetResponseBody("{}");

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(response))
        .WillOnce(Return(secondResponse));

    auto dispatcher = Dispatcher();

    // act
    auto result = GameKitListAchievements(achievementsInstance, 100, false, dispatcher.get(), DispatchCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);

    GameKitAchievementsInstanceRelease(achievementsInstance);
}
