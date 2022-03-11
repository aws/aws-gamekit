// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../core/test_common.h"
#include <aws/gamekit/user-gameplay-data/gamekit_user_gameplay_data_models.h>

namespace GameKit
{
    namespace Tests
    {
        namespace DataModels
        {
            class UserGameplayDataModelsTestFixture : public ::testing::Test
            {
            public:
                UserGameplayDataModelsTestFixture();
                ~UserGameplayDataModelsTestFixture();

                virtual void SetUp() override;
                virtual void TearDown() override;
            };
        }
    }
}