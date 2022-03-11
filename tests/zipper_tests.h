// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// GameKit
#include <aws/gamekit/core/gamekit_account.h>
#include <aws/gamekit/core/feature_resources.h>


// GTest
#include <gtest/gtest.h>
namespace GameKit
{
    namespace Tests
    {
        namespace GameKitZipper
        {
            Aws::UniquePtr<GameKit::Zipper> gamekitZipperInstance = nullptr;
            class GameKitZipperTestFixture;
        }
    }
}
