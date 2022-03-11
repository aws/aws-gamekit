// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <gmock/gmock.h>

#include <aws/sts/STSClient.h>
#include <aws/sts/model/GetAccessKeyInfoRequest.h>
#include <aws/sts/model/GetCallerIdentityRequest.h>

namespace GameKit
{
    namespace Mocks
    {
        class MockSTSClient : public Aws::STS::STSClient
        {
        public:
            MOCK_METHOD(Aws::STS::Model::GetCallerIdentityOutcome, GetCallerIdentity, (const Aws::STS::Model::GetCallerIdentityRequest&), (const, override));
            MOCK_METHOD(Aws::STS::Model::AssumeRoleOutcome, AssumeRole, (const Aws::STS::Model::AssumeRoleRequest&), (const, override));
        };
    }
}