// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "ticker_tests.h"
#include "test_log.h"

#include <aws/core/utils/memory/stl/AWSAllocator.h>

class GameKitUtilsTickerTestFixture;
typedef std::vector<bool> TickCallbacks;

using namespace GameKit::Tests::Utils;

GameKitUtilsTickerTestFixture* tickerTestInstance = nullptr;
class GameKitUtilsTickerTestFixture : public ::testing::Test
{
protected:
    typedef TestLog<GameKitUtilsTickerTestFixture> TestLogger;

public:
    GameKitUtilsTickerTestFixture()
    {}

    ~GameKitUtilsTickerTestFixture()
    {}

    void SetUp()
    {
        tickerTestInstance = this;
        TestLogger::Clear();
    }

    void TearDown()
    {
        tickerTestInstance = nullptr;
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

    std::vector<bool> GetCallbacks1()
    {
        return callBacks1;
    }

    std::vector<bool> GetCallbacks2()
    {
        return callBacks2;
    }

protected:
    GameKit::Utils::Ticker* m_ticker;
    TickCallbacks callBacks1;
    TickCallbacks callBacks2;
};

void TickCallbackWrapper1()
{
    if (tickerTestInstance != nullptr)
    {
        tickerTestInstance->MockTickCallback1();
    }
}

void TickCallbackWrapper2()
{
    if (tickerTestInstance != nullptr)
    {
        tickerTestInstance->MockTickCallback2();
    }
}

void TickCallbackWrapperAbort()
{
    if (tickerTestInstance != nullptr)
    {
        tickerTestInstance->MockTickCallbackAbort();
    }
}



using namespace GameKit::Tests::Utils;
TEST_F(GameKitUtilsTickerTestFixture, Ticker_Executecallback_Success)
{
    // arrange
    GameKit::Utils::Ticker* t = new GameKit::Utils::Ticker(1, TickCallbackWrapper1, TestLogger::Log);

    // act
    // the Ticker will execute every second for 4 seconds. At each tick, it will add an item
    // to the std::vector "callBacks". All ticks will be executed, Stop() will wait for thread completion.
    t->Start();
    std::this_thread::sleep_for(std::chrono::seconds(4));
    t->Stop();

    // assert
    ASSERT_EQ(4, tickerTestInstance->GetCallbacks1().size());
}

TEST_F(GameKitUtilsTickerTestFixture, Ticker_Abort_Success)
{
    // arrange
    GameKit::Utils::Ticker* t = new GameKit::Utils::Ticker(1, TickCallbackWrapperAbort, TestLogger::Log);
    m_ticker = t;

    // act
    t->Start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    t->Stop();

    // assert
    ASSERT_EQ(1, tickerTestInstance->GetCallbacks1().size());
}

TEST_F(GameKitUtilsTickerTestFixture, SharedTicker_ThreadStopsAfterTickerDestroyed)
{
    // arrange
    std::shared_ptr<GameKit::Utils::Ticker> sharedTicker;
    sharedTicker = Aws::MakeShared<GameKit::Utils::Ticker>("ticker", 1, TickCallbackWrapper1, TestLogger::Log);

    // act
    sharedTicker->Start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    sharedTicker->Stop();

    sharedTicker.reset(new GameKit::Utils::Ticker(1, TickCallbackWrapper2, TestLogger::Log));
    sharedTicker->Start();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    sharedTicker->Stop();

    // assert
    ASSERT_EQ(2, tickerTestInstance->GetCallbacks1().size());
    ASSERT_EQ(3, tickerTestInstance->GetCallbacks2().size());
}
