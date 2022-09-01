// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/core/internal/platform_string.h>

#include "gamekit_account_test.h"
#include "test_stack.h"
#include "test_log.h"

class GameKit::Tests::GameKitAccount::GameKitAccountTestFixture : public ::testing::Test
{
protected:
    TestStackInitializer testStack;
    typedef TestLog<GameKitAccountTestFixture> TestLogger;

public:
    GameKitAccountTestFixture()
    {}

    ~GameKitAccountTestFixture()
    {}

    void SetUp()
    {
        TestLogger::Clear();
        testStack.Initialize();

        testGamekitAccountInstance = Aws::MakeUnique<GameKit::GameKitAccount>(
            "testGamekitAccountInstance",
            GameKit::AccountInfo{ "dev", "123456789012", "TestCompany", "testgame" },
            GameKit::AccountCredentials{ "us-west-2", "AKIA...", "naRg8H..." },
            TestLogger::Log);

        accountS3Mock = Aws::MakeUnique<GameKit::Mocks::MockS3Client>("accountS3Mock");
        accountSsmMock = Aws::MakeUnique<GameKit::Mocks::MockSSMClient>("accountSsmMock");
        accountCfnMock = Aws::MakeUnique<GameKit::Mocks::MockCloudFormationClient>("accountCfnMock");
        accountSecretsMock = Aws::MakeUnique<GameKit::Mocks::MockSecretsManagerClient>("accountSecretsMock");

        accountCfnMock->DelegateToFake();

        testGamekitAccountInstance->SetS3Client(accountS3Mock.get());
        testGamekitAccountInstance->SetSSMClient(accountSsmMock.get());
        testGamekitAccountInstance->SetCloudFormationClient(accountCfnMock.get());
        testGamekitAccountInstance->SetSecretsManagerClient(accountSecretsMock.get());
        testGamekitAccountInstance->SetPluginRoot(PLUGIN_ROOT);
        testGamekitAccountInstance->SetGameKitRoot(GAMEKIT_ROOT);
    }

    void TearDown()
    {
        testGamekitAccountInstance.reset();

        testStack.Cleanup();
        ASSERT_TRUE(Mock::VerifyAndClearExpectations(accountS3Mock.get()));
        ASSERT_TRUE(Mock::VerifyAndClearExpectations(accountSsmMock.get()));
        ASSERT_TRUE(Mock::VerifyAndClearExpectations(accountCfnMock.get()));
        ASSERT_TRUE(Mock::VerifyAndClearExpectations(accountSecretsMock.get()));
    }
};

using namespace GameKit::Tests::GameKitAccount;
TEST_F(GameKitAccountTestFixture, BucketExists_TestHasbootstrapBucket_True)
{
    // arrange
    S3Model::ListBucketsResult bucketResult;
    S3Model::Bucket bucket;
    std::string base36AccountId = GameKit::Utils::EncodingUtils::DecimalToBase("123456789012", GameKit::Utils::BASE_36);
    std::string name = "do-not-delete-gamekit-dev-uswe2-" + base36AccountId + "-testgame";
    bucket.SetName(ToAwsString(name));
    bucketResult.AddBuckets(bucket);
    auto listOutcome = S3Model::ListBucketsOutcome(bucketResult);
    EXPECT_CALL(*accountS3Mock.get(), ListBuckets())
        .Times(1)
        .WillOnce(Return(listOutcome));

    // act
    auto result = testGamekitAccountInstance->HasBootstrapBucket();

    // assert
    ASSERT_TRUE(result);
}

TEST_F(GameKitAccountTestFixture, BucketNotExists_TestHasbootstrapBucket_False)
{
    // arrange
    S3Model::ListBucketsResult bucketResult;
    S3Model::Bucket bucket;
    bucket.SetName("do-not-delete-gamekit-dev-210987654321-testgame");
    bucketResult.AddBuckets(bucket);
    auto listOutcome = S3Model::ListBucketsOutcome(bucketResult);
    EXPECT_CALL(*accountS3Mock.get(), ListBuckets())
        .Times(1)
        .WillOnce(Return(listOutcome));

    // act
    auto result = testGamekitAccountInstance->HasBootstrapBucket();

    // assert	
    ASSERT_FALSE(result);
}

TEST_F(GameKitAccountTestFixture, BucketNotExists_TestBootstrap_Create)
{
    // arrange
    S3Model::ListBucketsResult bucketResult;
    auto listOutcome = S3Model::ListBucketsOutcome(bucketResult);
    EXPECT_CALL(*accountS3Mock.get(), ListBuckets())
        .Times(1)
        .WillOnce(Return(listOutcome));

    S3Model::CreateBucketResult createBucketResult;
    createBucketResult.SetLocation("testlocation");
    auto createOutcome = S3Model::CreateBucketOutcome(createBucketResult);
    EXPECT_CALL(*accountS3Mock.get(), CreateBucket(_))
        .Times(1)
        .WillOnce(Return(createOutcome));
    EXPECT_CALL(*accountS3Mock.get(), PutBucketLifecycleConfiguration(_))
        .Times(1);

    // act
    auto result = testGamekitAccountInstance->Bootstrap();

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
}


TEST_F(GameKitAccountTestFixture, MissingKeyAndSecret_TestHasValidCredentials_False)
{
    // arrange
    Aws::UniquePtr<GameKit::GameKitAccount> testGamekitAccountInstanceMissingKeyAndSecret = Aws::MakeUnique<GameKit::GameKitAccount>(
        "testGamekitAccountInstance",
        GameKit::AccountInfo{ "dev", "123456789012", "TestCompany", "testgame" },
        GameKit::AccountCredentials{ "us-west-2", "", "" },
        TestLogger::Log);

    testGamekitAccountInstanceMissingKeyAndSecret->SetS3Client(accountS3Mock.get());
    testGamekitAccountInstanceMissingKeyAndSecret->SetSSMClient(accountSsmMock.get());
    testGamekitAccountInstanceMissingKeyAndSecret->SetCloudFormationClient(accountCfnMock.get());
    testGamekitAccountInstanceMissingKeyAndSecret->SetSecretsManagerClient(accountSecretsMock.get());

    // act
    auto result = testGamekitAccountInstanceMissingKeyAndSecret->HasValidCredentials();

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitAccountTestFixture, MissingAccessKey_TestHasValidCredentials_False)
{
    // arrange
    Aws::UniquePtr<GameKit::GameKitAccount> testGamekitAccountInstanceMissingAccessKey = Aws::MakeUnique<GameKit::GameKitAccount>(
        "testGamekitAccountInstance",
        GameKit::AccountInfo{ "dev", "123456789012", "TestCompany", "testgame" },
        GameKit::AccountCredentials{ "us-west-2", "", "naRg8H..." },
        TestLogger::Log);

    testGamekitAccountInstanceMissingAccessKey->SetS3Client(accountS3Mock.get());
    testGamekitAccountInstanceMissingAccessKey->SetSSMClient(accountSsmMock.get());
    testGamekitAccountInstanceMissingAccessKey->SetCloudFormationClient(accountCfnMock.get());
    testGamekitAccountInstanceMissingAccessKey->SetSecretsManagerClient(accountSecretsMock.get());

    // act
    auto result = testGamekitAccountInstanceMissingAccessKey->HasValidCredentials();

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitAccountTestFixture, MissingAccessSecret_TestHasValidCredentials_False)
{
    // arrange
    Aws::UniquePtr<GameKit::GameKitAccount> testGamekitAccountInstanceMissingAccessSecret = Aws::MakeUnique<GameKit::GameKitAccount>(
        "testGamekitAccountInstance",
        GameKit::AccountInfo{ "dev", "123456789012", "TestCompany", "testgame" },
        GameKit::AccountCredentials{ "us-west-2", "AKIA...", "" },
        TestLogger::Log);

    testGamekitAccountInstanceMissingAccessSecret->SetS3Client(accountS3Mock.get());
    testGamekitAccountInstanceMissingAccessSecret->SetSSMClient(accountSsmMock.get());
    testGamekitAccountInstanceMissingAccessSecret->SetCloudFormationClient(accountCfnMock.get());
    testGamekitAccountInstanceMissingAccessSecret->SetSecretsManagerClient(accountSecretsMock.get());

    // act
    auto result = testGamekitAccountInstanceMissingAccessSecret->HasValidCredentials();

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitAccountTestFixture, InvalidCredentials_TestHasValidCredentials_False)
{
    // arrange
    Aws::S3::S3Error error = Aws::S3::S3Error(Aws::Client::AWSError<Aws::S3::S3Errors>(Aws::S3::S3Errors::INVALID_ACCESS_KEY_ID, false));
    auto listOutcome = S3Model::ListBucketsOutcome(error);
    EXPECT_CALL(*accountS3Mock.get(), ListBuckets())
        .Times(1)
        .WillOnce(Return(listOutcome));

    // act
    auto result = testGamekitAccountInstance->HasValidCredentials();

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitAccountTestFixture, ValidCredentials_TestHasValidCredentials_True)
{
    // arrange
    S3Model::ListBucketsResult bucketResult;
    auto listOutcome = S3Model::ListBucketsOutcome(bucketResult);
    EXPECT_CALL(*accountS3Mock.get(), ListBuckets())
        .Times(1)
        .WillOnce(Return(listOutcome));

    // act
    auto result = testGamekitAccountInstance->HasValidCredentials();

    // assert
    ASSERT_TRUE(result);
}

TEST_F(GameKitAccountTestFixture, SecretNotExist_TestSaveSecret_Create)
{
    // arrange
    auto describeOutcome = SecretsModel::DescribeSecretOutcome();
    EXPECT_CALL(*accountSecretsMock.get(), DescribeSecret(_))
        .Times(1)
        .WillOnce(Return(describeOutcome));

    SecretsModel::CreateSecretResult createResult;
    createResult.SetName("key");
    auto createOutcome = SecretsModel::CreateSecretOutcome(createResult);
    EXPECT_CALL(*accountSecretsMock.get(), CreateSecret(_))
        .Times(1)
        .WillOnce(Return(createOutcome));

    // act
    auto result = testGamekitAccountInstance->SaveSecret("key", "secret");

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
}

TEST_F(GameKitAccountTestFixture, SecretExists_TestSaveSecret_Update)
{
    // arrange
    SecretsModel::DescribeSecretResult describeResult;
    describeResult.SetName("key");
    auto describeOutcome = SecretsModel::DescribeSecretOutcome(describeResult);
    EXPECT_CALL(*accountSecretsMock.get(), DescribeSecret(_))
        .Times(1)
        .WillOnce(Return(describeOutcome));

    SecretsModel::UpdateSecretResult updateResult;
    updateResult.SetName("key");
    auto createOutcome = SecretsModel::UpdateSecretOutcome(updateResult);
    EXPECT_CALL(*accountSecretsMock.get(), UpdateSecret(_))
        .Times(1)
        .WillOnce(Return(createOutcome));

    // act
    auto result = testGamekitAccountInstance->SaveSecret("key", "secret");

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
}

TEST_F(GameKitAccountTestFixture, SecretNotExists_TestCheckSecretExists_Warning)
{
    // arrange
    auto describeOutcome = SecretsModel::DescribeSecretOutcome();
    EXPECT_CALL(*accountSecretsMock.get(), DescribeSecret(_))
        .Times(1)
        .WillOnce(Return(describeOutcome));

    // act
    auto result = testGamekitAccountInstance->CheckSecretExists("secret");

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_WARNING_SECRETSMANAGER_SECRET_NOT_FOUND, result);
}

TEST_F(GameKitAccountTestFixture, SecretExists_TestCheckSecretExists_Success)
{
    // arrange
    SecretsModel::DescribeSecretResult describeResult;
    describeResult.SetName("key");
    auto describeOutcome = SecretsModel::DescribeSecretOutcome(describeResult);
    EXPECT_CALL(*accountSecretsMock.get(), DescribeSecret(_))
        .Times(1)
        .WillOnce(Return(describeOutcome));

    // act
    auto result = testGamekitAccountInstance->CheckSecretExists("key");

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
}

TEST_F(GameKitAccountTestFixture, SecretExists_TestDeleteSecret)
{
    // arrange
    SecretsModel::DescribeSecretResult describeResult;
    describeResult.SetName("key");
    auto describeOutcome = SecretsModel::DescribeSecretOutcome(describeResult);
    EXPECT_CALL(*accountSecretsMock.get(), DescribeSecret(_))
        .Times(1)
        .WillOnce(Return(describeOutcome));

    SecretsModel::DeleteSecretResult deleteResult;
    deleteResult.SetName("key");
    auto deleteOutcome = SecretsModel::DeleteSecretOutcome(deleteResult);
    EXPECT_CALL(*accountSecretsMock.get(), DeleteSecret(_))
        .Times(1)
        .WillOnce(Return(deleteOutcome));

    // act
    auto result = testGamekitAccountInstance->DeleteSecret("key");

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
}

TEST_F(GameKitAccountTestFixture, SecretNotExists_TestDeleteSecret)
{
    // arrange
    auto describeOutcome = SecretsModel::DescribeSecretOutcome();
    EXPECT_CALL(*accountSecretsMock.get(), DescribeSecret(_))
        .Times(1)
        .WillOnce(Return(describeOutcome));

    EXPECT_CALL(*accountSecretsMock.get(), DeleteSecret(_))
        .Times(0);

    // act
    auto result = testGamekitAccountInstance->DeleteSecret("key");

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
}

TEST_F(GameKitAccountTestFixture, ValidFunctionsPath_TestUploadFunctions_Uploaded)
{
    // arrange
    testGamekitAccountInstance->SetPluginRoot("../core/test_data/sampleplugin/base");
    testGamekitAccountInstance->SetGameKitRoot("../core/test_data/sampleplugin/instance");

    SSMModel::PutParameterResult putParamResult;
    putParamResult.SetVersion(1);
    auto putParamOutcome = SSMModel::PutParameterOutcome(putParamResult);
    EXPECT_CALL(*accountSsmMock.get(), PutParameter(_))
        .Times(AtLeast(3))
        .WillRepeatedly(Return(putParamOutcome));

    S3Model::PutObjectResult putObjResult;
    putObjResult.SetETag("abc-123");
    auto putObjOutcome = S3Model::PutObjectOutcome(putObjResult);
    EXPECT_CALL(*accountS3Mock.get(), PutObject(_))
        .Times(AtLeast(7)) // seven sample lambda functions in sample plugin directory
        .WillRepeatedly(Return(putObjOutcome));

    // act
    auto result = testGamekitAccountInstance->UploadFunctions();

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
}

TEST_F(GameKitAccountTestFixture, MainStackDoesNotExist_TestCreateMainStack_Created)
{
    // arrange
    testGamekitAccountInstance->SetPluginRoot("../core/test_data/sampleplugin/base");
    testGamekitAccountInstance->SetGameKitRoot("../core/test_data/sampleplugin/instance");

    auto stack = CfnModel::Stack();
    auto stacks = Aws::Vector<CfnModel::Stack>();

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
    EXPECT_CALL(*accountCfnMock.get(), DescribeStacks(_))
        .Times(5)
        .WillOnce(Return(describeNoResultOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillOnce(Return(describeStackCompleteResult))
        .WillOnce(Return(describeStackCompleteResult));

    EXPECT_CALL(*accountCfnMock.get(), CreateStackCallable(_))
        .Times(1);

    EXPECT_CALL(*accountCfnMock.get(), DescribeStackEventsCallable(_))
        .Times(3);

    // act
    auto result = testGamekitAccountInstance->CreateOrUpdateMainStack();

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
}

TEST_F(GameKitAccountTestFixture, MainStackExists_TestUpdateMainStack_Updated)
{
    // arrange
    testGamekitAccountInstance->SetPluginRoot("../core/test_data/sampleplugin/base");
    testGamekitAccountInstance->SetGameKitRoot("../core/test_data/sampleplugin/instance");

    auto stack = CfnModel::Stack();
    auto stacks = Aws::Vector<CfnModel::Stack>();

    CfnModel::DescribeStacksResult describeStackExistsResult;
    stack.SetStackStatus(CfnModel::StackStatus::UPDATE_COMPLETE);
    stacks.push_back(stack);
    describeStackExistsResult.SetStacks(stacks);
    auto describeStackExistsOutcome = CfnModel::DescribeStacksOutcome(describeStackExistsResult);

    stacks.clear();
    CfnModel::DescribeStacksResult describeStackInProgressResult;
    stack.SetStackStatus(CfnModel::StackStatus::UPDATE_IN_PROGRESS);
    stacks.push_back(stack);
    describeStackInProgressResult.SetStacks(stacks);
    auto describeStackInProgressOutcome = CfnModel::DescribeStacksOutcome(describeStackInProgressResult);

    stacks.clear();
    CfnModel::DescribeStacksResult describeStackCompleteResult;
    stack.SetStackStatus(CfnModel::StackStatus::UPDATE_COMPLETE);
    stacks.push_back(stack);
    describeStackCompleteResult.SetStacks(stacks);
    auto describeStackCompleteOutcome = CfnModel::DescribeStacksOutcome(describeStackCompleteResult);

    EXPECT_CALL(*accountCfnMock.get(), DescribeStacks(_))
        .Times(5)
        .WillOnce(Return(describeStackExistsOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillRepeatedly(Return(describeStackCompleteResult));

    EXPECT_CALL(*accountCfnMock.get(), UpdateStackCallable(_))
        .Times(1);

    EXPECT_CALL(*accountCfnMock.get(), DescribeStackEventsCallable(_))
        .Times(3);

    // act
    auto result = testGamekitAccountInstance->CreateOrUpdateMainStack();

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
}

TEST_F(GameKitAccountTestFixture, FeatureStacksDoNotExist_TestCreateFeatureStacks_Created)
{
    // arrange
    testGamekitAccountInstance->SetPluginRoot("../core/test_data/sampleplugin/base");
    testGamekitAccountInstance->SetGameKitRoot("../core/test_data/sampleplugin/instance");

    auto stack = CfnModel::Stack();
    auto stacks = Aws::Vector<CfnModel::Stack>();

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
    EXPECT_CALL(*accountCfnMock.get(), DescribeStacks(_))
        .Times(13)
        .WillOnce(Return(describeNoResultOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillOnce(Return(describeStackCompleteResult))
        .WillOnce(Return(describeNoResultOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillOnce(Return(describeStackCompleteResult))
        .WillOnce(Return(describeNoResultOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillRepeatedly(Return(describeStackCompleteResult));

    EXPECT_CALL(*accountCfnMock.get(), UpdateStackCallable(_))
        .Times(2);

    EXPECT_CALL(*accountCfnMock.get(), CreateStackCallable(_))
        .Times(1);

    EXPECT_CALL(*accountCfnMock.get(), DescribeStackEventsCallable(_))
        .Times(7);

    // act
    auto result = testGamekitAccountInstance->CreateOrUpdateFeatureStacks();

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
}

TEST_F(GameKitAccountTestFixture, FeatureStacksExist_TestUpdateFeatureStacks_Updated)
{
    // arrange
    testGamekitAccountInstance->SetPluginRoot("../core/test_data/sampleplugin/base");
    testGamekitAccountInstance->SetGameKitRoot("../core/test_data/sampleplugin/instance");


    auto stack = CfnModel::Stack();
    auto stacks = Aws::Vector<CfnModel::Stack>();

    CfnModel::DescribeStacksResult describeStackExistsResult;
    stack.SetStackStatus(CfnModel::StackStatus::UPDATE_COMPLETE);
    stacks.push_back(stack);
    describeStackExistsResult.SetStacks(stacks);
    auto describeStackExistsOutcome = CfnModel::DescribeStacksOutcome(describeStackExistsResult);

    stacks.clear();
    CfnModel::DescribeStacksResult describeStackInProgressResult;
    stack.SetStackStatus(CfnModel::StackStatus::UPDATE_IN_PROGRESS);
    stacks.push_back(stack);
    describeStackInProgressResult.SetStacks(stacks);
    auto describeStackInProgressOutcome = CfnModel::DescribeStacksOutcome(describeStackInProgressResult);

    stacks.clear();
    CfnModel::DescribeStacksResult describeStackCompleteResult;
    stack.SetStackStatus(CfnModel::StackStatus::UPDATE_COMPLETE);
    stacks.push_back(stack);
    describeStackCompleteResult.SetStacks(stacks);
    auto describeStackCompleteOutcome = CfnModel::DescribeStacksOutcome(describeStackCompleteResult);

    EXPECT_CALL(*accountCfnMock.get(), DescribeStacks(_))
        .Times(13)
        .WillOnce(Return(describeStackExistsOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillOnce(Return(describeStackCompleteResult))
        .WillOnce(Return(describeStackExistsOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillOnce(Return(describeStackCompleteResult))
        .WillOnce(Return(describeStackExistsOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillOnce(Return(describeStackInProgressOutcome))
        .WillRepeatedly(Return(describeStackCompleteResult));

    EXPECT_CALL(*accountCfnMock.get(), UpdateStackCallable(_))
        .Times(3);

    EXPECT_CALL(*accountCfnMock.get(), DescribeStackEventsCallable(_))
        .Times(7);

    // act
    auto result = testGamekitAccountInstance->CreateOrUpdateFeatureStacks();

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
}
