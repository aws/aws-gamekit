// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// GameKit
#include "timestamp_ticker_tests.h"

// GTest
#include <gtest/gtest.h>

using namespace GameKit::Tests::Utils;

TEST_F(GameKitUtilsSystemClockTickerTestFixture, Ticker_Executecallback_Success)
{
    Test_Ticker_ExecuteCallback_Success();
}

TEST_F(GameKitUtilsSystemClockTickerTestFixture, Ticker_Abort_Success)
{
    Test_Ticker_Abort_Success();
}

TEST_F(GameKitUtilsSystemClockTickerTestFixture, SharedTicker_ThreadStopsAfterTickerDestroyed)
{
    Test_SharedTicker_ThreadStopsAfterTickerDestroyed();
}

TEST_F(GameKitUtilsSystemClockTickerTestFixture, Ticker_StartCalledTwice_NewThreadNotStarted)
{
    Test_Ticker_StartCalledTwice_NewThreadNotStarted();
}