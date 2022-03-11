// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// AWS SDK
#include <aws/core/utils/json/JsonSerializer.h>

// GameKit
#include <aws/gamekit/achievements/gamekit_achievements_models.h>

Aws::Utils::Json::JsonValue GameKit::Achievement::ToJson() const
{
    Aws::Utils::Json::JsonValue json;

    json.WithString("achievement_id", achievementId);
    json.WithString("title", title);
    json.WithString("locked_description", lockedDescription);
    json.WithString("unlocked_description", unlockedDescription);

    json.WithString("locked_icon_url", lockedIcon);
    json.WithString("unlocked_icon_url", unlockedIcon);

    json.WithInteger("max_value", requiredAmount);
    json.WithInteger("points", points);
    json.WithInteger("order_number", orderNumber);

    json.WithBool("is_stateful", isStateful);
    json.WithBool("is_secret", isSecret);
    json.WithBool("is_hidden", isHidden);

    return json;
}