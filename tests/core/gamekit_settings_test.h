// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "test_common.h"
#include "aws/gamekit/core/gamekit_settings.h"
#include "aws/gamekit/core/model/config_consts.h"
#include "aws/gamekit/core/utils/file_utils.h"

namespace GameKit
{
    namespace Tests
    {
        namespace GameKitSettings
        {
            std::unique_ptr<GameKit::GameKitSettings> gamekitSettingsInstance = nullptr;
            class GameKitSettingsTestFixture;
        }
    }
}
