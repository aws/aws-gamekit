// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "test_common.h"

namespace GameKit
{
    namespace Tests
    {
        namespace SimpleIntegration
        {
            Aws::UniquePtr<GameKit::Mocks::MockS3Client> s3Mock;
            Aws::UniquePtr<GameKit::Mocks::MockSSMClient> ssmMock;
            Aws::UniquePtr<GameKit::Mocks::MockCloudFormationClient> cfnMock;

            using namespace testing;
            namespace S3Model = Aws::S3::Model;
            namespace SSMModel = Aws::SSM::Model;
            namespace CfnModel = Aws::CloudFormation::Model;

            class SimpleIntegrationTestFixture;
        }
    }
}
