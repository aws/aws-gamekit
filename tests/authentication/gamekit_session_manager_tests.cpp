#include "gamekit_session_manager_tests.h"
#include "../core/mocks/mock_cognito_client.h"
#include "../core/test_stack.h"
#include "../core/test_log.h"

#include <aws/cognito-idp/CognitoIdentityProviderClient.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class GameKit::Tests::GameKitSessionManager::GameKitSessionManagerTestFixture : public ::testing::Test
{
protected:
    TestStackInitializer testStack;
    typedef TestLog<GameKitSessionManagerTestFixture> TestLogger;

public:
    GameKitSessionManagerTestFixture()
    {}

    ~GameKitSessionManagerTestFixture()
    {}

    void SetUp()
    {
        TestLogger::Clear();
        testStack.Initialize();

        gamekitSessionManagerInstance = Aws::MakeUnique<GameKit::Authentication::GameKitSessionManager>("sessionManager", "../core/test_data/sampleplugin/instance/testgame/dev/awsGameKitClientConfig.yml", TestLogger::Log);
    }

    void TearDown()
    {
        testStack.Cleanup();
    }
};

using namespace GameKit::Tests::GameKitSessionManager;
using namespace Aws::CognitoIdentityProvider::Model;
using namespace testing;
TEST_F(GameKitSessionManagerTestFixture, KeyDoesNotExist_TestAddKey_Success)
{
    // act
    gamekitSessionManagerInstance->SetToken(GameKit::TokenType::AccessToken, "abc");
    auto token = gamekitSessionManagerInstance->GetToken(GameKit::TokenType::AccessToken);

    // assert
    ASSERT_EQ("abc", token);
}

TEST_F(GameKitSessionManagerTestFixture, KeyExists_TestAddKey_Success)
{
    // act
    gamekitSessionManagerInstance->SetToken(GameKit::TokenType::AccessToken, "abc");
    gamekitSessionManagerInstance->SetToken(GameKit::TokenType::AccessToken, "xyz");

    auto token = gamekitSessionManagerInstance->GetToken(GameKit::TokenType::AccessToken);

    // assert
    ASSERT_EQ("xyz", token);
}

TEST_F(GameKitSessionManagerTestFixture, No_RefreshToken_Abort_Success)
{
    // arrange
    gamekitSessionManagerInstance->SetToken(GameKit::TokenType::RefreshToken, "");
    auto cognitoMock = Aws::MakeShared<GameKit::Mocks::MockCognitoIdentityProviderClient>("cognitoMock");
    gamekitSessionManagerInstance->SetCognitoClient(cognitoMock.get());

    EXPECT_CALL(*cognitoMock.get(), InitiateAuth(_))
        .Times(0);

    // act
    gamekitSessionManagerInstance->SetSessionExpiration(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(cognitoMock.get()));
}
