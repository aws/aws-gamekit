// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// AWS SDK
#include <aws/core/utils/memory/stl/AWSAllocator.h>

// GameKit
#include "ticker_tests.h"
#include "aws/gamekit/core/utils/timestamp_ticker.h"

namespace GameKit
{
    namespace Tests
    {
        namespace Utils
        {
            class GameKitUtilsSystemClockTickerTestFixture : public GameKitUtilsTickerTestFixture
            {
            protected:
                std::unique_ptr<GameKit::Utils::Ticker> CreateTicker(int interval, std::function<void()> tickFunc, FuncLogCallback logCb) override
                {
                    return std::make_unique<GameKit::Utils::TimestampTicker>(interval, tickFunc, TestLogger::Log);
                }

                std::shared_ptr<GameKit::Utils::Ticker> MakeSharedTicker(const char* allocationTag, int interval, std::function<void()> tickFunc, FuncLogCallback logCb) override
                {
                    return Aws::MakeShared<GameKit::Utils::TimestampTicker>(allocationTag, interval, tickFunc, logCb);
                }
            };
        }
    }
}
