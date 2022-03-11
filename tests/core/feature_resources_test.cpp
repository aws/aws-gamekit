// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "feature_resources_test.h"
#include "test_stack.h"
#include "test_log.h"

#include <boost/filesystem.hpp>

class GameKit::Tests::GameKitFeatureResources::GameKitFeatureResourcesTestFixture : public ::testing::Test
{
protected:
    TestStackInitializer testStackInitializer;
    typedef TestLog<GameKitFeatureResourcesTestFixture> TestLogger;

public:
    GameKitFeatureResourcesTestFixture()
    {}

    ~GameKitFeatureResourcesTestFixture()
    {}

    void SetUp()
    {
        TestLogger::Clear();
        testStackInitializer.Initialize();

        gamekitAccountInstance = Aws::MakeUnique<GameKit::GameKitAccount>(
            "gamekitAccountInstance",
            GameKit::AccountInfo{ "dev", "123456789012", "TestCompany", "testgame" },
            GameKit::AccountCredentials{ "us-west-2", "AKIA...", "naRg8H..." },
            TestLogger::Log);

        gamekitFeatureResourcesInstance = Aws::MakeUnique<GameKit::GameKitFeatureResources>(
            "GameKitFeatureResources",
            gamekitAccountInstance->GetAccountInfo(),
            gamekitAccountInstance->GetAccountCredentials(),
            GameKit::FeatureType::Identity,
            TestLogger::Log);
    }

    void TearDown()
    {
        gamekitAccountInstance.reset();
        gamekitFeatureResourcesInstance.reset();

        testStackInitializer.Cleanup();
    }
};

using namespace GameKit::Tests::GameKitFeatureResources;
TEST_F(GameKitFeatureResourcesTestFixture, TestSaveLocalCloudFormation_Saved)
{
    // arrange
    gamekitAccountInstance->SetPluginRoot("../core/test_data/sampleplugin/base");
    gamekitAccountInstance->SetGameKitRoot("../core/test_data/sampleplugin/instance");
    gamekitFeatureResourcesInstance->SetPluginRoot("../core/test_data/sampleplugin/base");
    gamekitFeatureResourcesInstance->SetGameKitRoot("../core/test_data/sampleplugin/instance");
    gamekitFeatureResourcesInstance->SetBaseCloudFormationPath(gamekitAccountInstance->GetBaseCloudFormationPath());
    gamekitFeatureResourcesInstance->SetInstanceCloudFormationPath(gamekitAccountInstance->GetInstanceCloudFormationPath());
    gamekitFeatureResourcesInstance->SetBaseFunctionsPath(gamekitAccountInstance->GetBaseFunctionsPath());
    gamekitFeatureResourcesInstance->SetInstanceFunctionsPath(gamekitAccountInstance->GetInstanceFunctionsPath());

    // act
    unsigned result = gamekitFeatureResourcesInstance->SaveFunctionInstances();
    result = gamekitFeatureResourcesInstance->SaveCloudFormationInstance();

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
}

TEST_F(GameKitFeatureResourcesTestFixture, TestWriteEmptyConfigFile_Saved)
{
    // arrange
    gamekitAccountInstance->SetPluginRoot("../core/test_data/sampleplugin/base");
    gamekitAccountInstance->SetGameKitRoot("../core/test_data/sampleplugin/alternativeInstanceEmptyConfig");
    gamekitFeatureResourcesInstance->SetPluginRoot("../core/test_data/sampleplugin/base");
    gamekitFeatureResourcesInstance->SetGameKitRoot("../core/test_data/sampleplugin/alternativeInstanceEmptyConfig");

    // act
    remove("../core/test_data/sampleplugin/alternativeInstanceEmptyConfig/testgame/dev/awsGameKitClientConfig.yml");
    const unsigned result = gamekitFeatureResourcesInstance->WriteEmptyClientConfiguration();

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    ASSERT_TRUE(boost::filesystem::exists("../core/test_data/sampleplugin/alternativeInstanceEmptyConfig/testgame/dev/awsGameKitClientConfig.yml"));
}

TEST_F(GameKitFeatureResourcesTestFixture, WhenSetAwsClient_ThenClientPreservedInDestructor)
{
    // arrange
    const auto s3Mock = Aws::MakeUnique<GameKit::Mocks::MockS3Client>("s3Mock");
    const bool isShared = true;

    {
        gamekitFeatureResourcesInstance->SetS3Client(s3Mock.get(), isShared);
        s3Mock->CallDieInDestructor(true);


        // act
        gamekitFeatureResourcesInstance.reset(); // calls delete() on the pointed-to object

        // assert
        // TODO:: This fails on non-Windows
        EXPECT_CALL(*s3Mock, Die()).Times(0);   // Insure the s3Mock is not deleted when resetting FeatureResourceInstance
    }

    // At this point the s3Mock has gone out of scope of the test so it will call the destructor independently of gamekitFeatureResourcesInstance
    EXPECT_CALL(*s3Mock, Die()).Times(1);
}

TEST_F(GameKitFeatureResourcesTestFixture, WhenNoNewValues_DoNotWriteClientConfiguration)
{
    // arrange
    const auto cfnMock = Aws::MakeUnique<GameKit::Mocks::MockCloudFormationClient>("cfnMock");
    const auto cfnClientMock = cfnMock.get();

    EXPECT_CALL(*cfnClientMock, DescribeStacks(_)).Times(1);

    ON_CALL(*cfnClientMock, DescribeStacks).WillByDefault([this](const Aws::CloudFormation::Model::DescribeStacksRequest& request)
    {
        return GameKit::Mocks::FakeCloudFormationClient().DescribeStacks(request);
    });

    // act
    gamekitFeatureResourcesInstance->SetCloudFormationClient(cfnClientMock, true);

    const unsigned result = gamekitFeatureResourcesInstance->WriteClientConfiguration();

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(cfnMock.get()));
}

TEST_F(GameKitFeatureResourcesTestFixture, CanGetExistingParameters)
{
    gamekitFeatureResourcesInstance->SetGameKitRoot("../core/test_data/sampleplugin/instance");
    gamekitFeatureResourcesInstance->SetPluginRoot("../core/test_data/sampleplugin/base");
    // arrange
    const auto cfnMock = Aws::MakeUnique<GameKit::Mocks::MockCloudFormationClient>("cfnMock");
    const auto cfnClientMock = cfnMock.get();

    EXPECT_CALL(*cfnClientMock, DescribeStacks(_)).Times(1);

    ON_CALL(*cfnClientMock, DescribeStacks).WillByDefault([this](const Aws::CloudFormation::Model::DescribeStacksRequest& request)
        {
            return GameKit::Mocks::FakeCloudFormationClient().DescribeStacks(request);
        });

    // act
    gamekitFeatureResourcesInstance->SetCloudFormationClient(cfnClientMock, true);

    const DeployedParametersCallback callback = [](const char* key, const char* value)
    {
        ASSERT_NE(std::string(key).length(), 0);
        ASSERT_NE(std::string(value).length(), 0);
    };

    // Write out values with current stack status
    const unsigned status = gamekitFeatureResourcesInstance->GetDeployedCloudFormationParameters(callback);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, status);
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(cfnMock.get()));
}