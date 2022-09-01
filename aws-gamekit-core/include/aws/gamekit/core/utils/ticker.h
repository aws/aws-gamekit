// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <functional>
#include <chrono>
#include <future>
#include <cstdio>

// GameKit
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/logging.h>

namespace GameKit
{
    namespace Utils
    {
        /**
        * @brief Utility class that calls a function in its own background thread at defined intervals.
        */
        class GAMEKIT_API Ticker
        {
        private:
            static const int TICKER_PULSE = 250;

            std::mutex m_tickerMutex;
            std::condition_variable m_completedVar;
            std::thread::id m_threadId;
            std::thread m_funcThread;
            int m_interval = 0;
            std::function<void()> m_tickFunc;
            FuncLogCallback m_logCb;
            bool m_isRunning;
            bool m_aborted;
            bool m_isCompleted;
            bool m_wasOnDestroyCalled;

        protected:
            /**
             * @brief This method must be called by derived types in their destructor.
             *
             * @details This method performs the destructor logic for this base class. It can't happen during the
             * regular base class destructor (~Ticker()) because it calls Stop() which waits for the background thread
             * to complete and causes an exception to be thrown. The exception happens because during the final loop
             * of the background thread, the abstract method countDownInterval is called which no longer exists on
             * the derived type (because the derived type's destructor has already been called).
             */
            void OnDestroy();

            /**
             * @brief Start counting down a new interval.
             * @param intervalSeconds The number of seconds in the new interval.
             */
            virtual void startNewInterval(int intervalSeconds) = 0;

            /**
             * @brief Count down the current interval.
             * @param sleepTime The amount of time the background thread slept before calling this method.
             * This value does not include any time that passed while the device was sleeping or hibernating.
             */
            virtual void countDownInterval(std::chrono::milliseconds sleepTime) = 0;

            /**
             * @brief Check if the interval is over.
             * @return Return true if the current interval is over, or false if still counting down.
             */
            virtual bool isIntervalOver() const = 0;

        public:
            /**
            * @brief Create a new instance of this class.
            * @param interval The interval in seconds.
            * @param tickFunc The function to call at the end of every interval.
            * @param logCb The log callback function.
            */
            Ticker(int interval, std::function<void()> tickFunc, FuncLogCallback logCb);

            /**
            * 
            */
            virtual ~Ticker();

            /**
            * @brief Start the ticker loop in a background thread.
            *
            * @details Each ticker instance only supports one background thread running at a time.
            * If Start() is called while the ticker is already running, a warning will be logged and no new thread will be started.
            */
            void Start();

            /**
            * @brief Stop the ticker.
            *
            * @details The ticker can be restarted with a new interval by calling Start().
            *
            * @details This method blocks until the background thread finishes terminating.
            */
            void Stop();

            /**
            * @brief Get the running state of the ticker loop.
            * @return True if the ticker is running, false otherwise.
            */
            bool IsRunning() const;

            /**
            * @brief Abort the loop. This should be called inside the tick function.
            *
            * @details Once aborted, the ticker cannot be restarted with Start(). A new ticker must be created.
            *
            * @details This method does not wait for the background thread to finish terminating before it returns.
            */
            void AbortLoop();

            /**
            * @brief Reschedule the loop to a new interval. This should be called inside the tick function.
            * @param newInterval The new interval in seconds.
            */
            void RescheduleLoop(int newInterval);
        };

    }
}
