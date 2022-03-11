// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// GameKit
#include <aws/gamekit/core/logging.h>

// Standard Library
#include <vector>
#include <string>
#include <chrono>

/*
Helper class to capture GameKit logs during tests.
*/
template <typename T>
class TestLog
{
private:
    static std::vector<std::string> logLines;

public:
    static void Log(unsigned int level, const char* message, int size)
    {
        long long nowMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        std::string formattedMessage =
            std::to_string(nowMilliseconds)
            .append("|")
            .append(std::to_string(level))
            .append("|")
            .append(message);

        TestLog::logLines.push_back(formattedMessage);
    }

    static const std::vector<std::string>& GetLogLines()
    {
        return TestLog::logLines;
    }

    static void DumpToConsole()
    {
        for (const std::string& line : TestLog::logLines)
        {
            std::cout << line << std::endl;
        }
    }

    static void Clear()
    {
        TestLog::logLines.clear();
    }
};

template <typename T>
std::vector<std::string> TestLog<T>::logLines;