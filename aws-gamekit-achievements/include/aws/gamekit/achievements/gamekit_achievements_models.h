// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// AWS SDK Forward Declaration
namespace Aws { namespace Utils { namespace Json { class JsonValue; class JsonView; } } }

namespace GameKit
{

    struct Achievement
    {
        /**
         * @brief Unique identifier for the achievement
         */
        const char* achievementId;

        /**
         * @brief Title for the achievement, can be used for display purposes.
         */
        const char* title;

        /**
         * @brief Description that should show if the achievement is unearned and/or secret.
         */
        const char* lockedDescription;

        /**
         * @brief Description that should show after an achievement is earned.
         */
        const char* unlockedDescription;

        /**
         * @brief Icon path that should be concatenated onto base icon url, this icon should
         * show when the achievement is unearned or secret.
         */
        const char* lockedIcon;

        /**
         * @brief Icon path that should be concatenated onto base icon url, this icon should
         * be shown after the achievement is earned.
         */
        const char* unlockedIcon;

        /**
         * @brief The number of steps a player must make on the achievement before it is earned.
         */
        unsigned int requiredAmount;

        /**
         * @brief How many points should be attributed to earning this achievement.
         */
        unsigned int points;

        /**
         * @brief A Number you can use to sort which achievements should be displayed first.
         */
        unsigned int orderNumber;

        /**
         * @brief Describes whether this achievement only requires one step to complete, or multiple.
         */
        bool isStateful;

        /**
         * @brief A flag that can be used to filter out achievements from the player's view.
         */
        bool isSecret;

        /**
         * @brief When hidden players cannot make progress on or earn the achievement, until set to false.
         */
        bool isHidden;

        Aws::Utils::Json::JsonValue ToJson() const;
    };
}
