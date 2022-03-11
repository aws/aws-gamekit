// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <string>

namespace GameKit
{
    // Marshalable struct to be used with P/Invoke
    struct AccountCredentials
    {
        const char* region;
        const char* accessKey;
        const char* accessSecret;
        const char* accountId;
    };

    struct AccountCredentialsCopy
    {
        std::string region;
        std::string accessKey;
        std::string accessSecret;
        std::string shortRegionCode;
        std::string accountId;
    };

    inline AccountCredentialsCopy CreateAccountCredentialsCopy(const AccountCredentials& credentials)
    {
        return AccountCredentialsCopy{ credentials.region, credentials.accessKey, credentials.accessSecret };
    }

    inline AccountCredentialsCopy CreateAccountCredentialsCopy(const AccountCredentials& credentials, const std::string& shortRegionCode)
    {
        return AccountCredentialsCopy{ credentials.region, credentials.accessKey, credentials.accessSecret, shortRegionCode };
    }
}
