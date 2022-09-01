// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// AWS SDK
#include <aws/core/utils/memory/stl/AWSAllocator.h>

// GameKit
#include "ticker_tests.h"
#include "aws/gamekit/core/utils/count_ticker.h"

namespace GameKit
{
    namespace Tests
    {
        namespace Utils
        {
            class GameKitUtilsCountTickerTestFixture : public GameKitUtilsTickerTestFixture
            {
            protected:
                std::unique_ptr<GameKit::Utils::Ticker> CreateTicker(int interval, std::function<void()> tickFunc, FuncLogCallback logCb) override
                {
                    return std::make_unique<GameKit::Utils::CountTicker>(interval, tickFunc, TestLogger::Log);
                }

                std::shared_ptr<GameKit::Utils::Ticker> MakeSharedTicker(const char* allocationTag, int interval, std::function<void()> tickFunc, FuncLogCallback logCb) override
                {
                    return Aws::MakeShared<GameKit::Utils::CountTicker>(allocationTag, interval, tickFunc, logCb);
                }
            };
        }
    }
}
