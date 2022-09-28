// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// GameKit
#include <aws/gamekit/core/logging.h>

// GTest
#include <gtest/gtest.h>

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

    static void DumpToConsole(const std::string& header, bool isError)
    {
        std::ostream& os = isError ? std::cerr : std::cout;
        size_t prefix = std::hash<std::string>{}(header);
        os << prefix << '|' << "TestLog::DumpToConsole() for " << header << " start" << std::endl;
        for (const std::string& line : TestLog::logLines)
        {
            os << prefix << '|' << line << std::endl;
        }
        os << prefix << '|' << "TestLog::DumpToConsole() for " << header << " end" << std::endl;
    }

    static void DumpToConsole(const testing::TestInfo* test_info, bool isError)
    {
        std::stringstream testName;
        testName << test_info->test_suite_name() << "." << test_info->name();

        DumpToConsole(testName.str(), isError);
    }

    static void DumpToConsole(bool isError = false)
    {
        DumpToConsole("(unknown)", "", isError);
    }

    static void DumpToConsoleIfTestFailed()
    {
        if (::testing::Test::HasFailure())
        {
            const testing::TestInfo* test_info = testing::UnitTest::GetInstance()->current_test_info();
            DumpToConsole(test_info, true);
        }
    }

    static void Clear()
    {
        TestLog::logLines.clear();
    }
};

template <typename T>
std::vector<std::string> TestLog<T>::logLines;