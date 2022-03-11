// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "gmock/gmock.h"

#include <aws/gamekit/core/utils/current_time_provider.h>

using testing::Return;

namespace GameKit
{
    namespace Mocks
    {
        class MockCurrentTimeProvider : public GameKit::Utils::ICurrentTimeProvider
        {
        public:
            MockCurrentTimeProvider() {}
            ~MockCurrentTimeProvider() override {}

            /**
             * Convenience constructor. Makes all mock methods return the provided timestamp.
             */
            MockCurrentTimeProvider(int64_t millisecondsSinceEpoch)
            {
                ON_CALL(*this, GetCurrentTimeMilliseconds()).WillByDefault(Return(millisecondsSinceEpoch));
            }

            MOCK_METHOD(int64_t, GetCurrentTimeMilliseconds, (), (const, override));
        };
    }
}
