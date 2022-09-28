// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <vector>
#include <string>
#include <map>

namespace TestFileSystemUtils
{
    std::ptrdiff_t CountFilesInDirectory(const std::string& path);
    std::map<std::string, std::ptrdiff_t> CountFilesInDirectories(const std::vector<std::string>& directories);
    size_t DeleteDirectory(const std::string& path);
};

namespace TestExecutionSettings
{
    struct CustomTestExecutionSettings
    {
        bool AbortOnFailure = false;
        std::vector<std::string> DirectoriesToWatch;
        std::map<std::string, std::ptrdiff_t> InitialFileCount;
    };

    extern CustomTestExecutionSettings Settings;
};

namespace TestExecutionUtils
{
    void AbortOnFailureIfEnabled();
};