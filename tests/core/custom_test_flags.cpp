// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// GameKit
#include "custom_test_flags.h"
#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>

// GTest
#include <gtest/gtest.h>

namespace TestExecutionSettings
{
    CustomTestExecutionSettings Settings;
}

std::ptrdiff_t TestFileSystemUtils::CountFilesInDirectory(const std::string& path)
{
    return std::count_if(
        boost::filesystem::recursive_directory_iterator(path),
        boost::filesystem::recursive_directory_iterator(),
        static_cast<bool(*)(const boost::filesystem::path&)>(boost::filesystem::is_regular_file));
}

std::map<std::string, std::ptrdiff_t> TestFileSystemUtils::CountFilesInDirectories(const std::vector<std::string>& directories)
{
    std::map<std::string, std::ptrdiff_t> fileCountPerDir;
    for (const std::string& dir : directories)
    {
        fileCountPerDir[dir] = TestFileSystemUtils::CountFilesInDirectory(dir);
    }

    return fileCountPerDir;
}

size_t TestFileSystemUtils::DeleteDirectory(const std::string& path)
{
    if (boost::filesystem::exists(path))
    {
        return boost::filesystem::remove_all(path);
    }

    return 0;
}

void TestExecutionUtils::AbortOnFailureIfEnabled()
{
    if (TestExecutionSettings::Settings.AbortOnFailure && ::testing::Test::HasFailure())
    {
        abort();
    }
}