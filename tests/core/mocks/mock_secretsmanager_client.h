// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "gmock/gmock.h"

#include <aws/secretsmanager/SecretsManagerClient.h>

namespace GameKit
{
    namespace Mocks
    {
        class MockSecretsManagerClient : public Aws::SecretsManager::SecretsManagerClient
        {
        public:
            MockSecretsManagerClient() {}
            ~MockSecretsManagerClient() {}
            MOCK_METHOD(Aws::SecretsManager::Model::DescribeSecretOutcome, DescribeSecret, (const Aws::SecretsManager::Model::DescribeSecretRequest& request), (const, override));
            MOCK_METHOD(Aws::SecretsManager::Model::CreateSecretOutcome, CreateSecret, (const Aws::SecretsManager::Model::CreateSecretRequest& request), (const, override));
            MOCK_METHOD(Aws::SecretsManager::Model::UpdateSecretOutcome, UpdateSecret, (const Aws::SecretsManager::Model::UpdateSecretRequest& request), (const, override));
            MOCK_METHOD(Aws::SecretsManager::Model::DeleteSecretOutcome, DeleteSecret, (const Aws::SecretsManager::Model::DeleteSecretRequest& request), (const, override));
        };
    }
}
