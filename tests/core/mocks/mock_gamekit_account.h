// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// GTest
#include <gmock/gmock.h>

// GameKit
#include <aws/gamekit/core/gamekit_account.h>

using namespace GameKit;

class MockGameKitAccount : public GameKitAccount
{
public:
    MockGameKitAccount(const AccountInfo& accountInfo, const AccountCredentials& credentials, FuncLogCallback logCallback)
        : GameKitAccount(accountInfo, credentials, logCallback) {}
    ~MockGameKitAccount() {}

    MOCK_METHOD(unsigned int, DeployApiGatewayStage, (), (override));
};