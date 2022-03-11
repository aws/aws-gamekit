// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "test_common.h"
#include <gtest/gtest.h>
#include "aws/gamekit/core/utils/sts_utils.h"
#include "test_stack.h"
#include "test_log.h"

#include <gtest/gtest.h>
namespace GameKit
{
    namespace Tests
    {
        namespace Utils
        {
            class STSUtilsTestFixture : public ::testing::Test
            {
            protected:
                TestStackInitializer testStack;
                typedef TestLog<STSUtilsTestFixture> TestLogger;

            public:
                STSUtilsTestFixture() {}
                ~STSUtilsTestFixture() {}
                virtual void SetUp() override;
                virtual void TearDown() override;
            };
        }
    }
}
