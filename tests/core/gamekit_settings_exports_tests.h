// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <gtest/gtest.h>
#include <aws/gamekit/core/exports.h>
#include <aws/gamekit/core/gamekit_settings.h>

namespace GameKit
{
    namespace Tests
    {
        namespace GameKitSettingsExport
        {
            void* createSettingsInstance();

            class GameKitSettingsExportTestFixture;
        }
    }
}
