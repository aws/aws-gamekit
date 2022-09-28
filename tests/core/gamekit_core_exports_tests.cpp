// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/core/internal/platform_string.h>

#include "gamekit_core_exports_tests.h"
#include "test_stack.h"
#include "test_log.h"
#include "custom_test_flags.h"

using ::testing::Action;

#define INSTANCE_FILES_DIR "../core/test_data/sampleplugin/instance/testgame/dev/uswe2"

class GameKit::Tests::CoreExports::GameKitCoreExportsTestFixture : public ::testing::Test
{
protected:
    TestStackInitializer testStackInitializer;
    typedef TestLog<GameKitCoreExportsTestFixture> TestLogger;

public:
    GameKitCoreExportsTestFixture()
    {
    }

    ~GameKitCoreExportsTestFixture()
    {
    }

    void SetUp()
    {
        testStackInitializer.Initialize();
    }

    void TearDown()
    {
        coreS3Mock.release();
        coreSsmMock.release();
        coreCfnMock.release();
        coreSecretsMock.release();
        coreApigwMock.release();

        testStackInitializer.CleanupAndLog<TestLogger>();
        TestExecutionUtils::AbortOnFailureIfEnabled();
    }

    GAMEKIT_ACCOUNT_INSTANCE_HANDLE createAccountInstance()
    {
        return GameKitAccountInstanceCreateWithRootPaths(GameKit::AccountInfo{ "dev", "123456789012", "TestCompany", "testgame" },
            GameKit::AccountCredentials{ "us-west-2", "AKIA...", "naRg8H..." }, "../core/test_data/sampleplugin/instance", "../core/test_data/sampleplugin/base", TestLogger::Log);
    }

    GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE createFeatureResourceInstance(GameKit::FeatureType featureType)
    {
        return GameKitResourcesInstanceCreateWithRootPaths(GameKit::AccountInfo{ "dev", "123456789012", "TestCompany", "testgame" },
            GameKit::AccountCredentials{ "us-west-2", "AKIA...", "naRg8H..." }, featureType, "../core/test_data/sampleplugin/instance", "../core/test_data/sampleplugin/base", TestLogger::Log);
    }
};

static std::vector<std::string> resourceInfoCache;
void ResourceInfoCallbackTest(const char* logicalResourceId, const char* resourceType, const char* resourceStatus)
{
    resourceInfoCache[0] = logicalResourceId;
    resourceInfoCache[1] = resourceType;
    resourceInfoCache[2] = resourceStatus;
}

using namespace GameKit::Tests::CoreExports;

class StackStatusReciever
{
public:
    std::string stackStatus;
    void OnRecieveStackStatus(const char* stackStatus)
    {
        this->stackStatus = stackStatus;
    }
};

class StackStatusDispatcher
{
public:
    static void StackStatusCallbackDispatcher(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* stackStatus)
    {
        ((StackStatusReciever*)dispatchReceiver)->OnRecieveStackStatus(stackStatus);
    }
};

TEST_F(GameKitCoreExportsTestFixture, TestGameKitAccountInstanceCreate_Success)
{
    // act
    GameKit::GameKitAccount* acctInstance = (GameKit::GameKitAccount*)createAccountInstance();

    // assert
    ASSERT_NE(acctInstance, nullptr);

    delete(acctInstance);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitAccountInstanceRelease_Success)
{
    // arrange
    void* acctInstance = createAccountInstance();

    // act
    GameKitAccountInstanceRelease(acctInstance);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitAccountSetGetRootPath_Success)
{
    // arrange
    auto acctInstance = createAccountInstance();

    // act
    GameKitAccountSetRootPath(acctInstance, "/a/b/c");
    std::string result = GameKitAccountGetRootPath(acctInstance);

    GameKitAccountInstanceRelease(acctInstance);

    // assert
    ASSERT_EQ(result, "/a/b/c");
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitAccountSetGetPluginRootPath_Success)
{
    // arrange
    auto acctInstance = createAccountInstance();

    // act
    GameKitAccountSetPluginRootPath(acctInstance, "/a/b/c");
    std::string result = GameKitAccountGetPluginRootPath(acctInstance);

    GameKitAccountInstanceRelease(acctInstance);

    // assert
    ASSERT_EQ(result, "/a/b/c");
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitAccountGetBaseAndInstancePaths_Success)
{
    // arrange
    auto acctInstance = createAccountInstance();
    GameKitAccountSetRootPath(acctInstance, "/a/b/c");
    GameKitAccountSetPluginRootPath(acctInstance, "/x/y/z");

    // act
    const char* cfBasePath = GameKitAccountGetBaseCloudFormationPath(acctInstance);
    const char* funcBasePath = GameKitAccountGetBaseFunctionsPath(acctInstance);
    const char* cfInstPath = GameKitAccountGetInstanceCloudFormationPath(acctInstance);
    const char* funcInstPath = GameKitAccountGetInstanceFunctionsPath(acctInstance);

    // assert
    ASSERT_EQ(strcmp(cfBasePath, "/x/y/z/cloudformation/"), 0);
    ASSERT_EQ(strcmp(funcBasePath, "/x/y/z/functions/"), 0);
    ASSERT_EQ(strcmp(cfInstPath, "/a/b/c/testgame/dev/uswe2/cloudformation/"), 0);
    ASSERT_EQ(strcmp(funcInstPath, "/a/b/c/testgame/dev/uswe2/functions/"), 0);

    GameKitAccountInstanceRelease(acctInstance);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitAccountInstanceHasValidCredentials_True)
{
    // arrange
    auto acctInstance = createAccountInstance();
    setAccountMocks(acctInstance);

    S3Model::ListBucketsResult bucketResult;
    auto listOutcome = S3Model::ListBucketsOutcome(bucketResult);
    EXPECT_CALL(*coreS3Mock.get(), ListBuckets())
        .Times(1)
        .WillOnce(Return(listOutcome));

    // act
    auto result = GameKitAccountHasValidCredentials(acctInstance);

    // assert
    ASSERT_TRUE(result);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreS3Mock.get()));

    GameKitAccountInstanceRelease(acctInstance);
    coreS3Mock.reset();
    coreCfnMock.reset();
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitGetAccountId_Error)
{
    class MockAccountIdCaller : public SimpleCaller
    {
    public:
        MOCK_METHOD(void, OnHandleResult, (const char*), (override));
    };

    typedef CallbackHandler<void(MockAccountIdCaller::*)(const char*), &MockAccountIdCaller::OnHandleResult> MockCallbackHandler;

    // arrange
    const char* key = nullptr;
    const char* secret = nullptr;
    MockAccountIdCaller caller;
    EXPECT_CALL(caller, OnHandleResult(_)).Times(0);

    // act
    unsigned int result = GameKitGetAwsAccountId(&caller, &MockCallbackHandler::OnCallback, key, secret, TestLogger::Log);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_GENERAL);
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&caller));
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitAccountInstanceBootstrap_Success)
{
    // arrange
    auto acctInstance = createAccountInstance();
    setAccountMocks(acctInstance);

    GameKitAccountSetRootPath(acctInstance, "../core/test_data/sampleplugin/instance");

    S3Model::ListBucketsResult bucketResult;
    S3Model::Bucket bucket;
    std::string base36AccountId = GameKit::Utils::EncodingUtils::DecimalToBase("123456789012", GameKit::Utils::BASE_36);
    std::string name = "do-not-delete-gamekit-dev-uswe2-" + base36AccountId + "-testgame";
    bucket.SetName(ToAwsString(name));
    bucketResult.AddBuckets(bucket);
    auto listOutcome = S3Model::ListBucketsOutcome(bucketResult);
    EXPECT_CALL(*coreS3Mock.get(), ListBuckets())
        .Times(1)
        .WillOnce(Return(listOutcome));

    // act
    auto result = GameKitAccountInstanceBootstrap(acctInstance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreS3Mock.get()));

    GameKitAccountInstanceRelease(acctInstance);
    coreS3Mock.reset();
    coreCfnMock.reset();
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitAccountSaveSecret_Success)
{
    // arrange
    auto acctInstance = createAccountInstance();
    setAccountMocks(acctInstance);

    auto describeOutcome = SecretsModel::DescribeSecretOutcome();
    EXPECT_CALL(*coreSecretsMock.get(), DescribeSecret(_))
        .Times(1)
        .WillOnce(Return(describeOutcome));

    SecretsModel::CreateSecretResult createResult;
    createResult.SetName("key");
    auto createOutcome = SecretsModel::CreateSecretOutcome(createResult);
    EXPECT_CALL(*coreSecretsMock.get(), CreateSecret(_))
        .Times(1)
        .WillOnce(Return(createOutcome));

    // act
    auto result = GameKitAccountSaveSecret(acctInstance, "key", "secret");

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreSecretsMock.get()));

    GameKitAccountInstanceRelease(acctInstance);
    coreSecretsMock.reset();
    coreCfnMock.reset();
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitAccountSaveFeatureInstanceTemplates_Success)
{
    // arrange
    auto acctInstance = createAccountInstance();
    setAccountMocks(acctInstance);

    GameKitAccountSetRootPath(acctInstance, "../core/test_data/sampleplugin/instance");
    GameKitAccountSetPluginRootPath(acctInstance, "../core/test_data/sampleplugin/base");

    // act
    auto result = GameKitAccountSaveFeatureInstanceTemplates(acctInstance);

    GameKitAccountInstanceRelease(acctInstance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    coreCfnMock.reset();

    // clean artifacts
    TestFileSystemUtils::DeleteDirectory(INSTANCE_FILES_DIR);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitAccountUploadFunctions_Success)
{
    // arrange
    auto acctInstance = createAccountInstance();
    setAccountMocks(acctInstance);

    GameKitAccountSetRootPath(acctInstance, "../core/test_data/sampleplugin/instance");
    GameKitAccountSetPluginRootPath(acctInstance, "../core/test_data/sampleplugin/base");

    SSMModel::PutParameterResult putParamResult;
    putParamResult.SetVersion(1);
    auto putParamOutcome = SSMModel::PutParameterOutcome(putParamResult);
    EXPECT_CALL(*coreSsmMock.get(), PutParameter(_))
        .Times(AtLeast(3)) // three features in sample plugin directory
        .WillRepeatedly(Return(putParamOutcome));

    S3Model::PutObjectResult putObjResult;
    putObjResult.SetETag("abc-123");
    auto putObjOutcome = S3Model::PutObjectOutcome(putObjResult);
    EXPECT_CALL(*coreS3Mock.get(), PutObject(_))
        .Times(AtLeast(7)) // seven sample lambda functions in sample plugin directory
        .WillRepeatedly(Return(putObjOutcome));

    // act
    unsigned int saveTemplatesResult = GameKitAccountSaveFeatureInstanceTemplates(acctInstance);
    unsigned int uploadResult = GameKitAccountUploadFunctions(acctInstance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, saveTemplatesResult);
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, uploadResult);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreS3Mock.get()));
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreSsmMock.get()));

    GameKitAccountInstanceRelease(acctInstance);
    coreS3Mock.reset();
    coreSsmMock.reset();
    coreCfnMock.reset();

    // clean artifacts
    TestFileSystemUtils::DeleteDirectory(INSTANCE_FILES_DIR);
}
TEST_F(GameKitCoreExportsTestFixture, TestGameKitAccountCreateOrUpdateMainStack_Success)
{
    // arrange
    auto acctInstance = createAccountInstance();
    setAccountMocks(acctInstance);

    GameKitAccountSetRootPath(acctInstance, "../core/test_data/sampleplugin/instance");
    GameKitAccountSetPluginRootPath(acctInstance, "../core/test_data/sampleplugin/base");

    auto stack = CfnModel::Stack();
    auto stacks = Aws::Vector<CfnModel::Stack>();

    // arrange -- upload functions mock
    SSMModel::PutParameterResult putParamResult;
    putParamResult.SetVersion(1);
    auto putParamOutcome = SSMModel::PutParameterOutcome(putParamResult);

    S3Model::PutObjectResult putObjResult;
    putObjResult.SetETag("abc-123");
    auto putObjOutcome = S3Model::PutObjectOutcome(putObjResult);

    CfnModel::DescribeStacksResult describeStackInProgressResult;
    stack.SetStackStatus(CfnModel::StackStatus::CREATE_IN_PROGRESS);
    stacks.push_back(stack);
    describeStackInProgressResult.SetStacks(stacks);
    auto describeStackInProgressOutcome = CfnModel::DescribeStacksOutcome(describeStackInProgressResult);

    stacks.clear();
    CfnModel::DescribeStacksResult describeStackCompleteResult;
    stack.SetStackStatus(CfnModel::StackStatus::CREATE_COMPLETE);
    stacks.push_back(stack);
    describeStackCompleteResult.SetStacks(stacks);
    auto describeStackCompleteOutcome = CfnModel::DescribeStacksOutcome(describeStackCompleteResult);

    auto describeNoResultOutcome = CfnModel::DescribeStacksOutcome();

    EXPECT_CALL(*coreCfnMock.get(), DescribeStacks(_))
        .Times(5)
        .WillOnce(Return(describeNoResultOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillRepeatedly(Return(describeStackCompleteResult));

    EXPECT_CALL(*coreCfnMock.get(), CreateStackCallable(_))
        .Times(1);

    EXPECT_CALL(*coreCfnMock.get(), DescribeStackEventsCallable(_))
        .Times(3);

    // act
    unsigned int saveTemplatesResult = GameKitAccountSaveFeatureInstanceTemplates(acctInstance);
    unsigned int createResult = GameKitAccountCreateOrUpdateMainStack(acctInstance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, saveTemplatesResult);
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, createResult);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreCfnMock.get()));
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreS3Mock.get()));
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreSsmMock.get()));

    GameKitAccountInstanceRelease(acctInstance);
    coreCfnMock.reset();

    // clean artifacts
    TestFileSystemUtils::DeleteDirectory(INSTANCE_FILES_DIR);
}
TEST_F(GameKitCoreExportsTestFixture, TestGameKitAccountCreateOrUpdateStacks_Success)
{
    // arrange
    auto acctInstance = createAccountInstance();
    setAccountMocks(acctInstance);

    GameKitAccountSetRootPath(acctInstance, "../core/test_data/sampleplugin/instance");
    GameKitAccountSetPluginRootPath(acctInstance, "../core/test_data/sampleplugin/base");

    auto stack = CfnModel::Stack();
    auto stacks = Aws::Vector<CfnModel::Stack>();

    // arrange -- upload functions mock
    SSMModel::PutParameterResult putParamResult;
    putParamResult.SetVersion(1);
    auto putParamOutcome = SSMModel::PutParameterOutcome(putParamResult);
    EXPECT_CALL(*coreSsmMock.get(), PutParameter(_))
        .Times(AtLeast(3)) // three features in sample plugin directory
        .WillRepeatedly(Return(putParamOutcome));

    S3Model::PutObjectResult putObjResult;
    putObjResult.SetETag("abc-123");
    auto putObjOutcome = S3Model::PutObjectOutcome(putObjResult);
    EXPECT_CALL(*coreS3Mock.get(), PutObject(_))
        .Times(AtLeast(7)) // seven sample lambda functions in sample plugin directory
        .WillRepeatedly(Return(putObjOutcome));

    CfnModel::DescribeStacksResult describeStackInProgressResult;
    stack.SetStackStatus(CfnModel::StackStatus::CREATE_IN_PROGRESS);
    stacks.push_back(stack);
    describeStackInProgressResult.SetStacks(stacks);
    auto describeStackInProgressOutcome = CfnModel::DescribeStacksOutcome(describeStackInProgressResult);

    stacks.clear();
    CfnModel::DescribeStacksResult describeStackCompleteResult;
    stack.SetStackStatus(CfnModel::StackStatus::CREATE_COMPLETE);
    stacks.push_back(stack);
    describeStackCompleteResult.SetStacks(stacks);
    auto describeStackCompleteOutcome = CfnModel::DescribeStacksOutcome(describeStackCompleteResult);

    auto describeNoResultOutcome = CfnModel::DescribeStacksOutcome();

    EXPECT_CALL(*coreCfnMock.get(), DescribeStacks(_))
        .Times(AtLeast(10))
        .WillOnce(Return(describeNoResultOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillRepeatedly(Return(describeStackCompleteResult));

    EXPECT_CALL(*coreCfnMock.get(), UpdateStackCallable(_))
        .Times(3);

    EXPECT_CALL(*coreCfnMock.get(), CreateStackCallable(_))
        .Times(AtLeast(1));

    EXPECT_CALL(*coreCfnMock.get(), DescribeStackEventsCallable(_))
        .Times(AtLeast(2));

    // act
    unsigned int saveTemplatesResult = GameKitAccountSaveFeatureInstanceTemplates(acctInstance);
    unsigned int createResult = GameKitAccountCreateOrUpdateStacks(acctInstance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, createResult);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreCfnMock.get()));
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreS3Mock.get()));
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreSsmMock.get()));

    GameKitAccountInstanceRelease(acctInstance);
    coreSsmMock.reset();
    coreS3Mock.reset();
    coreCfnMock.reset();

    // clean artifacts
    TestFileSystemUtils::DeleteDirectory(INSTANCE_FILES_DIR);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitFeatureResourceInstanceCreate_Success)
{
    // act
    GameKit::GameKitFeatureResources* resourceInstance = (GameKit::GameKitFeatureResources*)createFeatureResourceInstance(GameKit::FeatureType::Identity);

    // assert
    ASSERT_NE(resourceInstance, nullptr);

    delete(resourceInstance);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitFeatureResourceInstanceRelease_Success)
{
    // arrange
    void* resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Identity);

    // act
    GameKitResourcesInstanceRelease(resourceInstance);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitFeatureResourceSetGetRootPath_Success)
{
    // arrange
    auto resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Identity);

    // act
    GameKitResourcesSetRootPath(resourceInstance, "/a/b/c");
    std::string result = GameKitResourcesGetRootPath(resourceInstance);

    GameKitResourcesInstanceRelease(resourceInstance);

    // assert
    ASSERT_EQ(result, "/a/b/c");
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitFeatureResourceSetGetPluginRootPath_Success)
{
    // arrange
    auto resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Identity);

    // act
    GameKitResourcesSetPluginRootPath(resourceInstance, "/a/b/c");
    std::string result = GameKitResourcesGetPluginRootPath(resourceInstance);

    GameKitResourcesInstanceRelease(resourceInstance);

    // assert
    ASSERT_EQ(result, "/a/b/c");
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitFeatureResourceGetBaseAndInstancePaths_Success)
{
    // arrange
    auto resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Identity);
    GameKitResourcesSetRootPath(resourceInstance, "/a/b/c");
    GameKitResourcesSetPluginRootPath(resourceInstance, "/x/y/z");

    // act
    const char* cfBasePath = GameKitResourcesGetBaseCloudFormationPath(resourceInstance);
    const char* funcBasePath = GameKitResourcesGetBaseFunctionsPath(resourceInstance);
    const char* cfInstPath = GameKitResourcesGetInstanceCloudFormationPath(resourceInstance);
    const char* funcInstPath = GameKitResourcesGetInstanceFunctionsPath(resourceInstance);

    // assert
    ASSERT_EQ(strcmp(cfBasePath, "/x/y/z/cloudformation/identity/"), 0);
    ASSERT_EQ(strcmp(funcBasePath, "/x/y/z/functions/identity/"), 0);
    ASSERT_EQ(strcmp(cfInstPath, "/a/b/c/testgame/dev/uswe2/cloudformation/identity/"), 0);
    ASSERT_EQ(strcmp(funcInstPath, "/a/b/c/testgame/dev/uswe2/functions/identity/"), 0);

    GameKitResourcesInstanceRelease(resourceInstance);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitFeatureResourceCreateOrUpdateStacks_Success)
{
    // arrange
    auto resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Identity);
    setResourceMocks(resourceInstance);

    
    EXPECT_CALL(*coreCfnMock.get(), DescribeStacks(_))
        .Times(3);

    EXPECT_CALL(*coreCfnMock.get(), CreateStackCallable(_))
        .Times(1);

    EXPECT_CALL(*coreCfnMock.get(), DescribeStackEventsCallable(_))
        .Times(1);

    // act
    const auto result = GameKitResourcesInstanceCreateOrUpdateStack(resourceInstance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreCfnMock.get()));
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreS3Mock.get()));
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreSsmMock.get()));

    GameKitResourcesInstanceRelease(resourceInstance);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitFeatureResourceDeleteStack_Success)
{
    // arrange
    auto resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Identity);
    setResourceMocks(resourceInstance);

    GameKitResourcesSetRootPath(resourceInstance, DUMMY_INSTANCE_PATH);

    auto stack = CfnModel::Stack();
    auto stacks = Aws::Vector<CfnModel::Stack>();

    stacks.clear();
    CfnModel::DescribeStacksResult describeDeleteProgressResult;
    stack.SetStackStatus(CfnModel::StackStatus::DELETE_IN_PROGRESS);
    stacks.push_back(stack);
    describeDeleteProgressResult.SetStacks(stacks);
    CfnModel::DescribeStacksOutcome describeDeleteProgressOutcome(describeDeleteProgressResult);

    stacks.clear();
    CfnModel::DescribeStacksResult describeDeleteCompleteResult;
    stack.SetStackStatus(CfnModel::StackStatus::DELETE_COMPLETE);
    stacks.push_back(stack);
    describeDeleteCompleteResult.SetStacks(stacks);
    auto describeDeleteCompleteOutcome = CfnModel::DescribeStacksOutcome(describeDeleteCompleteResult);

    EXPECT_CALL(*coreCfnMock.get(), DescribeStacks(_))
        .Times(3)
        .WillOnce(Return(describeDeleteProgressOutcome))
        .WillOnce(Return(describeDeleteProgressOutcome))
        .WillOnce(Return(describeDeleteCompleteOutcome));

    EXPECT_CALL(*coreCfnMock.get(), DeleteStackCallable(_))
        .Times(1);

    EXPECT_CALL(*coreCfnMock.get(), DescribeStackEventsCallable(_))
        .Times(2);

    // act
    auto result = GameKitResourcesInstanceDeleteStack(resourceInstance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreCfnMock.get()));

    GameKitResourcesInstanceRelease(resourceInstance);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitFeatureGetCurrentStackStatus_Success)
{
    // arrange
    auto resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Identity);
    setResourceMocks(resourceInstance);

    auto stack = CfnModel::Stack();
    auto stacks = Aws::Vector<CfnModel::Stack>();

    stacks.clear();
    CfnModel::DescribeStacksResult describeStackCompleteResult;
    stack.SetStackStatus(CfnModel::StackStatus::CREATE_COMPLETE);
    stacks.push_back(stack);
    describeStackCompleteResult.SetStacks(stacks);
    auto describeStackCompleteOutcome = CfnModel::DescribeStacksOutcome(describeStackCompleteResult);

    auto describeNoResultOutcome = CfnModel::DescribeStacksOutcome();

    EXPECT_CALL(*coreCfnMock.get(), DescribeStacks(_))
        .Times(1)
        .WillOnce(Return(describeStackCompleteResult));

    // act
    StackStatusReciever* const receiver = new StackStatusReciever();
    auto result = GameKitResourcesGetCurrentStackStatus(resourceInstance, receiver, StackStatusDispatcher::StackStatusCallbackDispatcher);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    ASSERT_EQ(strcmp(receiver->stackStatus.c_str(), CfnModel::StackStatusMapper::GetNameForStackStatus(CfnModel::StackStatus::CREATE_COMPLETE).c_str()), 0);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreCfnMock.get()));

    GameKitResourcesInstanceRelease(resourceInstance);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitFeatureGetCurrentStackStatus_NoCurrentStackStatus)
{
    // arrange
    auto resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Identity);
    setResourceMocks(resourceInstance);

    EXPECT_CALL(*coreCfnMock.get(), DescribeStacks(_))
        .Times(1);

    // act
    StackStatusReciever* const receiver = new StackStatusReciever();
    auto result = GameKitResourcesGetCurrentStackStatus(resourceInstance, receiver, StackStatusDispatcher::StackStatusCallbackDispatcher);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_ERROR_CLOUDFORMATION_NO_CURRENT_STACK_STATUS, result);
    ASSERT_EQ(strcmp(receiver->stackStatus.c_str(), GameKit::ERR_STACK_CURRENT_STATUS_UNDEPLOYED), 0);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreCfnMock.get()));

    GameKitResourcesInstanceRelease(resourceInstance);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitAccountDeployApiGatewayStage_Success)
{
    // arrange
    auto acctInstance = createAccountInstance();
    setAccountMocks(acctInstance);

    const CfnModel::DescribeStackResourceResult describeResult;;

    EXPECT_CALL(*coreCfnMock.get(), DescribeStackResource(_))
        .Times(1)
        .WillOnce(Return(describeResult));

    const ApigwModel::CreateDeploymentResult createDeploymentResults;

    EXPECT_CALL(*coreApigwMock.get(), CreateDeployment(_))
        .Times(1)
        .WillOnce(Return(createDeploymentResults));

    const ApigwModel::UpdateStageResult updateStageResults;

    EXPECT_CALL(*coreApigwMock.get(), UpdateStage(_))
        .Times(1)
        .WillOnce(Return(updateStageResults));

    // act
    const auto result = GameKitAccountDeployApiGatewayStage(acctInstance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreApigwMock.get()));
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreCfnMock.get()));

    GameKitAccountInstanceRelease(acctInstance);
    coreApigwMock.reset();
    coreCfnMock.reset();
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitFeatureDescribeStackResources_Success)
{
    // arrange
    auto resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Identity);
    setResourceMocks(resourceInstance);

    CfnModel::DescribeStackResourcesResult describeResult;

    Aws::Vector<CfnModel::StackResource> resources;
    CfnModel::StackResource resource;
    resource.SetLogicalResourceId("Resource123");
    resource.SetResourceType("Type123");
    resource.SetResourceStatus(CfnModel::ResourceStatus::CREATE_COMPLETE);
    resources.push_back(resource);
    describeResult.SetStackResources(resources);
    auto outcome = CfnModel::DescribeStackResourcesOutcome(describeResult);

    EXPECT_CALL(*coreCfnMock.get(), DescribeStackResources(_))
        .Times(1)
        .WillOnce(Return(describeResult));

    resourceInfoCache.resize(3);

    // act
    unsigned int result = GameKitResourcesDescribeStackResources(resourceInstance, ResourceInfoCallbackTest);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);
    ASSERT_STREQ(resourceInfoCache[0].c_str(), "Resource123");
    ASSERT_STREQ(resourceInfoCache[1].c_str(), "Type123");
    ASSERT_STREQ(resourceInfoCache[2].c_str(), "CREATE_COMPLETE");
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreCfnMock.get()));

    GameKitResourcesInstanceRelease(resourceInstance);

    resourceInfoCache.clear();
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitResourcesSaveCloudFormationInstance_Success)
{
    // arrange
    auto resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Identity);
    setResourceMocks(resourceInstance);

    // act
    auto result = GameKitResourcesSaveCloudFormationInstance(resourceInstance);

    GameKitResourcesInstanceRelease(resourceInstance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);

    // clean artifacts
    TestFileSystemUtils::DeleteDirectory(INSTANCE_FILES_DIR);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitResourcesSaveFunctionInstances_Success)
{
    // arrange
    auto resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Identity);
    setResourceMocks(resourceInstance);

    // act
    auto result = GameKitResourcesSaveFunctionInstances(resourceInstance);

    GameKitResourcesInstanceRelease(resourceInstance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);

    // clean artifacts
    TestFileSystemUtils::DeleteDirectory(INSTANCE_FILES_DIR);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitResourcesUploadFeatureFunctions_Success)
{
    // arrange
    auto resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Identity);
    setResourceMocks(resourceInstance);

    SSMModel::PutParameterResult putParamResult;
    putParamResult.SetVersion(1);
    auto putParamOutcome = SSMModel::PutParameterOutcome(putParamResult);
    EXPECT_CALL(*coreSsmMock.get(), PutParameter(_))
        .Times(AtLeast(1))
        .WillOnce(Return(putParamOutcome));

    S3Model::PutObjectResult putObjResult;
    putObjResult.SetETag("abc-123");
    auto putObjOutcome = S3Model::PutObjectOutcome(putObjResult);
    EXPECT_CALL(*coreS3Mock.get(), PutObject(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(putObjOutcome));

    // act
    unsigned int saveResult = GameKitResourcesSaveFunctionInstances(resourceInstance);
    unsigned int uploadResult = GameKitResourcesUploadFeatureFunctions(resourceInstance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, saveResult);
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, uploadResult);
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreSsmMock.get()));
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreS3Mock.get()));

    GameKitResourcesInstanceRelease(resourceInstance);

    // clean artifacts
    TestFileSystemUtils::DeleteDirectory(INSTANCE_FILES_DIR);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitResourcesIsCloudFormationInstanceTemplatePresent_False)
{
    // arrange
    auto resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Identity);
    setResourceMocks(resourceInstance);

    GameKitResourcesSetRootPath(resourceInstance, "/x/y/z");

    // act
    auto result = GameKitResourcesIsCloudFormationInstanceTemplatePresent(resourceInstance);

    GameKitResourcesInstanceRelease(resourceInstance);

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitResourcesIsCloudFormationInstanceTemplatePresent_True)
{
    // arrange
    auto resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Identity);
    setResourceMocks(resourceInstance);

    // act
    unsigned int saveResult = GameKitResourcesSaveCloudFormationInstance(resourceInstance);
    unsigned int templatePresentResult = GameKitResourcesIsCloudFormationInstanceTemplatePresent(resourceInstance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, saveResult);
    ASSERT_TRUE(templatePresentResult);
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreCfnMock.get()));

    GameKitResourcesInstanceRelease(resourceInstance);

    // clean artifacts
    TestFileSystemUtils::DeleteDirectory(INSTANCE_FILES_DIR);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitResourcesGetDeployedCloudFormationTemplate_Fail)
{
    // arrange
    // no expectation result set to simulate GetTemplate() call's outcome is not successful
    auto resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Main);
    setResourceMocks(resourceInstance);

    EXPECT_CALL(*coreCfnMock.get(), GetTemplate(_))
        .Times(1);

    // act
    auto result = GameKitResourcesSaveDeployedCloudFormationTemplate(resourceInstance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_ERROR_CLOUDFORMATION_GET_TEMPLATE_FAILED, result);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreCfnMock.get()));

    GameKitResourcesInstanceRelease(resourceInstance);
}

TEST_F(GameKitCoreExportsTestFixture, TestGameKitResourcesGetDeployedCloudFormationTemplate_Success)
{
    // arrange
    auto resourceInstance = createFeatureResourceInstance(GameKit::FeatureType::Main);
    setResourceMocks(resourceInstance);

    std::string resultTemplate = "---\n# THIS IS A SAMPLE CLOUDFORMATION TEMPLATE\nParameters:\n  GameKitApiName:\n    Type: String\nResources :\n  RestApi :\n    Type : AWS::ApiGateway::RestApi\n    Properties :\n      Name : !Ref GameKitApiName\n      Parameters:\n        endpointConfigurationTypes: REGIONAL\n";
    CfnModel::GetTemplateResult getTemplateResult;
    getTemplateResult.SetTemplateBody(resultTemplate.c_str());
    auto getTemplateOutcome = CfnModel::GetTemplateOutcome(getTemplateResult);

    EXPECT_CALL(*coreCfnMock.get(), GetTemplate(_))
        .Times(1)
        .WillOnce(Return(getTemplateOutcome));

    EXPECT_CALL(*coreCfnMock.get(), DescribeStackResources(_))
        .Times(1);

    // act
    std::string origTemplate;
    GameKit::Utils::FileUtils::ReadFileIntoString("../core/test_data/sampleplugin/instance/testgame/dev/cloudformation/main/cloudFormation.yml", origTemplate);
    auto result = GameKitResourcesSaveDeployedCloudFormationTemplate(resourceInstance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);

    std::string updatedTemplate;
    GameKit::Utils::FileUtils::ReadFileIntoString("../core/test_data/sampleplugin/instance/testgame/dev/cloudformation/main/cloudFormation.yml", updatedTemplate);
    ASSERT_EQ(origTemplate.compare(updatedTemplate), 0);
    ASSERT_EQ(resultTemplate.compare(updatedTemplate), 0);
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(coreCfnMock.get()));

    GameKitResourcesInstanceRelease(resourceInstance);

    // clean artifacts
    TestFileSystemUtils::DeleteDirectory(INSTANCE_FILES_DIR);
}

void GameKit::Tests::CoreExports::setAccountMocks(void* acctInstance)
{
    coreS3Mock.release();
    coreSsmMock.release();
    coreCfnMock.release();
    coreSecretsMock.release();
    coreApigwMock.release();

    coreS3Mock = std::make_unique<GameKit::Mocks::MockS3Client>();
    coreSsmMock = std::make_unique<GameKit::Mocks::MockSSMClient>();
    coreCfnMock = std::make_unique<GameKit::Mocks::MockCloudFormationClient>();
    coreSecretsMock = std::make_unique<GameKit::Mocks::MockSecretsManagerClient>();
    coreApigwMock = std::make_unique<GameKit::Mocks::MockAPIGatewayClient>();

    coreCfnMock->DelegateToFake();

    GameKit::GameKitAccount* inst = (GameKit::GameKitAccount*)acctInstance;
    inst->DeleteClients();
    inst->DeleteClientsOnDestruction(false);
    inst->SetS3Client(coreS3Mock.get());
    inst->SetSSMClient(coreSsmMock.get());
    inst->SetCloudFormationClient(coreCfnMock.get());
    inst->SetSecretsManagerClient(coreSecretsMock.get());
    inst->SetApiGatewayClient(coreApigwMock.get());
}

void GameKit::Tests::CoreExports::setResourceMocks(void* resourceInstance)
{
    coreS3Mock.release();
    coreSsmMock.release();
    coreCfnMock.release();
    coreSecretsMock.release();
    coreApigwMock.release();

    coreS3Mock = std::make_unique<GameKit::Mocks::MockS3Client>();
    coreSsmMock = std::make_unique<GameKit::Mocks::MockSSMClient>();
    coreCfnMock = std::make_unique<GameKit::Mocks::MockCloudFormationClient>();
    coreSecretsMock = std::make_unique<GameKit::Mocks::MockSecretsManagerClient>();
    coreApigwMock = std::make_unique<GameKit::Mocks::MockAPIGatewayClient>();

    coreCfnMock->DelegateToFake();

    GameKit::GameKitFeatureResources* inst = (GameKit::GameKitFeatureResources*)resourceInstance;
    inst->SetS3Client(coreS3Mock.get(), false);
    inst->SetSSMClient(coreSsmMock.get(), false);
    inst->SetCloudFormationClient(coreCfnMock.get(), false);
}
