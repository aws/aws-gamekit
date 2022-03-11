// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "simple_integration_test.h"

class GameKit::Tests::SimpleIntegration::SimpleIntegrationTestFixture : public ::testing::Test
{
public:
    SimpleIntegrationTestFixture()
    {}

    ~SimpleIntegrationTestFixture()
    {}

    void SetUp()
    {
        // TODO:: Replace with real credentials when running integration test on build machine.
        // Read it from config.
        gamekitAccountInstance = Aws::MakeUnique<GameKit::GameKitAccount>(
            "gamekitAccountInstance",
            GameKit::AccountInfo{ "dev", "123456789012", "TestCompany", "testgame" },
            GameKit::AccountCredentials{ "us-west-2", "AKIA4...", "naRg8..." },
            nullptr);

        gamekitAccountInstance->InitializeDefaultAwsClients();
    }

    void TearDown()
    {
    }
};

using namespace GameKit::Tests::SimpleIntegration;
TEST_F(SimpleIntegrationTestFixture, DISABLED_SimpleIntegrationTest)
{
    gamekitAccountInstance->SetGameKitRoot("../core/test_data/sampleplugin/instance");
    gamekitAccountInstance->SetPluginRoot("../core/test_data/sampleplugin/base");

    auto result = gamekitAccountInstance->HasValidCredentials();
    ASSERT_TRUE(result) << "Failed credentials check.";

    result = gamekitAccountInstance->Bootstrap();
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result) << "Failed bootstrapped account.";

    result = gamekitAccountInstance->SaveSecret("facebook_client_secret", "abcdefghijklmnopqrstuvwxyz");
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result) << "Failed to save secret to Secrets Manager.";

    result = gamekitAccountInstance->SaveFeatureInstanceTemplates();
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result) << "Failed to save Instance CloudFormation templates and Lambda Functions";

    result = gamekitAccountInstance->UploadFunctions();
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result) << "Failed to upload Lambda Functions to S3 bootstrap bucket";

    result = gamekitAccountInstance->CreateOrUpdateMainStack();
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result) << "Failed to create/update Main Stack";

    result = gamekitAccountInstance->CreateOrUpdateFeatureStacks();
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result) << "Failed to create/update Feature Stacks";

    result = gamekitAccountInstance->DeployApiGatewayStage();
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result) << "Failed to deploy latest API Gateway changes";
}
