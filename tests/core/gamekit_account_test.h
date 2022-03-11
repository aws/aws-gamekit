// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "test_common.h"

namespace GameKit
{
    namespace Tests
    {
        namespace GameKitAccount
        {
            static const std::string PLUGIN_ROOT = "../core/test_data/sampleplugin/base";
            static const std::string GAMEKIT_ROOT = "../core/test_data/sampleplugin/instance";

            Aws::UniquePtr<GameKit::Mocks::MockS3Client> accountS3Mock;
            Aws::UniquePtr<GameKit::Mocks::MockSSMClient> accountSsmMock;
            Aws::UniquePtr<GameKit::Mocks::MockCloudFormationClient> accountCfnMock;
            Aws::UniquePtr<GameKit::Mocks::MockSecretsManagerClient> accountSecretsMock;

            using namespace testing;
            namespace S3Model = Aws::S3::Model;
            namespace SSMModel = Aws::SSM::Model;
            namespace CfnModel = Aws::CloudFormation::Model;
            namespace SecretsModel = Aws::SecretsManager::Model;
            namespace STSModel = Aws::STS::Model;

            Aws::UniquePtr<GameKit::GameKitAccount> testGamekitAccountInstance = nullptr;
            class GameKitAccountTestFixture;
        }
    }
}
