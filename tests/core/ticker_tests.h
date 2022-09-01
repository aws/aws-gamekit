// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// GameKit
#include "test_log.h"
#include "aws/gamekit/core/utils/ticker.h"

// GTest
#include <gtest/gtest.h>

namespace GameKit
{
    namespace Tests
    {
        namespace Utils
        {
            class GameKitUtilsTickerTestFixture : public ::testing::Test
            {
            protected:
                typedef std::vector<bool> TickCallbacks;
                typedef TestLog<GameKitUtilsTickerTestFixture> TestLogger;

                GameKit::Utils::Ticker* m_ticker;
                TickCallbacks callBacks1;
                TickCallbacks callBacks2;

                virtual std::unique_ptr<GameKit::Utils::Ticker> CreateTicker(int interval, std::function<void()> tickFunc, FuncLogCallback logCb) = 0;
                virtual std::shared_ptr<GameKit::Utils::Ticker> MakeSharedTicker(const char* allocationTag, int interval, std::function<void()> tickFunc, FuncLogCallback logCb) = 0;

#pragma region Base Class Tests
                // Each of these methods is a single unit test for the base class functionality.
                // Each test should be called from within it's own TEST_F() inside each Ticker subclass's unit tests.
                //
                // Example:
                //
                // /foo_ticker_tests.cpp/
                // TEST_F(GameKitUtilsFooTickerTestFixture, Ticker_ExecuteCallback_Success)
                // {
                //     Test_Ticker_ExecuteCallback_Success();
                // }
                //
                // /bar_ticker_tests.cpp/
                // TEST_F(GameKitUtilsBarTickerTestFixture, Ticker_ExecuteCallback_Success)
                // {
                //     Test_Ticker_ExecuteCallback_Success();
                // }
                void Test_Ticker_ExecuteCallback_Success();
                void Test_Ticker_Abort_Success();
                void Test_SharedTicker_ThreadStopsAfterTickerDestroyed();
                void Test_Ticker_StartCalledTwice_NewThreadNotStarted();
#pragma  endregion

            public:
                GameKitUtilsTickerTestFixture()
                {}

                ~GameKitUtilsTickerTestFixture() override
                {}

                void SetUp() override
                {
                    TestLogger::Clear();
                }

                void TearDown() override
                {
                    callBacks1.clear();
                    callBacks2.clear();
                }

                void MockTickCallback1()
                {
                    callBacks1.push_back(true);
                }

                void MockTickCallback2()
                {
                    callBacks2.push_back(true);
                }

                void MockTickCallbackAbort()
                {
                    callBacks1.push_back(true);
                    m_ticker->AbortLoop();
                }

                std::vector<bool> GetCallbacks1() const
                {
                    return callBacks1;
                }

                std::vector<bool> GetCallbacks2() const
                {
                    return callBacks2;
                }
            };
        }
    }
}
