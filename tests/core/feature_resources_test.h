// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "test_common.h"

namespace GameKit
{
    namespace Tests
    {
        namespace GameKitFeatureResources
        {
            std::unique_ptr<GameKit::Mocks::MockS3Client> s3Mock;
            std::unique_ptr<GameKit::Mocks::MockSSMClient> ssmMock;
            std::unique_ptr<GameKit::Mocks::MockCloudFormationClient> cfnMock;

            using namespace testing;
            namespace S3Model = Aws::S3::Model;
            namespace SSMModel = Aws::SSM::Model;
            namespace CfnModel = Aws::CloudFormation::Model;

            class GameKitFeatureResourcesTestFixture;
        }
    }
}

