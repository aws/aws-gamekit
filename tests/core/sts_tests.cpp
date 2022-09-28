// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// AWS SDK
#include <aws/sts/STSClient.h>
#include <aws/sts/model/GetAccessKeyInfoRequest.h>
#include <aws/sts/model/GetCallerIdentityRequest.h>

// GameKit
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/core/utils/sts_utils.h>

#include "sts_tests.h"

namespace STSModel = Aws::STS::Model;
using namespace GameKit::Utils;
using namespace GameKit::Tests::Utils;
using namespace ::testing;

void STSUtilsTestFixture::SetUp()
{
    testStack.Initialize();
}

void STSUtilsTestFixture::TearDown()
{
    testStack.CleanupAndLog<TestLogger>();
    TestExecutionUtils::AbortOnFailureIfEnabled();
}

TEST_F(STSUtilsTestFixture, ValidSTSClient_TestGetAccountId_ApiCalled)
{
    // arrange
    STSUtils stsUtils("key", "secret", TestLogger::Log);

    std::shared_ptr<GameKit::Mocks::MockSTSClient> stsMock = std::make_shared<GameKit::Mocks::MockSTSClient>();
    STSModel::GetCallerIdentityResult result;
    std::string expectedAccountId = "A0123456789";
    result.SetAccount(ToAwsString(expectedAccountId));
    auto identityOutcome = STSModel::GetCallerIdentityOutcome(result);
    EXPECT_CALL(*stsMock, GetCallerIdentity(_))
        .Times(1)
        .WillOnce(Return(identityOutcome));

    // act
    stsUtils.SetSTSClient(stsMock);
    std::string actualAccountId = stsUtils.GetAwsAccountId();

    // assert
    ASSERT_STREQ(actualAccountId.c_str(), expectedAccountId.c_str());
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&stsMock));
}

TEST_F(STSUtilsTestFixture, ValidSTSClient_TestTryGetAssumeRoleCredentials_ApiCalled)
{
    // arrange
    STSUtils stsUtils("key", "secret", TestLogger::Log);

    std::shared_ptr<GameKit::Mocks::MockSTSClient> stsMock = std::make_shared<GameKit::Mocks::MockSTSClient>();
    STSModel::AssumeRoleResult result;
    STSModel::Credentials credentials;
    std::string accessKeyId = "ACCESSKEYID123456789";
    std::string secret = "secret";
    std::string sessionToken = "sessionToken";
    credentials.SetAccessKeyId(ToAwsString(accessKeyId));
    credentials.SetSecretAccessKey(ToAwsString(secret));
    credentials.SetSessionToken(ToAwsString(sessionToken));
    result.SetCredentials(credentials);
    auto outcome = STSModel::AssumeRoleOutcome(result);

    EXPECT_CALL(*stsMock, AssumeRole(_))
        .Times(1)
        .WillOnce(Return(outcome));

    // act
    stsUtils.SetSTSClient(stsMock);
    Aws::STS::Model::Credentials actualCredentials;
    bool success = stsUtils.TryGetAssumeRoleCredentials("roleArn", "roleSessionName", "policy", actualCredentials);

    // assert
    ASSERT_TRUE(success);
    ASSERT_STREQ(actualCredentials.GetAccessKeyId().c_str(), accessKeyId.c_str());
    ASSERT_STREQ(actualCredentials.GetSecretAccessKey().c_str(), secret.c_str());
    ASSERT_STREQ(actualCredentials.GetSessionToken().c_str(), sessionToken.c_str());
    ASSERT_TRUE(actualCredentials.AccessKeyIdHasBeenSet());
    ASSERT_TRUE(actualCredentials.SecretAccessKeyHasBeenSet());
    ASSERT_TRUE(actualCredentials.SessionTokenHasBeenSet());
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&stsMock));
}

TEST_F(STSUtilsTestFixture, ValidSTSClient_TestTryGetAssumeRoleCredentials_ApiReturnsError)
{
    // arrange
    STSUtils stsUtils("key", "secret", TestLogger::Log);

    std::shared_ptr<GameKit::Mocks::MockSTSClient> stsMock = std::make_shared<GameKit::Mocks::MockSTSClient>();
    Aws::STS::STSError error;
    auto outcome = STSModel::AssumeRoleOutcome(error);

    EXPECT_CALL(*stsMock, AssumeRole(_))
        .Times(1)
        .WillOnce(Return(outcome));

    // act
    stsUtils.SetSTSClient(stsMock);
    Aws::STS::Model::Credentials actualCredentials;
    bool success = stsUtils.TryGetAssumeRoleCredentials("roleArn", "roleSessionName", "policy", actualCredentials);

    // assert
    ASSERT_FALSE(success);
    ASSERT_STREQ(actualCredentials.GetAccessKeyId().c_str(), "");
    ASSERT_STREQ(actualCredentials.GetSecretAccessKey().c_str(), "");
    ASSERT_STREQ(actualCredentials.GetSessionToken().c_str(), "");
    ASSERT_FALSE(actualCredentials.AccessKeyIdHasBeenSet());
    ASSERT_FALSE(actualCredentials.SecretAccessKeyHasBeenSet());
    ASSERT_FALSE(actualCredentials.SessionTokenHasBeenSet());
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(&stsMock));
}