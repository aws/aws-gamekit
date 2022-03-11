// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "gmock/gmock.h"

#include <aws/ssm/SSMClient.h>

namespace GameKit
{
    namespace Mocks
    {
        class MockSSMClient : public Aws::SSM::SSMClient
        {
        public:
            MockSSMClient() {}
            ~MockSSMClient() {}
            MOCK_METHOD(Aws::SSM::Model::PutParameterOutcome, PutParameter, (const Aws::SSM::Model::PutParameterRequest& request), (const, override));
        };
    }
}
