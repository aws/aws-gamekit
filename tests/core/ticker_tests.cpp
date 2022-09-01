// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// GameKit
#include "test_log.h"
#include "ticker_tests.h"

using namespace GameKit::Tests::Utils;

#pragma region Base Class Tests
void GameKitUtilsTickerTestFixture::Test_Ticker_ExecuteCallback_Success()
{
    // arrange
    std::unique_ptr<GameKit::Utils::Ticker> t = CreateTicker(1, std::bind(&GameKitUtilsTickerTestFixture::MockTickCallback1, this), TestLogger::Log);

    // act
    // the Ticker will execute every second for 4 seconds. At each tick, it will add an item
    // to the std::vector "callBacks". All ticks will be executed, Stop() will wait for thread completion.
    t->Start();
    std::this_thread::sleep_for(std::chrono::seconds(4));
    t->Stop();

    // assert
    ASSERT_EQ(4, GetCallbacks1().size());
}

void GameKitUtilsTickerTestFixture::Test_Ticker_Abort_Success()
{
    // arrange
    std::unique_ptr<GameKit::Utils::Ticker> t = CreateTicker(1, std::bind(&GameKitUtilsTickerTestFixture::MockTickCallbackAbort, this), TestLogger::Log);
    m_ticker = t.get();

    // act
    t->Start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    t->Stop();

    // assert
    ASSERT_EQ(1, GetCallbacks1().size());
}

void GameKitUtilsTickerTestFixture::Test_SharedTicker_ThreadStopsAfterTickerDestroyed()
{
    // arrange
    std::shared_ptr<GameKit::Utils::Ticker> sharedTicker = MakeSharedTicker("ticker",
        1, std::bind(&GameKitUtilsTickerTestFixture::MockTickCallback1, this), TestLogger::Log);

    // act
    sharedTicker->Start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    sharedTicker->Stop();

    sharedTicker.reset(CreateTicker(1, std::bind(&GameKitUtilsTickerTestFixture::MockTickCallback2, this), TestLogger::Log).release());
    sharedTicker->Start();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    sharedTicker->Stop();

    // assert
    ASSERT_EQ(2, GetCallbacks1().size());
    ASSERT_EQ(3, GetCallbacks2().size());
}

void GameKitUtilsTickerTestFixture::Test_Ticker_StartCalledTwice_NewThreadNotStarted()
{
    // arrange
    std::unique_ptr<GameKit::Utils::Ticker> t = CreateTicker(1, std::bind(&GameKitUtilsTickerTestFixture::MockTickCallback1, this), TestLogger::Log);

    // act
    t->Start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    t->Start();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    t->Stop();

    // assert
    ASSERT_EQ(5, GetCallbacks1().size());
}
#pragma endregion
