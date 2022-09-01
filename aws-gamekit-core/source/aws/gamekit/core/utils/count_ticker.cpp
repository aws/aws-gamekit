// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/core/utils/count_ticker.h>

using namespace GameKit::Utils;

CountTicker::~CountTicker()
{
    OnDestroy();
}

#pragma region Protected Methods
void CountTicker::startNewInterval(int intervalSeconds)
{
    m_intervalTimeLeft = std::chrono::seconds(intervalSeconds);
}

void CountTicker::countDownInterval(std::chrono::milliseconds sleepTime)
{
    m_intervalTimeLeft -= sleepTime;
}

bool CountTicker::isIntervalOver() const
{
    return m_intervalTimeLeft.count() <= 0;
}
#pragma endregion
