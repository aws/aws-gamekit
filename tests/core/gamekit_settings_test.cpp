// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "gamekit_settings_test.h"
#include "test_log.h"

class GameKit::Tests::GameKitSettings::GameKitSettingsTestFixture : public ::testing::Test
{
protected:
    typedef TestLog<GameKitSettingsTestFixture> TestLogger;

public:
    GameKitSettingsTestFixture()
    {}

    ~GameKitSettingsTestFixture()
    {}

    void SetUp()
    {
        TestLogger::Clear();
        gamekitSettingsInstance = std::make_unique<GameKit::GameKitSettings>("../core/test_data/sampleplugin/instance", "1.0.0", "testgame", "dev", TestLogger::Log);
    }

    void TearDown()
    {
        // cleanup
        auto path = gamekitSettingsInstance->GetSettingsFilePath();
        boost::filesystem::remove(gamekitSettingsInstance->GetSettingsFilePath());

        gamekitSettingsInstance.reset();
    }
};

using namespace GameKit::Tests::GameKitSettings;
TEST_F(GameKitSettingsTestFixture, SetAndGetGameInfo_GameNameSet)
{
    // act
    gamekitSettingsInstance->SetGameName("This is a sample game");
    gamekitSettingsInstance->SaveSettings();
    gamekitSettingsInstance->Reload();
    auto gameDesc = gamekitSettingsInstance->GetGameName();

    // assert
    ASSERT_EQ(gameDesc, "This is a sample game");
}

TEST_F(GameKitSettingsTestFixture, AddAndGetEnvironment_CustomEnvironmentSet)
{
    // act
    gamekitSettingsInstance->AddCustomEnvironment("cd1", "Custom Env 1");
    gamekitSettingsInstance->SaveSettings();
    gamekitSettingsInstance->Reload();
    auto envDesc = gamekitSettingsInstance->GetCustomEnvironmentDescription("cd1");

    // assert
    ASSERT_EQ(envDesc, "Custom Env 1");
}

TEST_F(GameKitSettingsTestFixture, DeleteAndGetEnvironment_CustomEnvironmentDeleted)
{
    // arrange: add custom environment first
    gamekitSettingsInstance->AddCustomEnvironment("cd1", "Custom Env 1");
    gamekitSettingsInstance->SaveSettings();

    // act
    gamekitSettingsInstance->DeleteCustomEnvironment("cd1");
    gamekitSettingsInstance->SaveSettings();
    gamekitSettingsInstance->Reload();
    auto envDesc = gamekitSettingsInstance->GetCustomEnvironmentDescription("cd1");

    // assert
    ASSERT_TRUE(envDesc.empty());
}

TEST_F(GameKitSettingsTestFixture, GetEnvironments_ReturnAll)
{
    // arrange: add custom environment first
    gamekitSettingsInstance->AddCustomEnvironment("cd1", "Custom Env 1");
    gamekitSettingsInstance->AddCustomEnvironment("cd2", "Custom Env 2");
    gamekitSettingsInstance->SaveSettings();

    // act
    auto envs = gamekitSettingsInstance->GetCustomEnvironments();

    // assert
    ASSERT_EQ(envs.size(), 2);
    ASSERT_EQ(envs["cd1"], "Custom Env 1");
    ASSERT_EQ(envs["cd2"], "Custom Env 2");
}

TEST_F(GameKitSettingsTestFixture, ActivateAndGetFeatureStatus_FeatureActivated)
{
    // act
    gamekitSettingsInstance->ActivateFeature(GameKit::FeatureType::Identity);
    gamekitSettingsInstance->SaveSettings();
    auto status = gamekitSettingsInstance->IsFeatureActive(GameKit::FeatureType::Identity);

    // assert
    ASSERT_TRUE(status);
}

TEST_F(GameKitSettingsTestFixture, DeactivateAndGetFeatureStatus_FeatureDeactivated)
{
    // arrange: set feature to active first
    gamekitSettingsInstance->ActivateFeature(GameKit::FeatureType::Identity);
    gamekitSettingsInstance->SaveSettings();

    // act
    gamekitSettingsInstance->DeactivateFeature(GameKit::FeatureType::Identity);
    gamekitSettingsInstance->SaveSettings();
    gamekitSettingsInstance->Reload();
    auto status = gamekitSettingsInstance->IsFeatureActive(GameKit::FeatureType::Identity);

    // assert
    ASSERT_FALSE(status);
}

TEST_F(GameKitSettingsTestFixture, SetAndGetFeatureVar_FeatureVarSet)
{
    // act
    std::map<std::string, std::string> vars = { { "a", "apple" } };
    gamekitSettingsInstance->SetFeatureVariables(GameKit::FeatureType::Identity, vars);
    gamekitSettingsInstance->SaveSettings();
    auto val = gamekitSettingsInstance->GetFeatureVariable(GameKit::FeatureType::Identity, "a");

    // assert
    ASSERT_EQ(val, "apple");
}

TEST_F(GameKitSettingsTestFixture, DeleteAndGetFeatureVar_FeatureVarDeleted)
{
    // arrange: set feature var first
    std::map<std::string, std::string> vars = { { "a", "apple" } };
    gamekitSettingsInstance->SetFeatureVariables(GameKit::FeatureType::Identity, vars);
    gamekitSettingsInstance->SaveSettings();

    // act
    gamekitSettingsInstance->DeleteFeatureVariable(GameKit::FeatureType::Identity, "a");
    gamekitSettingsInstance->SaveSettings();
    gamekitSettingsInstance->Reload();
    auto val = gamekitSettingsInstance->GetFeatureVariable(GameKit::FeatureType::Identity, "a");

    // assert
    ASSERT_TRUE(val.empty());
}

TEST_F(GameKitSettingsTestFixture, GetFeatureVars_ReturnAll)
{
    // arrange: set feature var first
    std::map<std::string, std::string> vars = { { "a", "apple" }, {"b", "banana"} };
    gamekitSettingsInstance->SetFeatureVariables(GameKit::FeatureType::Identity, vars);
    gamekitSettingsInstance->SaveSettings();

    // act
    auto featureVars = gamekitSettingsInstance->GetFeatureVariables(GameKit::FeatureType::Identity);

    // assert
    ASSERT_EQ(featureVars.size(), 2);
    ASSERT_EQ(featureVars["a"], "apple");
    ASSERT_EQ(featureVars["b"], "banana");
}


TEST_F(GameKitSettingsTestFixture, GetFeatureVars_ReturnNone)
{
    // arrange: set Identity feature vars first
    std::map<std::string, std::string> vars = { { "a", "apple" }, {"b", "banana"} };
    gamekitSettingsInstance->SetFeatureVariables(GameKit::FeatureType::Identity, vars);
    gamekitSettingsInstance->SaveSettings();

    // act: get GameSaving feature vars
    auto featureVars = gamekitSettingsInstance->GetFeatureVariables(GameKit::FeatureType::GameStateCloudSaving);

    // assert: vars are empty
    ASSERT_EQ(featureVars.size(), 0);
}

TEST_F(GameKitSettingsTestFixture, SaveSettings_ValidateWarningExists)
{
    // act
    gamekitSettingsInstance->SaveSettings();

    // assert
    std::string saveInfo;
    GameKit::Utils::FileUtils::ReadFileIntoString(gamekitSettingsInstance->GetSettingsFilePath(), saveInfo);

    ASSERT_THAT(saveInfo, testing::StartsWith(GameKit::Configuration::DO_NOT_EDIT));
}