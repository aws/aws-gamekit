// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/core/utils/ticker.h>
#include <aws/gamekit/core/utils/debug.h>

using namespace GameKit::Logger;
using namespace GameKit::Utils;

#pragma region Constructors/Destructor
Ticker::Ticker(int interval, std::function<void()> tickFunc, FuncLogCallback logCb)
{
    m_interval = interval;
    m_tickFunc = tickFunc;
    m_logCb = logCb;
    m_isRunning = false;
    m_aborted = false;
    m_isCompleted = false;
}

Ticker::~Ticker()
{
    if (m_isRunning)
    {
        this->Stop();
    }
}
#pragma endregion

#pragma region Public Methods
void Ticker::Start()
{
    std::stringstream buffer;
    buffer << "Ticker::Start(): Interval: " << m_interval;
    Logging::Log(m_logCb, Level::Info, buffer.str().c_str(), this);

    m_isRunning = true;
    m_funcThread = std::thread([&]()
    {
        std::chrono::milliseconds pulse{ TICKER_PULSE };
        long long currentInterval = m_interval * 1000ll;
        while (m_isRunning && !m_aborted)
        {
            // Wake the ticker every TICKER_PULSE and count down currentInterval
            std::this_thread::sleep_for(pulse);
            currentInterval -= pulse.count();

            // If currentInterval reaches 0, execute tickFunc
            if (currentInterval <= 0)
            {
                m_threadId = std::this_thread::get_id();
                m_tickFunc();
                m_threadId = std::thread::id();

                // reset currentInterval
                currentInterval = m_interval * 1000ll;
            }
        }
        Logging::Log(m_logCb, Level::Info, "Ticker::Stop(): Ticker loop exited.", this);

        // notify Stop() method that this thread completed
        m_isCompleted = true;
        m_completedVar.notify_one();
    });

    Logging::Log(m_logCb, Level::Info, "Ticker::Start(): Ticker loop started.", this);
}

void Ticker::Stop()
{
    Logging::Log(m_logCb, Level::Info, "Ticker::Stop()", this);

    // notify thread that we're stopping
    {
        std::lock_guard<std::mutex> lock(m_tickerMutex);
        Logging::Log(m_logCb, Level::Info, "Ticker::Stop(): Stopping...", this);
        m_isRunning = false;
    }

    m_funcThread.join();

    {
        // wait for thread to send completion signal
        std::unique_lock<std::mutex> lock(m_tickerMutex);
        m_completedVar.wait(lock, [&] { return m_isCompleted; });
    }

    // reset the flag in case the Ticker is restarted
    m_isCompleted = false;
    Logging::Log(m_logCb, Level::Info, "Ticker::Stop(): Stopped.", this);
    m_logCb = nullptr;
}

bool Ticker::IsRunning() const
{
    return m_isRunning;
}

void Ticker::AbortLoop()
{
    // Check that this is called inside the tick function
    GameKitInternalAssert(std::this_thread::get_id() == m_threadId);

    Logging::Log(m_logCb, Level::Info, "Ticker::AbortLoop(): Aborting ticker loop.", this);
    m_aborted = true;
}


void Ticker::RescheduleLoop(int newInterval)
{
    // Check that this is called inside the tick function
    GameKitInternalAssert(std::this_thread::get_id() == m_threadId);

    m_interval = newInterval;

    std::stringstream buffer;
    buffer << "Ticker::RescheduleLoop(): Interval: " << m_interval;
    Logging::Log(m_logCb, Level::Info, buffer.str().c_str(), this);
}
#pragma endregion
