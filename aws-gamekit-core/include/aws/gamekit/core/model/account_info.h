// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <string>
#include <regex>

// GameKit
#include <aws/gamekit/core/model/resource_environment.h>
#include <aws/gamekit/core/utils/encoding_utils.h>

namespace GameKit
{
    // Marshallable struct to be used with P/Invoke
    struct AccountInfo
    {
        const char* environment;
        const char* accountId;
        const char* companyName;
        const char* gameName;
    };

    struct AccountInfoCopy
    {
        ResourceEnvironment environment;
        std::string accountId;
        std::string companyName;
        std::string gameName;
    };

    std::string TruncateAndLower(const std::string& str, const std::regex& pattern);

    AccountInfoCopy GAMEKIT_API CreateAccountInfoCopy(const AccountInfo accountInfo);

    // Method to compose bootstrap bucket name
    std::string GetBootstrapBucketName(const AccountInfoCopy& accountInfo, const std::string& shortRegionCode);
}
