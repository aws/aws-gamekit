// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
// Standard library
#include <stdio.h>
#include <stdlib.h>
#include <string>

// AWS SDK
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/utils/json/JsonSerializer.h>

// GameKit
#include "errors.h"
#include "logging.h"

namespace GameKit
{
    class GameKitFeature
    {
    protected:
        const char* m_featureName = nullptr;
        FuncLogCallback m_logCb = nullptr;
    public:
        GameKitFeature() {};
        virtual ~GameKitFeature() {};
    };
}
