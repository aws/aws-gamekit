// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <functional>

// GameKit
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/utils/ticker.h>

namespace GameKit
{
    namespace Utils
    {
        /**
        * @brief Utility class that calls a function in its own background thread at defined intervals.
        *
        * @details The interval countdown is paused when the device is put to sleep or hibernates, and resumes when the device wakes up.
        */
        class GAMEKIT_API CountTicker : public Ticker
        {
        private:
            /**
             * @brief The number of milliseconds left in the current interval. The interval ends when this reaches zero.
             */
            std::chrono::milliseconds m_intervalTimeLeft = std::chrono::milliseconds(0);

        protected:
            void startNewInterval(int intervalSeconds) override;
            void countDownInterval(std::chrono::milliseconds sleepTime) override;
            bool isIntervalOver() const override;

        public:
            CountTicker(int interval, std::function<void()> tickFunc, FuncLogCallback logCb)
                : Ticker(interval, tickFunc, logCb) {}

            ~CountTicker() override;
        };
    }
}
