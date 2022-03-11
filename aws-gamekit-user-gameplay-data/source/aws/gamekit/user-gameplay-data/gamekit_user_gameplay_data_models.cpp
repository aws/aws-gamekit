// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// AWS SDK
#include <aws/core/utils/json/JsonSerializer.h>

// GameKit
#include <aws/gamekit/user-gameplay-data/gamekit_user_gameplay_data_models.h>

void GameKit::UserGameplayDataBundle::ToJson(Aws::Utils::Json::JsonValue& json) const
{
    for (size_t i = 0; i < numKeys; i++)
    {
        json.WithString(bundleItemKeys[i], bundleItemValues[i]);
    }
}

void GameKit::UserGameplayDataDeleteItemsRequest::ToJson(Aws::Utils::Json::JsonValue& json) const
{
    Aws::Utils::Array<Aws::Utils::Json::JsonValue> bundleItemJsonList(numKeys);
    for (size_t i = 0; i < bundleItemJsonList.GetLength(); ++i)
    {
        bundleItemJsonList[i].AsString(bundleItemKeys[i]);
    }

    json.WithArray("bundle_item_keys", std::move(bundleItemJsonList));
}

void GameKit::UserGameplayDataBundleItemValue::ToJson(Aws::Utils::Json::JsonValue& json)
{
    json.WithString("bundle_item_value", bundleItemValue);
}
