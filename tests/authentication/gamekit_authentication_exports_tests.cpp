#include "gamekit_authentication_exports_tests.h"
#include "../core/test_stack.h"
#include "../core/test_log.h"

class GameKit::Tests::GameKitSessionManager::GameKitAuthenticationExportsTestFixture : public ::testing::Test
{
protected:
    TestStackInitializer testStackInitializer;
    typedef TestLog<GameKitAuthenticationExportsTestFixture> TestLogger;

    void* createSessionManagerInstance()
    {
        return GameKitSessionManagerInstanceCreate("../core/test_data/sampleplugin/instance/testgame/dev/awsGameKitClientConfig.yml", TestLogger::Log);
    }

public:
    GameKitAuthenticationExportsTestFixture()
    {
    }

    ~GameKitAuthenticationExportsTestFixture()
    {   
    }

    void SetUp()
    {
        TestLogger::Clear();
        testStackInitializer.Initialize();
    }

    void TearDown()
    {
        testStackInitializer.Cleanup();
    }
};
using namespace GameKit::Tests::GameKitSessionManager;

TEST_F(GameKitAuthenticationExportsTestFixture, GameKitSessionManagerInstanceCreate_Success)
{
    // act
    GameKit::Authentication::GameKitSessionManager* sessionInstance = (GameKit::Authentication::GameKitSessionManager*)createSessionManagerInstance();

    // assert
    ASSERT_NE(sessionInstance, nullptr);

    delete(sessionInstance);
}

TEST_F(GameKitAuthenticationExportsTestFixture, SettingsLoaded_ReadSettings_Success)
{
    // arrange
    auto const sessionManagerHandle = createSessionManagerInstance();

    // act
    const bool result = GameKitSessionManagerAreSettingsLoaded(sessionManagerHandle, GameKit::FeatureType::Identity);

    // assert
    ASSERT_TRUE(result);

    GameKitSessionManagerInstanceRelease(sessionManagerHandle);
}   

TEST_F(GameKitAuthenticationExportsTestFixture, SettingsLoaded_ReloadSettings_Success)
{
    // arrange
    auto const sessionManagerHandle = createSessionManagerInstance();
    auto const sessionManager = (GameKit::Authentication::GameKitSessionManager*)sessionManagerHandle;
    
    ASSERT_EQ("Test", sessionManager->GetClientSettings().at(GameKit::ClientSettings::Authentication::SETTINGS_USER_POOL_CLIENT_ID));
    ASSERT_EQ("TestUrl", sessionManager->GetClientSettings().at(GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_API_GATEWAY_BASE_URL));
    ASSERT_EQ("TestRegion", sessionManager->GetClientSettings().at(GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_REGION));

    // act
    GameKitSessionManagerReloadConfigFile(sessionManagerHandle, "../core/test_data/sampleplugin/alternativeInstance/testgame/dev/awsGameKitClientConfig.yml");

    // assert
    ASSERT_EQ("TestClientID", sessionManager->GetClientSettings().at(GameKit::ClientSettings::Authentication::SETTINGS_USER_POOL_CLIENT_ID));
    ASSERT_EQ("TestGatewayURL", sessionManager->GetClientSettings().at(GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_API_GATEWAY_BASE_URL));
    ASSERT_EQ("us-west-3", sessionManager->GetClientSettings().at(GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_REGION));

    GameKitSessionManagerInstanceRelease(sessionManagerHandle);
}


TEST_F(GameKitAuthenticationExportsTestFixture, SettingsLoaded_ReloadSettings_Empty_Path_Clears)
{
    // arrange
    auto const sessionManagerHandle = createSessionManagerInstance();
    auto const sessionManager = (GameKit::Authentication::GameKitSessionManager*)sessionManagerHandle;

    // act
    GameKitSessionManagerReloadConfigFile(sessionManagerHandle, "../core/test_data/sampleplugin/alternativeInstance/testgame/dev/awsGameKitClientConfig.yml");
    GameKitSessionManagerReloadConfigFile(sessionManagerHandle, "");

    // assert
    ASSERT_EQ(0, sessionManager->GetClientSettings().count(GameKit::ClientSettings::Authentication::SETTINGS_USER_POOL_CLIENT_ID));
    ASSERT_EQ(0, sessionManager->GetClientSettings().count(GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_API_GATEWAY_BASE_URL));
    ASSERT_EQ(0, sessionManager->GetClientSettings().count(GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_REGION));
}

TEST_F(GameKitAuthenticationExportsTestFixture, SettingsLoaded_ReloadSettings_From_File_Contents)
{
    // arrange
    auto const sessionManagerHandle = createSessionManagerInstance();
    auto const sessionManager = (GameKit::Authentication::GameKitSessionManager*)sessionManagerHandle;

    // act
    GameKitSessionManagerReloadConfigContents(sessionManagerHandle, "user_pool_client_id: TestClientID\nidentity_api_gateway_base_url: TestGatewayURL\nidentity_region : us-west-3\n");

    // assert
    ASSERT_EQ("TestClientID", sessionManager->GetClientSettings().at(GameKit::ClientSettings::Authentication::SETTINGS_USER_POOL_CLIENT_ID));
    ASSERT_EQ("TestGatewayURL", sessionManager->GetClientSettings().at(GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_API_GATEWAY_BASE_URL));
    ASSERT_EQ("us-west-3", sessionManager->GetClientSettings().at(GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_REGION));
}

TEST_F(GameKitAuthenticationExportsTestFixture, SetToken)
{
    // arrange
    auto const sessionManagerHandle = createSessionManagerInstance();
    auto const sessionManager = (GameKit::Authentication::GameKitSessionManager*)sessionManagerHandle;

    // act
    GameKitSessionManagerReloadConfigContents(sessionManagerHandle, "user_pool_client_id: TestClientID\nidentity_api_gateway_base_url: TestGatewayURL\nidentity_region : us-west-3\n");

    // assert
    ASSERT_EQ("TestClientID", sessionManager->GetClientSettings().at(GameKit::ClientSettings::Authentication::SETTINGS_USER_POOL_CLIENT_ID));
    ASSERT_EQ("TestGatewayURL", sessionManager->GetClientSettings().at(GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_API_GATEWAY_BASE_URL));
    ASSERT_EQ("us-west-3", sessionManager->GetClientSettings().at(GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_REGION));
}

TEST_F(GameKitAuthenticationExportsTestFixture, KeyDoesNotExist_SetToken_Success)
{
    // arrange
    auto const sessionManagerHandle = createSessionManagerInstance();
    auto const sessionManager = (GameKit::Authentication::GameKitSessionManager*)sessionManagerHandle;

    // act
    sessionManager->SetToken(GameKit::TokenType::AccessToken, "abc");
    auto token = sessionManager->GetToken(GameKit::TokenType::AccessToken);

    // assert
    ASSERT_EQ("abc", token);
}

TEST_F(GameKitAuthenticationExportsTestFixture, KeyExists_SetToken_Success)
{
    // arrange
    auto const sessionManagerHandle = createSessionManagerInstance();
    auto const sessionManager = (GameKit::Authentication::GameKitSessionManager*)sessionManagerHandle;

    // act
    sessionManager->SetToken(GameKit::TokenType::AccessToken, "abc");
    sessionManager->SetToken(GameKit::TokenType::AccessToken, "xyz");

    auto token = sessionManager->GetToken(GameKit::TokenType::AccessToken);

    // assert
    ASSERT_EQ("xyz", token);
}

TEST_F(GameKitAuthenticationExportsTestFixture, TestGameKitIdentityInstanceRelease_Success)
{
    // arrange
    auto const sessionManagerHandle = createSessionManagerInstance();;

    // act
    GameKitSessionManagerInstanceRelease(sessionManagerHandle);
}
