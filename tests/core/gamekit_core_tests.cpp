// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// GameKit
#include "aws/gamekit/core/logging.h"
#include "custom_test_flags.h"
#include "test_log.h"

// Gtest
#include <gtest/gtest.h>

// Standard Library
#include <string>
#include <vector>
#include <algorithm>

using namespace GameKit::Logger;

class LoggingTestFixture;
static LoggingTestFixture* instance = nullptr;
typedef std::map<unsigned int, std::vector<std::string>> MessageMap;

class LoggingTestFixture : public ::testing::Test
{
public:
    typedef TestLog<LoggingTestFixture> TestLogger;

    LoggingTestFixture()
    {}

    void SetUp()
    {
        instance = this;
        TestLogger::Clear();
    }

    void TearDown()
    {
        instance = nullptr;

        TestLogger::DumpToConsoleIfTestFailed();
        TestLogger::Clear();
        TestExecutionUtils::AbortOnFailureIfEnabled();
    }

    ~LoggingTestFixture()
    {}

    static bool FindInLog(const std::string& str)
    {
        auto findHelper = [&](const std::string& line) 
        { 
            return line.find(str) != std::string::npos; 
        };

        return std::find_if(
            TestLogger::GetLogLines().begin(), 
            TestLogger::GetLogLines().end(), findHelper) != TestLogger::GetLogLines().end();
    }
};

TEST_F(LoggingTestFixture, Null_TestCallback)
{
    Logging::Log(nullptr, Level::Info, "hello");

    EXPECT_EQ(TestLogger::GetLogLines().size(), 0);
}

TEST_F(LoggingTestFixture, ValidCallback_TestCallback)
{
    Logging::Log(TestLogger::Log, Level::Verbose, "hello");

    EXPECT_TRUE(FindInLog("hello"));
    EXPECT_EQ(TestLogger::GetLogLines().size(), 1);
}

TEST_F(LoggingTestFixture, AllLevels_TestCallback)
{
    Logging::Log(TestLogger::Log, Level::None, "None");
    Logging::Log(TestLogger::Log, Level::Verbose, "Verbose");
    Logging::Log(TestLogger::Log, Level::Info, "Info");
    Logging::Log(TestLogger::Log, Level::Warning, "Warning");
    Logging::Log(TestLogger::Log, Level::Error, "Error");

    EXPECT_TRUE(FindInLog("None"));
    EXPECT_TRUE(FindInLog("Verbose"));
    EXPECT_TRUE(FindInLog("Info"));
    EXPECT_TRUE(FindInLog("Warning"));
    EXPECT_TRUE(FindInLog("Error"));
    EXPECT_EQ(TestLogger::GetLogLines().size(), 5);
}
