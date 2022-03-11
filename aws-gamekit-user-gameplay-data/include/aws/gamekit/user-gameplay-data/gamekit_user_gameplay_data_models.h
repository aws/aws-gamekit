// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// AWS SDK Forward Declarations
namespace Aws {
  namespace Utils {
    namespace Json { class JsonValue; }
  }
}

namespace GameKit
{
    /**
     *@struct UserGameplayDataBundle
     *@brief Struct that stores information about a bundle
     *
     *@var bundleName The name of the bundle
     *@var bundleItemKeys The item keys associated with this bundle
     *@var bundleItemValues The values corresponding to each item key
     * BundleItemValues can be converted back to any data type once they are retrieved from DynamoDB
     *@var numKeys The number of keys that are being referenced in this bundle.
    */
    struct UserGameplayDataBundle
    {
        const char* bundleName;
        const char* const* bundleItemKeys;
        const char* const* bundleItemValues;
        size_t numKeys;

        void GAMEKIT_API ToJson(Aws::Utils::Json::JsonValue& json) const;
    };

    /**
     *@struct UserGameplayDataBundleItem
     *@brief Struct that stores information needed to reference a single item contained in a bundle
     *
     *@var bundleName The name of the bundle being referenced
     *@var bundleItemKey The key of the item being referenced within the bundle
    */
    struct UserGameplayDataBundleItem
    {
        const char* bundleName;
        const char* bundleItemKey;
    };

    /**
     *@struct UserGameplayDataBundleItemValue
     *@brief Struct that stores information needed to update a single item contained in a bundle
     *
     *@var bundleName The name of the bundle that contains the item being updated
     *@var bundleItemKey The key of the item being updated
     *@var bundleItemValue The new char* value that should be associated with the bundleItemId for the calling user
    */
    struct UserGameplayDataBundleItemValue
    {
        const char* bundleName;
        const char* bundleItemKey;
        const char* bundleItemValue;

        void GAMEKIT_API ToJson(Aws::Utils::Json::JsonValue& json);
    };

    /**
     *@struct UserGameplayDataDeleteItemsRequest
     *@brief Struct that stores information needed to update a single item contained in a bundle
     *
     *@var bundleName The name of the bundle that contains the item being updated
     *@var bundleItemKey The key of the items being deleted
    */
    struct UserGameplayDataDeleteItemsRequest
    {
        const char* bundleName;
        const char** bundleItemKeys;
        size_t numKeys;

        void GAMEKIT_API ToJson(Aws::Utils::Json::JsonValue& json) const;
    };


    /**
     * @brief Settings for the User Gameplay Data API client.
     *
     */
    struct UserGameplayDataClientSettings
    {
        /**
         * @brief Connection timeout in seconds for the internal HTTP client. Default is 3. Uses default if set to 0.
         */
        unsigned int ClientTimeoutSeconds;

        /**
         * @brief Seconds to wait between retries. Default is 5. Uses default value if set to 0.
         */
        unsigned int RetryIntervalSeconds;

        /**
         * @brief Maximum length of custom http client request queue size. Once the queue is full, new requests will be dropped. Default is 256. Uses default if set to 0.
         */
        unsigned int MaxRetryQueueSize;

        /**
         * @brief Maximum number of times to retry a request before dropping it. Default is 32. Uses default if set to 0.
         */
        unsigned int MaxRetries;

        /**
         * @brief Retry strategy to use. Use 0 for Exponential Backoff, 1 for Constant Interval. Default is 0.
         */
        unsigned int RetryStrategy;

        /**
         * @brief Maximum retry threshold for Exponential Backoff. Forces a retry even if exponential backoff is set to a greater value. Default is 32. Uses default if set to 0.
         */
        unsigned int MaxExponentialRetryThreshold;

        /**
         * @brief Number of items to retrieve when executing paginated calls such as Get All Data. Default is 100. Uses default if set to 0.
         */
        unsigned int PaginationSize;
    };
}
