// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <iostream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "gamekit_admin_achievements_exports_tests.h"

#include <aws/gamekit/authentication/exports.h>
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/awsclients/api_initializer.h>

using namespace GameKit::Tests::AdminAchievementsExports;
using namespace testing;

void AdminAchievementsDispatchCallback(DISPATCH_RECEIVER_HANDLE receiver, const char* message)
{
    ((GameKit::Tests::AdminAchievementsExports::Dispatcher*) receiver)->CallbackHandler(message);
}

void GameKit::Tests::AdminAchievementsExports::Dispatcher::CallbackHandler(const char* message)
{
    this->message = message;
}

const std::string GameKitAdminAchievementsExportsTestFixture::MOCK_ACCESS_ID = "ACCESSKEYID123456789";
const std::string GameKitAdminAchievementsExportsTestFixture::MOCK_ACCESS_SECRET = "secret";
const std::string GameKitAdminAchievementsExportsTestFixture::MOCK_SESSION_TOKEN = "sessionToken";

GameKitAdminAchievementsExportsTestFixture::GameKitAdminAchievementsExportsTestFixture()
{}

GameKitAdminAchievementsExportsTestFixture::~GameKitAdminAchievementsExportsTestFixture()
{}

void GameKitAdminAchievementsExportsTestFixture::SetUp()
{
    TestLogger::Clear();
    testStackInitializer.Initialize();

    ::testing::internal::CaptureStdout();
}

void GameKitAdminAchievementsExportsTestFixture::TearDown()
{
    testStackInitializer.Cleanup();

    std::string capturedStdout = ::testing::internal::GetCapturedStdout().c_str();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

void* GameKitAdminAchievementsExportsTestFixture::createAchievementsInstance(bool setToken=true)
{
    void* sessMgr = GameKitSessionManagerInstanceCreate("../core/test_data/sampleplugin/instance/testgame/dev/awsGameKitClientConfig.yml", TestLogger::Log);
    if (setToken)
    {
        (static_cast<GameKit::Authentication::GameKitSessionManager *>(sessMgr))->SetToken(TokenType::IdToken, "test_token");
    }

    this->mockAccountCredentials = {
        "fake-region",
        MOCK_ACCESS_ID.c_str(),
        MOCK_ACCESS_SECRET.c_str(),
        "1234"
    };

    this->mockAccountInfo = {
        "dev",
        "123456789012",
        "test",
        "testgame"
    };

    return GameKitAdminAchievementsInstanceCreateWithSessionManager(sessMgr, "../core/test_data/sampleplugin/base", this->mockAccountCredentials, this->mockAccountInfo, TestLogger::Log);
}

void GameKitAdminAchievementsExportsTestFixture::setAchievementsMocks(void* instance)
{
    GameKit::Achievements::AdminAchievements* achievementsInstance = static_cast<GameKit::Achievements::AdminAchievements*>(instance);
    this->mockHttpClient = std::make_shared<MockHttpClient>();
    achievementsInstance->SetHttpClient(this->mockHttpClient);
    setAchievementsAdminCredentials(achievementsInstance);

}

void GameKitAdminAchievementsExportsTestFixture::setAchievementsAdminCredentials(GameKit::Achievements::AdminAchievements* achievementsInstance)
{
    Aws::STS::Model::Credentials credentials;
    std::string accessKeyId{ "ACCESSKEYID123456789" };
    std::string secret{ "secret" };
    std::string sessionToken{ "sessionToken" };
    Aws::Utils::DateTime expirationDate{ Aws::Utils::DateTime::CurrentTimeMillis() + 2 * Achievements::ADMIN_SESSION_EXPIRATION_BUFFER_MILLIS };
    credentials.SetAccessKeyId(ToAwsString(accessKeyId));
    credentials.SetSecretAccessKey(ToAwsString(secret));
    credentials.SetSessionToken(ToAwsString(sessionToken));
    credentials.SetExpiration(expirationDate);
    achievementsInstance->SetAdminApiSessionCredentials(credentials);
}
TEST_F(GameKitAdminAchievementsExportsTestFixture, TestGameKitAchievementsInstanceCreate_Success)
{
    // act
    GameKit::GameKitFeature* achievementsInstance = (GameKit::GameKitFeature*)createAchievementsInstance();

    // assert
    ASSERT_NE(achievementsInstance, nullptr);

    GameKitAdminAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAdminAchievementsExportsTestFixture, TestGameKitAchievementsInstanceRelease_Success)
{
    // arrange
    void* achievementsInstance = createAchievementsInstance();

    // act
    GameKitAdminAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAdminAchievementsExportsTestFixture, TestGameKitAchievementsInstanceRelease_SessionManager_Persists)
{
    // arrange
    void* sessMgr = GameKitSessionManagerInstanceCreate("../core/test_data/sampleplugin/instance/testgame/dev/awsGameKitClientConfig.yml", TestLogger::Log);

    this->mockAccountCredentials = {
        "fake-region",
        MOCK_ACCESS_ID.c_str(),
        MOCK_ACCESS_SECRET.c_str(),
        "1234"
    };

    this->mockAccountInfo = {
        "dev",
        "123456789012",
        "test",
        "testgame"
    };

    void* achievementsInstance = GameKitAdminAchievementsInstanceCreateWithSessionManager(sessMgr, "../core/test_data/sampleplugin/instance/awsGameKitAwsRegionMappings.yml", this->mockAccountCredentials, this->mockAccountInfo, TestLogger::Log);

    // act
    GameKitAdminAchievementsInstanceRelease(achievementsInstance);

    // assert
    ASSERT_NE(nullptr, sessMgr);
}

TEST_F(GameKitAdminAchievementsExportsTestFixture, TestGameKitAchievementsAdminListAchievements_Success)
{
    // arrange
    void* achievementsInstance = createAchievementsInstance();
    setAchievementsMocks(achievementsInstance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(200));
    response->SetResponseBody("{}");

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(response));

    auto dispatcher = GameKit::Tests::AdminAchievementsExports::Dispatcher();

    // act
    auto result = GameKitAdminListAchievements(achievementsInstance, 100, false, dispatcher.get(), AdminAchievementsDispatchCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);

    GameKitAdminAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAdminAchievementsExportsTestFixture, TestGameKitAchievementsAdminListAchievements_PaginatedSuccess)
{
    // arrange
    void* achievementsInstance = createAchievementsInstance();
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

    auto dispatcher = GameKit::Tests::AdminAchievementsExports::Dispatcher();

    // act
    auto result = GameKitAdminListAchievements(achievementsInstance, 100, false, dispatcher.get(), AdminAchievementsDispatchCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);

    GameKitAdminAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAdminAchievementsExportsTestFixture, TestGameKitAchievementsAdminDeleteAchievements_Success)
{
    // arrange
    void* achievementsInstance = createAchievementsInstance();
    setAchievementsMocks(achievementsInstance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(200));
    response->SetResponseBody("{}");

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(response));

    const char* ids[] = {"first_id", "second_id"};

    // act
    auto result = GameKitAdminDeleteAchievements(achievementsInstance, ids, 2);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);

    GameKitAdminAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAdminAchievementsExportsTestFixture, TestGameKitAchievementsAdminDeleteAchievements_EmptyArraySuccess)
{
    // arrange
    void* achievementsInstance = createAchievementsInstance();
    setAchievementsMocks(achievementsInstance);

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
        .Times(0);

    // act
    auto result = GameKitAdminDeleteAchievements(achievementsInstance, nullptr, 0);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);

    GameKitAdminAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAdminAchievementsExportsTestFixture, TestGameKitAchievementsAdminAddAchievements_Success)
{
    // arrange
    void* achievementsInstance = createAchievementsInstance();
    setAchievementsMocks(achievementsInstance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(200));
    response->SetResponseBody("{}");

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(response));

    GameKit::Achievement one {"id", "title", "lockedDesc", "unlockedDesc", "lockedIcon1", "unlockedIcon1",
                     10, 10, 10, true, false, false};

    GameKit::Achievement two {"id", "title", "lockedDesc", "unlockedDesc", "lockedIcon2", "unlockedIcon2",
                     10, 10, 10, true, false, false};

    GameKit::Achievement achievements[] = {one, two};

    // act
    auto result = GameKitAdminAddAchievements(achievementsInstance, achievements, 2);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);

    GameKitAdminAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAdminAchievementsExportsTestFixture, TestGameKitAchievementsAdminAddAchievements_EmptyArraySuccess)
{
    // arrange
    void* achievementsInstance = createAchievementsInstance();
    setAchievementsMocks(achievementsInstance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(200));
    response->SetResponseBody("{}");

    // act
    auto result = GameKitAdminAddAchievements(achievementsInstance, nullptr, 0);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);

    GameKitAdminAchievementsInstanceRelease(achievementsInstance);
}

TEST_F(GameKitAdminAchievementsExportsTestFixture, TestGameKitAchievementsAdminListAchievements_403_Recover)
{
    // arrange
    void* achievementsExportInstance = createAchievementsInstance();
    setAchievementsMocks(achievementsExportInstance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(403));
    response->SetResponseBody("{}");

    std::shared_ptr<FakeHttpResponse> secondResponse = std::make_shared<FakeHttpResponse>();
    secondResponse->SetResponseCode(Aws::Http::HttpResponseCode(200));
    secondResponse->SetResponseBody("{}");

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(response))
        .WillOnce(Return(secondResponse));

    Aws::STS::Model::AssumeRoleResult assumeRoleResult;
    Aws::STS::Model::Credentials credentials;
    credentials.SetAccessKeyId(ToAwsString(MOCK_ACCESS_ID));
    credentials.SetSecretAccessKey(ToAwsString(MOCK_ACCESS_SECRET));
    credentials.SetSessionToken(ToAwsString(MOCK_SESSION_TOKEN));
    assumeRoleResult.SetCredentials(credentials);
    auto outcome = Aws::STS::Model::AssumeRoleOutcome(assumeRoleResult);
    std::shared_ptr<GameKit::Mocks::MockSTSClient> mockStsClient = std::make_shared<GameKit::Mocks::MockSTSClient>();

    EXPECT_CALL(*mockStsClient, AssumeRole(_))
        .Times(1)
        .WillOnce(Return(outcome));

    auto dispatcher = GameKit::Tests::AdminAchievementsExports::Dispatcher();
    GameKit::Achievements::AdminAchievements* achievementsInstance = static_cast<GameKit::Achievements::AdminAchievements*>(achievementsExportInstance);
    achievementsInstance->SetSTSClient(mockStsClient);

    // act
    unsigned int result = GameKitAdminListAchievements(achievementsExportInstance, 100, true, dispatcher.get(), AdminAchievementsDispatchCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);

    GameKitAdminAchievementsInstanceRelease(achievementsExportInstance);
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&mockStsClient));
}
TEST_F(GameKitAdminAchievementsExportsTestFixture, TestGameKitAchievementsAdminAddAchievements_403_Recover)
{
    // arrange
    void* achievementsExportInstance = createAchievementsInstance();
    setAchievementsMocks(achievementsExportInstance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(403));
    response->SetResponseBody("{}");

    std::shared_ptr<FakeHttpResponse> secondResponse = std::make_shared<FakeHttpResponse>();
    secondResponse->SetResponseCode(Aws::Http::HttpResponseCode(200));
    secondResponse->SetResponseBody("{}");

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(response))
        .WillOnce(Return(secondResponse));

    Aws::STS::Model::AssumeRoleResult assumeRoleResult;
    Aws::STS::Model::Credentials credentials;
    credentials.SetAccessKeyId(ToAwsString(MOCK_ACCESS_ID));
    credentials.SetSecretAccessKey(ToAwsString(MOCK_ACCESS_SECRET));
    credentials.SetSessionToken(ToAwsString(MOCK_SESSION_TOKEN));
    assumeRoleResult.SetCredentials(credentials);
    auto outcome = Aws::STS::Model::AssumeRoleOutcome(assumeRoleResult);
    std::shared_ptr<GameKit::Mocks::MockSTSClient> mockStsClient = std::make_shared<GameKit::Mocks::MockSTSClient>();

    EXPECT_CALL(*mockStsClient, AssumeRole(_))
        .Times(1)
        .WillOnce(Return(outcome));

    GameKit::Achievement one{ "id", "title", "lockedDesc", "unlockedDesc", "lockedIcon1", "unlockedIcon1",
                     10, 10, 10, true, false, false };

    GameKit::Achievement two{ "id", "title", "lockedDesc", "unlockedDesc", "lockedIcon2", "unlockedIcon2",
                     10, 10, 10, true, false, false };

    GameKit::Achievement achievements[] = { one, two };


    GameKit::Achievements::AdminAchievements* achievementsInstance = static_cast<GameKit::Achievements::AdminAchievements*>(achievementsExportInstance);
    achievementsInstance->SetSTSClient(mockStsClient);

    // act
    unsigned int result = GameKitAdminAddAchievements(achievementsExportInstance, achievements, 2);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);

    GameKitAdminAchievementsInstanceRelease(achievementsExportInstance);
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&mockStsClient));
}

TEST_F(GameKitAdminAchievementsExportsTestFixture, TestGameKitAchievementsAdminDeleteAchievements_403_Recover)
{
    // arrange
    void* achievementsExportInstance = createAchievementsInstance();
    setAchievementsMocks(achievementsExportInstance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(403));
    response->SetResponseBody("{}");

    std::shared_ptr<FakeHttpResponse> secondResponse = std::make_shared<FakeHttpResponse>();
    secondResponse->SetResponseCode(Aws::Http::HttpResponseCode(200));
    secondResponse->SetResponseBody("{}");

    EXPECT_CALL(*this->mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(response))
        .WillOnce(Return(secondResponse));

    Aws::STS::Model::AssumeRoleResult assumeRoleResult;
    Aws::STS::Model::Credentials credentials;
    credentials.SetAccessKeyId(ToAwsString(MOCK_ACCESS_ID));
    credentials.SetSecretAccessKey(ToAwsString(MOCK_ACCESS_SECRET));
    credentials.SetSessionToken(ToAwsString(MOCK_SESSION_TOKEN));
    assumeRoleResult.SetCredentials(credentials);
    auto outcome = Aws::STS::Model::AssumeRoleOutcome(assumeRoleResult);
    std::shared_ptr<GameKit::Mocks::MockSTSClient> mockStsClient = std::make_shared<GameKit::Mocks::MockSTSClient>();

    EXPECT_CALL(*mockStsClient, AssumeRole(_))
        .Times(1)
        .WillOnce(Return(outcome));

    const char* ids[] = { "first_id", "second_id" };
    GameKit::Achievements::AdminAchievements* achievementsInstance = static_cast<GameKit::Achievements::AdminAchievements*>(achievementsExportInstance);
    achievementsInstance->SetSTSClient(mockStsClient);

    // act
    unsigned int result = GameKitAdminDeleteAchievements(achievementsExportInstance, ids, 2);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);

    GameKitAdminAchievementsInstanceRelease(achievementsExportInstance);
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&mockStsClient));
}

TEST_F(GameKitAdminAchievementsExportsTestFixture, TestGameKitAchievementIdValid_Success)
{
    // arrange
    void* achievementsExportInstance = createAchievementsInstance();
    bool pass = true;
    std::vector<std::string> testCases = {"abc", "ABC", "123", "a1B2", "a_b", "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb", "a__________________b"};

    for (std::string testString : testCases)
    {
        pass &= GameKitIsAchievementIdValid(testString.c_str());
    }

    ASSERT_TRUE(pass);
}

TEST_F(GameKitAdminAchievementsExportsTestFixture, TestGameKitAchievementIdInValid_Fails)
{
    // arrange
    void* achievementsExportInstance = createAchievementsInstance();
    bool pass = true;
    std::vector<std::string> testCases = { "a", "A", "1", "_", "_abc", "abc_", "Aa&Bb", ""};

    for (std::string testString : testCases)
    {
        pass &= !GameKitIsAchievementIdValid(testString.c_str());
    }

    ASSERT_TRUE(pass);
}

TEST_F(GameKitAdminAchievementsExportsTestFixture, TestGameKitAchievementsAdminCredentialsChanged_Success)
{
    // arrange
    void* achievementsExportInstance = createAchievementsInstance();

    GameKit::AccountCredentials newCreds = {
        "us-west-2",
        "987654321098",
        "NEWACCESSSECRET",
        "NEWACCOUNTID"
    };

    GameKit::AccountInfo newInfo = {
        "qa",
        "987654321098",
        "newcompany",
        "newgame"
    };

    // act
    auto result = GameKitAdminCredentialsChanged(achievementsExportInstance, newCreds, newInfo);

    // assert
    GameKit::Achievements::AdminAchievements* achievementsInstance = static_cast<GameKit::Achievements::AdminAchievements*>(achievementsExportInstance);
    GameKit::AccountCredentialsCopy returnedCreds = achievementsInstance->GetAccountCredentials();
    GameKit::AccountInfoCopy returnedInfo = achievementsInstance->GetAccountInfo();

    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);

    ASSERT_STREQ(newCreds.accessKey, returnedCreds.accessKey.c_str());
    ASSERT_STREQ(newCreds.accessSecret, returnedCreds.accessSecret.c_str());
    ASSERT_STREQ(newCreds.region, returnedCreds.region.c_str());

    ASSERT_STREQ(newInfo.accountId, returnedInfo.accountId.c_str());
    ASSERT_STREQ(newInfo.companyName, returnedInfo.companyName.c_str());
    ASSERT_STREQ(newInfo.gameName, returnedInfo.gameName.c_str());
    ASSERT_STREQ(newInfo.environment, returnedInfo.environment.GetEnvironmentString().c_str());

    GameKitAdminAchievementsInstanceRelease(achievementsExportInstance);
}
