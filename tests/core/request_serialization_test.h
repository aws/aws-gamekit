// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <gtest/gtest.h>
#include "aws/gamekit/core/utils/gamekit_httpclient_types.h"
#include "test_stack.h"
#include "test_log.h"

namespace GameKit
{
    namespace Tests
    {
        namespace Utils
        {
            class GameKitRequestSerializationTestFixture : public ::testing::Test
            {
            protected:
                TestStackInitializer testStack;
                typedef TestLog<GameKitRequestSerializationTestFixture> TestLogger;

            public:
                GameKitRequestSerializationTestFixture();
                ~GameKitRequestSerializationTestFixture();

                void SetUp();
                void TearDown();
            };
        }
    }
}
