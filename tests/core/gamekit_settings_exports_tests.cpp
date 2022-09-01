// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>
#include <aws/gamekit/core/utils/file_utils.h>

#include "../core/dispatchers.h"
#include "gamekit_settings_exports_tests.h"
#include "test_stack.h"
#include "test_log.h"
#include <boost/filesystem.hpp>

using namespace GameKit::Tests;
using namespace ::testing;

const std::string TEST_CREDENTIALS_FILE_LOCATION = "../core/test_data/testFiles/credentialsTests/test_credentials";

class GameKit::Tests::GameKitSettingsExport::GameKitSettingsExportTestFixture : public ::testing::Test
{
protected:
    TestStackInitializer testStack;
    typedef TestLog<GameKitSettingsExportTestFixture> TestLogger;

public:
    GameKitSettingsExportTestFixture()
    {}

    ~GameKitSettingsExportTestFixture()
    {}

    void SetUp()
    {
        // In case a previous test crashed we must clear the file before each test
        std::ofstream ofs;
        ofs.open(TEST_CREDENTIALS_FILE_LOCATION, std::ofstream::out | std::ofstream::trunc);
        ofs.close();

        TestLogger::Clear();
        testStack.Initialize();
    }

    void TearDown()
    {
        GameKitSettings* instance = (GameKit::GameKitSettings*)createSettingsInstance();

        // To avoid changes showing up in git, we should clear this file after each test. This can not be ignored though since the directory must exist
        std::ofstream ofs;
        ofs.open(TEST_CREDENTIALS_FILE_LOCATION, std::ofstream::out | std::ofstream::trunc);
        ofs.close();

        remove(instance->GetSettingsFilePath().c_str());
        testStack.Cleanup();
    }

    void* createSettingsInstance()
    {
        return GameKitSettingsInstanceCreate("../core/test_data/sampleplugin/instance", "1.0.0", "testgame", "dev", TestLogger::Log);
    }

};

using namespace GameKit::Tests::GameKitSettingsExport;

class GameNameInfoReciever
{
public:
    std::string gameName;
    void OnReceiveGameNameInfo(const char* gameName)
    {
        this->gameName = gameName;
    }
};

class LastUsedRegionReceiver
{
public:
    std::string lastUsedRegion;
    void OnReceiveLastUsedRegion(const char* region)
    {
        this->lastUsedRegion = region;
    }
};

class LastUsedEnvInfoReceiver
{
public:
    std::string lastUsedEnv;
    void OnReceiveLastUsedEnvInfo(const char* env)
    {
        this->lastUsedEnv = env;
    }
};

class EnvDescriptionReciever
{
public:
    std::string envDescription;
    void OnReceiveEnvDescription(const char* envDescription)
    {
        this->envDescription = envDescription;
    }
};

class AllEnvsReciever
{
public:
    std::map<std::string, std::string> envs;
    void OnReceiveEnvironments(const char* charKey, const char* charValue)
    {
        this->envs.insert({ charKey, charValue });
    }
};

class FeatureVarReciever
{
public:
    std::string varValue;
    void OnReceiveFeatureVar(const char* varValue)
    {
        this->varValue = varValue;
    }
};

class AllVarsReciever
{
public:
    std::map<std::string, std::string> vars;
    void OnReceiveVariables(const char* charKey, const char* charValue)
    {
        this->vars.insert({ charKey, charValue });
    }
};


class SettingsDispatcher
{
public:
    static void GameInfoCallbackDispatcher(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* gameName)
    {
        ((GameNameInfoReciever*)dispatchReceiver)->OnReceiveGameNameInfo(gameName);
    }

    static void LastUsedRegionCallbackDispatcher(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* region)
    {
        ((LastUsedRegionReceiver*)dispatchReceiver)->OnReceiveLastUsedRegion(region);
    }

    static void LastUsedEnvInfoCallbackDispatcher(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* gameName)
    {
        ((LastUsedEnvInfoReceiver*)dispatchReceiver)->OnReceiveLastUsedEnvInfo(gameName);
    }

    static void EnvDescriptionCallbackDispatcher(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* envName)
    {
        ((GameNameInfoReciever*)dispatchReceiver)->OnReceiveGameNameInfo(envName);
    }

    static void AllEnvsCallbackDispatcher(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* charKey, const char* charValue)
    {
        ((AllEnvsReciever*)dispatchReceiver)->OnReceiveEnvironments(charKey, charValue);
    }

    static void FeatureVarCallbackDispatcher(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* varValue)
    {
        ((FeatureVarReciever*)dispatchReceiver)->OnReceiveFeatureVar(varValue);
    }

    static void AllVarsCallbackDispatcher(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* charKey, const char* charValue)
    {
        ((AllEnvsReciever*)dispatchReceiver)->OnReceiveEnvironments(charKey, charValue);
    }
};

TEST_F(GameKitSettingsExportTestFixture, TestGameKitSettingsInstanceCreate_Success)
{
    // act
    const GameKit::GameKitSettings* settingsInstance = (GameKit::GameKitSettings*)createSettingsInstance();

    // assert
    ASSERT_NE(settingsInstance, nullptr);

    delete(settingsInstance);
}

TEST_F(GameKitSettingsExportTestFixture, TestGameKitSettingsInstanceRelease_Success)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();

    // act
    GameKitSettingsInstanceRelease(settingsInstance);
}

TEST_F(GameKitSettingsExportTestFixture, TestGameKitSettingsInstanceCreateWithNoAWSFolder_SuccessAndFolderCreated)
{
    // arrange
    remove(TEST_CREDENTIALS_FILE_LOCATION.c_str());

    // act
    const GameKit::GameKitSettings* settingsInstance = (GameKit::GameKitSettings*)createSettingsInstance();

    // assert
    ASSERT_NE(settingsInstance, nullptr);
    ASSERT_TRUE(boost::filesystem::exists(TEST_CREDENTIALS_FILE_LOCATION));

    delete(settingsInstance);
}

TEST_F(GameKitSettingsExportTestFixture, TestGameKitSettingsSetAndGetGameInfo_Success)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();

    // act
    GameKitSettingsSetGameName(settingsInstance, "This is a sample game");
    GameKitSettingsSave(settingsInstance);
    GameKitSettingsReload(settingsInstance);
    GameNameInfoReciever* const receiver = new GameNameInfoReciever();
    GameKitSettingsGetGameName(settingsInstance, receiver, SettingsDispatcher::GameInfoCallbackDispatcher);

    // assert
    ASSERT_STREQ(receiver->gameName.c_str(), "This is a sample game");

    // cleanup
    GameKitSettingsInstanceRelease(settingsInstance);
    delete(receiver);
}

TEST_F(GameKitSettingsExportTestFixture, TestGameKitSettingsGetMissingLastUsedEnvInfo_Success)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();

    // act
    LastUsedEnvInfoReceiver* const receiver = new LastUsedEnvInfoReceiver();
    GameKitSettingsGetLastUsedEnvironment(settingsInstance, receiver, SettingsDispatcher::LastUsedEnvInfoCallbackDispatcher);

    // assert
    ASSERT_STREQ(receiver->lastUsedEnv.c_str(), "dev");

    // cleanup
    GameKitSettingsInstanceRelease(settingsInstance);
    delete(receiver);
}

TEST_F(GameKitSettingsExportTestFixture, TestGameKitSettingsSetAndGetLastUsedEnvInfo_Success)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();

    // act
    GameKitSettingsSetLastUsedEnvironment(settingsInstance, "bec");
    GameKitSettingsSave(settingsInstance);
    GameKitSettingsReload(settingsInstance);
    LastUsedEnvInfoReceiver* const receiver = new LastUsedEnvInfoReceiver();
    GameKitSettingsGetLastUsedEnvironment(settingsInstance, receiver, SettingsDispatcher::LastUsedEnvInfoCallbackDispatcher);

    // assert
    ASSERT_STREQ(receiver->lastUsedEnv.c_str(), "bec");

    // cleanup
    GameKitSettingsInstanceRelease(settingsInstance);
    delete(receiver);
}

TEST_F(GameKitSettingsExportTestFixture, TestGameKitSettingsGetMissingLastUsedRegion_Success)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();

    // act
    LastUsedRegionReceiver* const receiver = new LastUsedRegionReceiver();
    GameKitSettingsGetLastUsedRegion(settingsInstance, receiver, SettingsDispatcher::LastUsedRegionCallbackDispatcher);

    // assert
    ASSERT_STREQ(receiver->lastUsedRegion.c_str(), "us-east-1");

    // cleanup
    GameKitSettingsInstanceRelease(settingsInstance);
    delete(receiver);
}
TEST_F(GameKitSettingsExportTestFixture, TestGameKitSettingsSetAndGetLastUsedRegion)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();

    // act
    GameKitSettingsSetLastUsedRegion(settingsInstance, "us-west-2");
    GameKitSettingsSave(settingsInstance);
    GameKitSettingsReload(settingsInstance);
    LastUsedRegionReceiver* const receiver = new LastUsedRegionReceiver();
    GameKitSettingsGetLastUsedRegion(settingsInstance, receiver, SettingsDispatcher::LastUsedRegionCallbackDispatcher);

    // assert
    ASSERT_STREQ(receiver->lastUsedRegion.c_str(), "us-west-2");

    // cleanup
    GameKitSettingsInstanceRelease(settingsInstance);
    delete(receiver);
}

TEST_F(GameKitSettingsExportTestFixture, TestGameKitSettingsPopulateAndSave_Success)
{
    // arrange
    const std::string GAME_NAME = "test game name";
    const std::string ENV = "tst";
    const std::string REGION = "test region";

    GameNameInfoReciever* const gameNameReceiver = new GameNameInfoReciever();
    LastUsedEnvInfoReceiver* const envReceiver = new LastUsedEnvInfoReceiver();
    LastUsedRegionReceiver* const regionReceiver = new LastUsedRegionReceiver();
    
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();

    // act
    unsigned int result = GameKitSettingsPopulateAndSave(settingsInstance, GAME_NAME.c_str(), ENV.c_str(), REGION.c_str());
   
    GameKitSettingsReload(settingsInstance);
    GameKitSettingsGetGameName(settingsInstance, gameNameReceiver, SettingsDispatcher::GameInfoCallbackDispatcher);
    GameKitSettingsGetLastUsedEnvironment(settingsInstance, envReceiver, SettingsDispatcher::LastUsedEnvInfoCallbackDispatcher);
    GameKitSettingsGetLastUsedRegion(settingsInstance, regionReceiver, SettingsDispatcher::LastUsedRegionCallbackDispatcher);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    ASSERT_STREQ(gameNameReceiver->gameName.c_str(), GAME_NAME.c_str());
    ASSERT_STREQ(envReceiver->lastUsedEnv.c_str(), ENV.c_str());
    ASSERT_STREQ(regionReceiver->lastUsedRegion.c_str(), REGION.c_str());

    // cleanup
    delete(regionReceiver);
    delete(envReceiver);
    delete(gameNameReceiver);
    GameKitSettingsInstanceRelease(settingsInstance);
}

TEST_F(GameKitSettingsExportTestFixture, AddAndGetEnvironment_CustomEnvironmentSet)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();

    // act
    GameKitSettingsAddCustomEnvironment(settingsInstance, "cd1", "Custom Env 1");
    GameKitSettingsSave(settingsInstance);
    GameKitSettingsReload(settingsInstance);
    EnvDescriptionReciever* const receiver = new EnvDescriptionReciever();
    GameKitSettingsGetCustomEnvironmentDescription(settingsInstance, receiver, "cd1", SettingsDispatcher::EnvDescriptionCallbackDispatcher);

    // assert
    ASSERT_STREQ(receiver->envDescription.c_str(), "Custom Env 1");

    // cleanup
    GameKitSettingsInstanceRelease(settingsInstance);
    delete(receiver);
}

TEST_F(GameKitSettingsExportTestFixture, DeleteAndGetEnvironment_CustomEnvironmentSet)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();
    GameKitSettingsAddCustomEnvironment(settingsInstance, "cd1", "Custom Env 1");
    GameKitSettingsSave(settingsInstance);

    // act
    GameKitSettingsDeleteCustomEnvironment(settingsInstance, "cd1");
    GameKitSettingsSave(settingsInstance);
    GameKitSettingsReload(settingsInstance);
    EnvDescriptionReciever* const receiver = new EnvDescriptionReciever();
    GameKitSettingsGetCustomEnvironmentDescription(settingsInstance, receiver, "cd1", SettingsDispatcher::EnvDescriptionCallbackDispatcher);

    // assert
    ASSERT_TRUE(receiver->envDescription.empty());

    // cleanup
    GameKitSettingsInstanceRelease(settingsInstance);
    delete(receiver);
}

TEST_F(GameKitSettingsExportTestFixture, GetEnvironments_ReturnAll)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();
    GameKitSettingsAddCustomEnvironment(settingsInstance, "cd1", "Custom Env 1");
    GameKitSettingsAddCustomEnvironment(settingsInstance, "cd2", "Custom Env 2");
    GameKitSettingsSave(settingsInstance);

    // act
    AllEnvsReciever* const receiver = new AllEnvsReciever();
    GameKitSettingsGetCustomEnvironments(settingsInstance, receiver, SettingsDispatcher::AllEnvsCallbackDispatcher);

    // assert
    ASSERT_EQ(receiver->envs.size(), 2);
    ASSERT_STREQ(receiver->envs["cd1"].c_str(), "Custom Env 1");
    ASSERT_STREQ(receiver->envs["cd2"].c_str(), "Custom Env 2");

    // cleanup
    GameKitSettingsInstanceRelease(settingsInstance);
    delete(receiver);
}

TEST_F(GameKitSettingsExportTestFixture, ActivateAndGetFeatureStatus_FeatureActivated)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();
    GameKitSettingsActivateFeature(settingsInstance, GameKit::FeatureType::Identity);
    GameKitSettingsSave(settingsInstance);

    // act
    bool const status = GameKitSettingsIsFeatureActive(settingsInstance, GameKit::FeatureType::Identity);

    // assert
    ASSERT_TRUE(status);

    // cleanup
    GameKitSettingsInstanceRelease(settingsInstance);
}

TEST_F(GameKitSettingsExportTestFixture, DeactivateAndGetFeatureStatus_FeatureDeactivated)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();
    GameKitSettingsActivateFeature(settingsInstance, GameKit::FeatureType::Identity);
    GameKitSettingsSave(settingsInstance);

    // act
    GameKitSettingsDeactivateFeature(settingsInstance, GameKit::FeatureType::Identity);
    GameKitSettingsSave(settingsInstance);
    GameKitSettingsReload(settingsInstance);
    bool const status = GameKitSettingsIsFeatureActive(settingsInstance, GameKit::FeatureType::Identity);

    // assert
    ASSERT_FALSE(status);

    // cleanup
    GameKitSettingsInstanceRelease(settingsInstance);
}

TEST_F(GameKitSettingsExportTestFixture, SetAndGetFeatureVar_FeatureVarSet)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();

    // act
    const char* varKeys[1] = { "a" };
    const char* varValues[1] = { "apple" };
    GameKitSettingsSetFeatureVariables(settingsInstance, GameKit::FeatureType::Identity, varKeys, varValues, 1);
    GameKitSettingsSave(settingsInstance);
    FeatureVarReciever* const receiver = new FeatureVarReciever();
    GameKitSettingsGetFeatureVariable(settingsInstance, receiver, GameKit::FeatureType::Identity, "a", SettingsDispatcher::FeatureVarCallbackDispatcher);

    // assert
    ASSERT_STREQ(receiver->varValue.c_str(), "apple");

    // cleanup
    GameKitSettingsInstanceRelease(settingsInstance);
    delete(receiver);
}

TEST_F(GameKitSettingsExportTestFixture, KeyNotPresent_GetFeatureVar_ReturnEmpty)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();

    // act
    FeatureVarReciever* const receiver = new FeatureVarReciever();
    GameKitSettingsGetFeatureVariable(settingsInstance, receiver, GameKit::FeatureType::Identity, "a", SettingsDispatcher::FeatureVarCallbackDispatcher);

    // assert
    ASSERT_TRUE(receiver->varValue.empty());

    // cleanup
    GameKitSettingsInstanceRelease(settingsInstance);
}

TEST_F(GameKitSettingsExportTestFixture, DeleteAndGetFeatureVar_FeatureVarDeleted)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();
    const char* varKeys[1] = { "a" };
    const char* varValues[1] = { "apple" };
    GameKitSettingsSetFeatureVariables(settingsInstance, GameKit::FeatureType::Identity, varKeys, varValues, 1);
    GameKitSettingsSave(settingsInstance);

    // act
    GameKitSettingsDeleteFeatureVariable(settingsInstance, GameKit::FeatureType::Identity, "a");
    GameKitSettingsSave(settingsInstance);
    GameKitSettingsReload(settingsInstance);
    FeatureVarReciever* const receiver = new FeatureVarReciever();
    GameKitSettingsGetFeatureVariable(settingsInstance, receiver, GameKit::FeatureType::Identity, "a", SettingsDispatcher::FeatureVarCallbackDispatcher);

    // assert
    ASSERT_TRUE(receiver->varValue.empty());

    // cleanup
    GameKitSettingsInstanceRelease(settingsInstance);
    delete(receiver);
}

TEST_F(GameKitSettingsExportTestFixture, GetFeatureVars_ReturnAll)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();
    const char* varKeys[2] = { "a", "b" };
    const char* varValues[2] = { "apple", "banana" };
    GameKitSettingsSetFeatureVariables(settingsInstance, GameKit::FeatureType::Identity, varKeys, varValues, 2);
    GameKitSettingsSave(settingsInstance);

    // act
    AllVarsReciever* const receiver = new AllVarsReciever();
    GameKitSettingsGetFeatureVariables(settingsInstance, receiver, GameKit::FeatureType::Identity, SettingsDispatcher::AllVarsCallbackDispatcher);

    // assert
    ASSERT_EQ(receiver->vars.size(), 2);
    ASSERT_STREQ(receiver->vars["a"].c_str(), "apple");
    ASSERT_STREQ(receiver->vars["b"].c_str(), "banana");

    // cleanup
    GameKitSettingsInstanceRelease(settingsInstance);
    delete(receiver);
}

TEST_F(GameKitSettingsExportTestFixture, KeyNotPresent_GetFeatureVars_ReturnEmpty)
{
    // arrange
    const GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance = createSettingsInstance();

    // act
    AllVarsReciever* const receiver = new AllVarsReciever();
    GameKitSettingsGetFeatureVariables(settingsInstance, receiver, GameKit::FeatureType::Identity, SettingsDispatcher::AllVarsCallbackDispatcher);

    // assert
    ASSERT_EQ(receiver->vars.size(), 0);

    // cleanup
    GameKitSettingsInstanceRelease(settingsInstance);
    delete(receiver);
}

TEST_F(GameKitSettingsExportTestFixture, FileEmpty_SaveNewAWSCredentials_ReturnSuccess)
{
    // act
    const unsigned int result = GameKitSaveAwsCredentials("GameKit-testgame", "AccessKey0406", "SecretKey0406", TestLogger::Log);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);
}

TEST_F(GameKitSettingsExportTestFixture, FileDoesNotExist_CreateCredentialsFile_ReturnSuccess)
{
    // arrange
    remove(TEST_CREDENTIALS_FILE_LOCATION.c_str());

    // act
    const unsigned int result = GameKitSaveAwsCredentials("GameKit-testgame", "AccessKey0406", "SecretKey0406", TestLogger::Log);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);
}

TEST_F(GameKitSettingsExportTestFixture, AWSCredentialsFileThatAlreadyExists_SaveNewAWSCredential_ReturnSuccess)
{
    // arrange
    std::ofstream output(TEST_CREDENTIALS_FILE_LOCATION);

    output << "[default]" << std::endl;
    output << "aws_access_key_id=DefaultAccessKey" << std::endl;
    output << "aws_secret_access_key=DefaultSecretKey" << std::endl;
    output << "#Comment \n\n" << std::endl;

    // act
    const unsigned int result = GameKitSaveAwsCredentials("GameKit-testgame", "AccessKey0406", "SecretKey0406", TestLogger::Log);


    // assert
    std::ifstream credentialsFile(TEST_CREDENTIALS_FILE_LOCATION);
    std::string verificationArray[7];

    if (credentialsFile.is_open())
    {
        for (int i = 0; i < 7; ++i)
        {
            credentialsFile >> verificationArray[i];
        }
    }

    ASSERT_STREQ(verificationArray[0].c_str(), "[GameKit-testgame]");
    ASSERT_STREQ(verificationArray[3].c_str(), "[default]");
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);
}

TEST_F(GameKitSettingsExportTestFixture, FileDoesNotExist_AwsProfileExists_ReturnFalse)
{
    // arrange
    remove(TEST_CREDENTIALS_FILE_LOCATION.c_str());

    // act
    const bool result = GameKitAwsProfileExists("GameKit-testgame");

    // assert
    ASSERT_EQ(result, false);
}

TEST_F(GameKitSettingsExportTestFixture, ProfileExists_AwsProfileExists_ReturnTrue)
{
    // arrange
    std::ofstream output(TEST_CREDENTIALS_FILE_LOCATION);

    output << "[default]" << std::endl;
    output << "aws_access_key_id=DefaultAccessKey" << std::endl;
    output << "aws_secret_access_key=DefaultSecretKey" << std::endl;

    // act
    const bool result = GameKitAwsProfileExists("default");

    // assert
    ASSERT_EQ(result, true);
}

TEST_F(GameKitSettingsExportTestFixture, ProfileDoesNotExists_AwsProfileExists_ReturnFalse)
{
    // arrange
    std::ofstream output(TEST_CREDENTIALS_FILE_LOCATION);

    output << "[default]" << std::endl;
    output << "aws_access_key_id=DefaultAccessKey" << std::endl;
    output << "aws_secret_access_key=DefaultSecretKey" << std::endl;

    // act
    const bool result = GameKitAwsProfileExists("jakesProfile");

    // assert
    ASSERT_EQ(result, false);
}

TEST_F(GameKitSettingsExportTestFixture, FileDoesNotExist_SetNewAccessKey_ReturnError)
{
    // arrange
    remove(TEST_CREDENTIALS_FILE_LOCATION.c_str());

    // act
    const unsigned int result = GameKitSetAwsAccessKey("GameKit-testgame", "NewAccessKey12", TestLogger::Log);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_CREDENTIALS_FILE_NOT_FOUND);
}

TEST_F(GameKitSettingsExportTestFixture, FileExists_SetNewAccessKey_ReturnSuccess)
{
    // arrange
    std::ofstream output(TEST_CREDENTIALS_FILE_LOCATION);

    output << "[default]" << std::endl;
    output << "aws_access_key_id=DefaultAccessKey" << std::endl;
    output << "aws_secret_access_key=DefaultSecretKey" << std::endl;

    // act
    const unsigned int result = GameKitSetAwsAccessKey("default", "AccessKey0406", TestLogger::Log);

    // assert
    std::ifstream credentialsFile(TEST_CREDENTIALS_FILE_LOCATION);
    std::string verificationArray[4];

    if (credentialsFile.is_open())
    {
        for (int i = 0; i < 4; ++i)
        {
            credentialsFile >> verificationArray[i];
        }
    }

    ASSERT_STREQ(verificationArray[0].c_str(), "[default]");
    ASSERT_STREQ(verificationArray[1].c_str(), "aws_access_key_id=AccessKey0406");
    ASSERT_STREQ(verificationArray[2].c_str(), "aws_secret_access_key=DefaultSecretKey");
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);
}

TEST_F(GameKitSettingsExportTestFixture, ProfileDoesNotExist_SetNewAccessKey_ReturnError)
{
    // arrange
    std::ofstream output(TEST_CREDENTIALS_FILE_LOCATION);

    output << "[default]" << std::endl;
    output << "aws_access_key_id=DefaultAccessKey" << std::endl;
    output << "aws_secret_access_key=DefaultSecretKey" << std::endl;

    // act
    const unsigned int result = GameKitSetAwsAccessKey("GameKit-testgame", "AccessKey0406", TestLogger::Log);

    // assert
    std::ifstream credentialsFile(TEST_CREDENTIALS_FILE_LOCATION);
    std::string verificationArray[4];

    if (credentialsFile.is_open())
    {
        for (int i = 0; i < 4; ++i)
        {
            credentialsFile >> verificationArray[i];
        }
    }

    ASSERT_STREQ(verificationArray[0].c_str(), "[default]");
    ASSERT_STREQ(verificationArray[1].c_str(), "aws_access_key_id=DefaultAccessKey");
    ASSERT_STREQ(verificationArray[2].c_str(), "aws_secret_access_key=DefaultSecretKey");
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_CREDENTIALS_NOT_FOUND);
}

TEST_F(GameKitSettingsExportTestFixture, FileDoesNotExist_SetNewSecret_ReturnError)
{
    // arrange
    remove(TEST_CREDENTIALS_FILE_LOCATION.c_str());

    // act
    const unsigned int result = GameKitSetAwsAccessKey("GameKit-testgame", "NewAccessKey12", TestLogger::Log);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_CREDENTIALS_FILE_NOT_FOUND);
}

TEST_F(GameKitSettingsExportTestFixture, FileExists_SetNewSecret_ReturnSuccess)
{
    // arrange
    std::ofstream output(TEST_CREDENTIALS_FILE_LOCATION);

    output << "[default]" << std::endl;
    output << "aws_access_key_id=DefaultAccessKey" << std::endl;
    output << "aws_secret_access_key=DefaultSecretKey" << std::endl;

    // act
    const unsigned int result = GameKitSetAwsSecretKey("default", "SecretKey0406", TestLogger::Log);

    // assert
    std::ifstream credentialsFile(TEST_CREDENTIALS_FILE_LOCATION);
    std::string verificationArray[4];

    if (credentialsFile.is_open())
    {
        for (int i = 0; i < 4; ++i)
        {
            credentialsFile >> verificationArray[i];
        }
    }

    ASSERT_STREQ(verificationArray[0].c_str(), "[default]");
    ASSERT_STREQ(verificationArray[1].c_str(), "aws_access_key_id=DefaultAccessKey");
    ASSERT_STREQ(verificationArray[2].c_str(), "aws_secret_access_key=SecretKey0406");
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);
}

TEST_F(GameKitSettingsExportTestFixture, ProfileDoesNotExist_SetNewSecret_ReturnError)
{
    // arrange
    std::ofstream output(TEST_CREDENTIALS_FILE_LOCATION);

    output << "[default]" << std::endl;
    output << "aws_access_key_id=DefaultAccessKey" << std::endl;
    output << "aws_secret_access_key=DefaultSecretKey" << std::endl;

    // act
    const unsigned int result = GameKitSetAwsSecretKey("GameKit-Credentials", "SecretKey0406", TestLogger::Log);

    // assert
    std::ifstream credentialsFile(TEST_CREDENTIALS_FILE_LOCATION);
    std::string verificationArray[4];

    if (credentialsFile.is_open())
    {
        for (int i = 0; i < 4; ++i)
        {
            credentialsFile >> verificationArray[i];
        }
    }

    ASSERT_STREQ(verificationArray[0].c_str(), "[default]");
    ASSERT_STREQ(verificationArray[1].c_str(), "aws_access_key_id=DefaultAccessKey");
    ASSERT_STREQ(verificationArray[2].c_str(), "aws_secret_access_key=DefaultSecretKey");
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_CREDENTIALS_NOT_FOUND);
}

TEST_F(GameKitSettingsExportTestFixture, FileExists_GetProfile_ReturnSuccess)
{
    // arrange
    std::ofstream output(TEST_CREDENTIALS_FILE_LOCATION);

    output << "[default]" << std::endl;
    output << "#Comment" << std::endl;
    output << "aws_access_key_id=DefaultAccessKey" << std::endl;
    output << "aws_secret_access_key=DefaultSecretKey" << std::endl;

    std::string retrievedAccessKey;
    std::string retrievedSecret;
    auto valueSetter = [&retrievedAccessKey, &retrievedSecret](const char* accessKey, const char* secret)
    {
        retrievedAccessKey = accessKey;
        retrievedSecret = secret;
    };
    typedef LambdaDispatcher<decltype(valueSetter), void, const char*, const char*> ValueSetter;

    // act
    const unsigned int result = GameKitGetAwsProfile("default", &valueSetter, ValueSetter::Dispatch, TestLogger::Log);

    // assert
    ASSERT_STREQ(retrievedAccessKey.c_str(), "DefaultAccessKey");
    ASSERT_STREQ(retrievedSecret.c_str(), "DefaultSecretKey");
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);
}

TEST_F(GameKitSettingsExportTestFixture, MalformedProfile_GetProfile_ReturnError)
{
    // arrange
    std::ofstream output(TEST_CREDENTIALS_FILE_LOCATION);

    output << "default" << std::endl;
    output << "aws_access_key_id=DefaultAccessKey" << std::endl;
    output << "aws_secret_access_key=DefaultSecretKey" << std::endl;

    std::string retrievedAccessKey;
    std::string retrievedSecret;
    auto valueSetter = [&retrievedAccessKey, &retrievedSecret](const char* accessKey, const char* secret)
    {
        retrievedAccessKey = accessKey;
        retrievedSecret = secret;
    };
    typedef LambdaDispatcher<decltype(valueSetter), void, const char*, const char*> ValueSetter;

    // act
    const unsigned int result = GameKitGetAwsProfile("default", &valueSetter, ValueSetter::Dispatch, TestLogger::Log);

    // assert
    ASSERT_STREQ(retrievedAccessKey.c_str(), "");
    ASSERT_STREQ(retrievedSecret.c_str(), "");
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_CREDENTIALS_FILE_MALFORMED);
}

TEST_F(GameKitSettingsExportTestFixture, MissingProfile_GetProfile_ReturnError)
{
    // arrange
    std::ofstream output(TEST_CREDENTIALS_FILE_LOCATION);

    output << "[default]" << std::endl;
    output << "aws_access_key_id=DefaultAccessKey" << std::endl;
    output << "aws_secret_access_key=DefaultSecretKey" << std::endl;

    std::string retrievedAccessKey;
    std::string retrievedSecret;
    auto valueSetter = [&retrievedAccessKey, &retrievedSecret](const char* accessKey, const char* secret)
    {
        retrievedAccessKey = accessKey;
        retrievedSecret = secret;
    };
    typedef LambdaDispatcher<decltype(valueSetter), void, const char*, const char*> ValueSetter;

    // act
    const unsigned int result = GameKitGetAwsProfile("GameKit-Credentials", &valueSetter, ValueSetter::Dispatch, TestLogger::Log);

    // assert
    ASSERT_STREQ(retrievedAccessKey.c_str(), "");
    ASSERT_STREQ(retrievedSecret.c_str(), "");
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_CREDENTIALS_NOT_FOUND);
}