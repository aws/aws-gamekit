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
    m_wasOnDestroyCalled = false;
}

Ticker::~Ticker()
{
    // Derived classes must call OnDestroy() in their destructors.
    if (!m_wasOnDestroyCalled)
    {
        Logging::Log(m_logCb, Level::Error, "Ticker::~Ticker(): OnDestroy() was never called. It must be called in the destructor of Ticker derived types.", this);
    }
}

void GameKit::Utils::Ticker::OnDestroy()
{
    if (m_isRunning)
    {
        this->Stop();
    }

    m_logCb = nullptr;
    m_wasOnDestroyCalled = true;
}
#pragma endregion

#pragma region Public Methods
void Ticker::Start()
{
    // Exit early if the ticker is already running
    {
        std::lock_guard<std::mutex> lock(m_tickerMutex);
        if (m_isRunning)
        {
            Logging::Log(m_logCb, Level::Warning, "Ticker::Start(): This ticker is already running. It can only support one background thread at a time. Skipped starting a new thread.", this);
            return;
        }
    }

    std::stringstream buffer;
    buffer << "Ticker::Start(): Interval: " << m_interval;
    Logging::Log(m_logCb, Level::Info, buffer.str().c_str(), this);

    m_isRunning = true;
    m_funcThread = std::thread([&]()
    {
        std::chrono::milliseconds pulse{ TICKER_PULSE };
        startNewInterval(m_interval);

        while (m_isRunning && !m_aborted)
        {
            // Wake the ticker every TICKER_PULSE and count down the current interval
            std::this_thread::sleep_for(pulse);
            countDownInterval(pulse);

            if (isIntervalOver())
            {
                // execute the tickFunc
                m_threadId = std::this_thread::get_id();
                m_tickFunc();
                m_threadId = std::thread::id();

                // set the next intervalEndTime
                startNewInterval(m_interval);
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
