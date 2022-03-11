// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "gmock/gmock.h"

#include <aws/apigateway/APIGatewayClient.h>

namespace GameKit
{
    namespace Mocks
    {
        class MockAPIGatewayClient : public Aws::APIGateway::APIGatewayClient
        {
        public:
            MockAPIGatewayClient() {}
            ~MockAPIGatewayClient() {}
            MOCK_METHOD(Aws::APIGateway::Model::CreateDeploymentOutcome, CreateDeployment, (const Aws::APIGateway::Model::CreateDeploymentRequest& request), (const, override));
            MOCK_METHOD(Aws::APIGateway::Model::UpdateStageOutcome, UpdateStage, (const Aws::APIGateway::Model::UpdateStageRequest& request), (const, override));
        };
    }
}
