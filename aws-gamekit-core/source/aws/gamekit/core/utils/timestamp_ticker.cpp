// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/core/utils/timestamp_ticker.h>

using namespace GameKit::Utils;

TimestampTicker::~TimestampTicker()
{
    OnDestroy();
}

#pragma region Protected Methods
void TimestampTicker::startNewInterval(int intervalSeconds)
{
    m_intervalEndTime = std::chrono::steady_clock::now() + std::chrono::seconds(intervalSeconds);
}

void TimestampTicker::countDownInterval(std::chrono::milliseconds sleepTime)
{
    // no-op by design
}

bool TimestampTicker::isIntervalOver() const
{
    return std::chrono::steady_clock::now() >= m_intervalEndTime;
}
#pragma endregion
