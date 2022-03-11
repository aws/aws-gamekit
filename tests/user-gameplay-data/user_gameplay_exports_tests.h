// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../core/test_common.h"
#include "../core/test_log.h"
#include "../core/test_stack.h"
#include "aws/gamekit/user-gameplay-data/gamekit_user_gameplay_data.h"

namespace GameKit
{
    namespace Tests
    {
        class GameKitUserGameplayDataExportsTestFixture : public ::testing::Test
        {
        protected:
            typedef TestLog<GameKitUserGameplayDataExportsTestFixture> TestLogger;

            TestStackInitializer testStack;

            void* sessionManagerInstance;

            void* CreateDefault();
            void SetMocks(void* handle, std::shared_ptr<Aws::Http::HttpClient> mockHttpClient);

            bool ValidateItemKeysProxy(const char* const* bundleItemKeys, int numKeys, std::stringstream& tempBuffer);

        public:
            GameKitUserGameplayDataExportsTestFixture() : sessionManagerInstance(nullptr) 
            {}

            ~GameKitUserGameplayDataExportsTestFixture() override 
            {}

            virtual void SetUp() override;
            virtual void TearDown() override;
        };
    }
}