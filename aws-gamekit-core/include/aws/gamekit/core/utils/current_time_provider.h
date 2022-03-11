// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// AWS SDK
#include <aws/core/utils/DateTime.h>

// GameKit
#include <aws/gamekit/core/api.h>

namespace GameKit
{
    namespace Utils
    {
        /**
         * This class provides a wrapper over the static functions in Aws::Utils::DateTime to enable these functions to be mocked with GTest.
         *
         * See: https://sdk.amazonaws.com/cpp/api/LATEST/class_aws_1_1_utils_1_1_date_time.html
         * See: http://google.github.io/googletest/gmock_cook_book.html#mocking-free-functions
         */
        class GAMEKIT_API ICurrentTimeProvider
        {
        public:
            ICurrentTimeProvider() {}
            virtual ~ICurrentTimeProvider() {}

            /**
             * Get the number of milliseconds since epoch representing this very instant.
             */
            virtual int64_t GetCurrentTimeMilliseconds() const = 0;
        };

        /**
         * Implementation that directly calls the corresponding static methods in Aws::Utils::DateTime:
         * https://sdk.amazonaws.com/cpp/api/LATEST/class_aws_1_1_utils_1_1_date_time.html
         */
        class GAMEKIT_API AwsCurrentTimeProvider : public ICurrentTimeProvider
        {
        public:

            int64_t GetCurrentTimeMilliseconds() const override
            {
                return Aws::Utils::DateTime::CurrentTimeMillis();
            }
        };
    }
}