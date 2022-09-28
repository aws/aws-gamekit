// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "feature_resources_test.h"
#include "test_stack.h"
#include "test_log.h"
#include "custom_test_flags.h"

#include <boost/filesystem.hpp>

#define INSTANCE_FILES_DIR "../core/test_data/sampleplugin/instance/testgame/dev/uswe2"

class GameKit::Tests::GameKitFeatureResources::GameKitFeatureResourcesTestFixture : public ::testing::Test
{
protected:
    TestStackInitializer testStackInitializer;
    typedef TestLog<GameKitFeatureResourcesTestFixture> TestLogger;
    Aws::UniquePtr<GameKit::GameKitAccount> gamekitAccountInstance = nullptr;
    Aws::UniquePtr<GameKit::GameKitFeatureResources> gamekitFeatureResourcesInstance = nullptr;

public:
    GameKitFeatureResourcesTestFixture()
    {}

    ~GameKitFeatureResourcesTestFixture()
    {}

    void SetUp()
    {
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

        s3Mock.reset();
        ssmMock.reset();
        cfnMock.reset();

        s3Mock = std::make_unique<GameKit::Mocks::MockS3Client>();
        ssmMock = std::make_unique<GameKit::Mocks::MockSSMClient>();
        cfnMock = std::make_unique<GameKit::Mocks::MockCloudFormationClient>();

        cfnMock->DelegateToFake();

        gamekitFeatureResourcesInstance->SetS3Client(s3Mock.get(), false);
        gamekitFeatureResourcesInstance->SetSSMClient(ssmMock.get(), false);
        gamekitFeatureResourcesInstance->SetCloudFormationClient(cfnMock.get(), false);
    }

    void TearDown()
    {
        ASSERT_TRUE(Mock::VerifyAndClearExpectations(s3Mock.get()));
        ASSERT_TRUE(Mock::VerifyAndClearExpectations(ssmMock.get()));
        ASSERT_TRUE(Mock::VerifyAndClearExpectations(cfnMock.get()));
        
        gamekitAccountInstance.reset();
        gamekitFeatureResourcesInstance.reset();
        s3Mock.release();
        ssmMock.release();
        cfnMock.release();

        testStackInitializer.CleanupAndLog<TestLogger>();
        TestExecutionUtils::AbortOnFailureIfEnabled();
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
    unsigned int saveFuncResult = gamekitFeatureResourcesInstance->SaveFunctionInstances();
    unsigned int saveCFResult = gamekitFeatureResourcesInstance->SaveCloudFormationInstance();

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, saveFuncResult);
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, saveCFResult);

    // clean artifacts
    TestFileSystemUtils::DeleteDirectory(INSTANCE_FILES_DIR);
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
    remove("../core/test_data/sampleplugin/alternativeInstanceEmptyConfig/testgame/dev/awsGameKitClientConfig.yml");
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
    gamekitFeatureResourcesInstance->SetGameKitRoot("../core/test_data/sampleplugin/instance");
    // arrange
    EXPECT_CALL(*cfnMock, DescribeStacks(_)).Times(1);

    ON_CALL(*cfnMock, DescribeStacks).WillByDefault([this](const Aws::CloudFormation::Model::DescribeStacksRequest& request)
    {
        return GameKit::Mocks::FakeCloudFormationClient().DescribeStacks(request);
    });

    // act
    const unsigned result = gamekitFeatureResourcesInstance->WriteClientConfiguration();

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(cfnMock.get()));
}

TEST_F(GameKitFeatureResourcesTestFixture, CanGetExistingParameters)
{
    gamekitFeatureResourcesInstance->SetPluginRoot("../core/test_data/sampleplugin/base");
    gamekitFeatureResourcesInstance->SetGameKitRoot("../core/test_data/sampleplugin/instance");
    // arrange
    EXPECT_CALL(*cfnMock, DescribeStacks(_)).Times(1);

    ON_CALL(*cfnMock, DescribeStacks).WillByDefault([this](const Aws::CloudFormation::Model::DescribeStacksRequest& request)
        {
            return GameKit::Mocks::FakeCloudFormationClient().DescribeStacks(request);
        });

    // act
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

TEST_F(GameKitFeatureResourcesTestFixture, UpdateDashboardStatusListsStacks)
{
    gamekitFeatureResourcesInstance->SetPluginRoot("../core/test_data/sampleplugin/base");
    gamekitFeatureResourcesInstance->SetGameKitRoot("../core/test_data/sampleplugin/instance");

    // arrange
    EXPECT_CALL(*cfnMock, ListStacks(_)).Times(1);
    std::unordered_set<FeatureType> features;
    features.insert(FeatureType::Identity);

    // act
    gamekitFeatureResourcesInstance->UpdateDashboardDeployStatus(features);

    // assert
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(cfnMock.get()));
}

TEST_F(GameKitFeatureResourcesTestFixture, DeployFeatureFunctions_Success)
{
    // arrange
    gamekitFeatureResourcesInstance->SetPluginRoot("../core/test_data/sampleplugin/base");
    gamekitFeatureResourcesInstance->SetGameKitRoot("../core/test_data/sampleplugin/instance");

    SSMModel::PutParameterResult putParamResult;
    putParamResult.SetVersion(1);
    auto putParamOutcome = SSMModel::PutParameterOutcome(putParamResult);
    EXPECT_CALL(*ssmMock.get(), PutParameter(_))
        .Times(AtLeast(1))
        .WillOnce(Return(putParamOutcome));

    S3Model::PutObjectResult putObjResult;
    putObjResult.SetETag("abc-123");
    auto putObjOutcome = S3Model::PutObjectOutcome(putObjResult);
    EXPECT_CALL(*s3Mock.get(), PutObject(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(putObjOutcome));

    // act
    unsigned int saveFuncResult = gamekitFeatureResourcesInstance->SaveFunctionInstances();
    unsigned int deployResult = gamekitFeatureResourcesInstance->DeployFeatureFunctions();

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, saveFuncResult);
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, deployResult);
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(ssmMock.get()));
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(s3Mock.get()));
    Mock::VerifyAndClearExpectations(testStackInitializer.GetMockHttpClientFactory()->GetClient().get());
    testStackInitializer.GetMockHttpClientFactory()->GetClient().reset();

    // clean artifacts
    TestFileSystemUtils::DeleteDirectory(INSTANCE_FILES_DIR);
}
