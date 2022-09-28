// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0


#include <aws/gamekit/core/internal/platform_string.h>
#include "user_gameplay_data_models_tests.h"

using namespace GameKit::Tests::DataModels;
using namespace Aws::Utils::Json;

UserGameplayDataModelsTestFixture::UserGameplayDataModelsTestFixture()
{}

UserGameplayDataModelsTestFixture::~UserGameplayDataModelsTestFixture()
{}

void UserGameplayDataModelsTestFixture::SetUp()
{}

void UserGameplayDataModelsTestFixture::TearDown()
{
    TestExecutionUtils::AbortOnFailureIfEnabled();
}

TEST_F(UserGameplayDataModelsTestFixture, Test_SerializeToJson_UserGameplayDataBundle)
{
    GameKit::UserGameplayDataBundle bundle;

    const char* keys[3] = { "Coins", "Food", "Potions" };
    const char* values[3] = { "0", "10", "Red" };
    bundle.bundleName = "PlayerInventory";
    bundle.bundleItemKeys = keys;
    bundle.bundleItemValues = values;
    bundle.numKeys = 3;

    JsonValue json;
    bundle.ToJson(json);
    std::string serialized = ToStdString(json.View().WriteCompact());
    std::string expected = "{\"Coins\":\"0\",\"Food\":\"10\",\"Potions\":\"Red\"}";

    ASSERT_STREQ(serialized.c_str(), expected.c_str());
};


TEST_F(UserGameplayDataModelsTestFixture, Test_SerializeToJson_UserGameplayDataBundleItemValue)
{
    GameKit::UserGameplayDataBundleItemValue bundleItemValue;

    bundleItemValue.bundleName = "PlayerInventory";
    bundleItemValue.bundleItemKey = "Coins";
    bundleItemValue.bundleItemValue = "10";

    JsonValue json;
    bundleItemValue.ToJson(json);
    std::string serialized = ToStdString(json.View().WriteCompact());
    std::string expected = "{\"bundle_item_value\":\"10\"}";

    ASSERT_STREQ(serialized.c_str(), expected.c_str());
};