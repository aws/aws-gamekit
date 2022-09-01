// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <chrono>
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
        * @details The interval continues to count down when the device is put to sleep or hibernates. The callback
        * tick function is invoked as soon as the device wakes up if the interval elapsed while the device was sleeping/hibernating.
        */
        class GAMEKIT_API TimestampTicker : public Ticker
        {
        private:
            std::chrono::time_point<std::chrono::steady_clock> m_intervalEndTime = std::chrono::time_point<std::chrono::steady_clock>();

        protected:
            void startNewInterval(int intervalSeconds) override;
            void countDownInterval(std::chrono::milliseconds sleepTime) override;
            bool isIntervalOver() const override;

        public:
            TimestampTicker(int interval, std::function<void()> tickFunc, FuncLogCallback logCb)
                : Ticker(interval, tickFunc, logCb) {}

            ~TimestampTicker() override;
        };
    }
}
