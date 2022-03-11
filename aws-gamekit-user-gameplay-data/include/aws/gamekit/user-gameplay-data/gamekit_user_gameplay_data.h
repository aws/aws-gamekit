// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <algorithm>
#include <chrono>
#include <deque>
#include <map>
#include <sstream>
#include <string>

// AWS SDK
#include <aws/cognito-idp/CognitoIdentityProviderClient.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>

// GameKit
#include <aws/gamekit/authentication/gamekit_session_manager.h>
#include <aws/gamekit/core/exports.h>
#include <aws/gamekit/core/gamekit_feature.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/awsclients/api_initializer.h>
#include <aws/gamekit/core/awsclients/default_clients.h>
#include <aws/gamekit/core/utils/ticker.h>
#include <aws/gamekit/core/utils/validation_utils.h>
#include <aws/gamekit/user-gameplay-data/exports.h>
#include <aws/gamekit/user-gameplay-data/gamekit_user_gameplay_data_client.h>
#include <aws/gamekit/user-gameplay-data/gamekit_user_gameplay_data_models.h>

using namespace GameKit::Logger;

namespace GameKit
{
    // Forward declaration for test class
    namespace Tests
    {
        class GameKitUserGameplayDataExportsTestFixture;
    }

    class IUserGameplayDataFeature
    {
        public:
            IUserGameplayDataFeature() {};
            virtual ~IUserGameplayDataFeature() {};
            virtual unsigned int AddUserGameplayData(UserGameplayDataBundle userGameplayDataBundle, DISPATCH_RECEIVER_HANDLE unprocessedItemsReceiver, FuncBundleResponseCallback unprocessedItemsCallback) = 0;

            virtual unsigned int ListUserGameplayDataBundles(DISPATCH_RECEIVER_HANDLE receiver, FuncListGameplayDataBundlesResponseCallback responseCallback) = 0;
            virtual unsigned int GetUserGameplayDataBundle(char* bundleName, DISPATCH_RECEIVER_HANDLE receiver, FuncBundleResponseCallback responseCallback) = 0;
            virtual unsigned int GetUserGameplayDataBundleItem(UserGameplayDataBundleItem userGameplayDataBundleItem, DISPATCH_RECEIVER_HANDLE receiver, FuncBundleItemResponseCallback responseCallback) = 0;

            virtual unsigned int UpdateUserGameplayDataBundleItem(UserGameplayDataBundleItemValue userGameplayDataBundleItemValue) = 0;

            virtual unsigned int DeleteAllUserGameplayData() = 0;
            virtual unsigned int DeleteUserGameplayDataBundle(char* bundleName) = 0;
            virtual unsigned int DeleteUserGameplayDataBundleItems(UserGameplayDataDeleteItemsRequest deleteItemsRequest) = 0;
    };

    namespace UserGameplayData
    {
        static const Aws::String HEADER_AUTHORIZATION = "Authorization";
        static const Aws::String BUNDLE_NAME = "bundle_name";
        static const Aws::String BUNDLE_NAMES = "bundle_names";
        static const Aws::String BUNDLE_ITEMS = "bundle_items";
        static const Aws::String BUNDLE_ITEM_KEY = "bundle_item_key";
        static const Aws::String BUNDLE_ITEM_VALUE = "bundle_item_value";
        static const Aws::String ENVELOPE_KEY_DATA = "data";
        static const Aws::String ENVELOPE_KEY_PAGING = "paging";
        static const Aws::String BUNDLE_PAGINATION_KEY = "next_start_key";
        static const Aws::String BUNDLE_PAGINATION_TOKEN = "paging_token";
        static const Aws::String CONSISTENT_READ_KEY = "use_consistent_read";
        static const Aws::String LIMIT_KEY = "limit";
        static const Aws::String UNPROCESSED_ITEMS = "unprocessed_items";

        static const std::string LIST_BUNDLES_PATH = "/bundles";
        static const std::string BUNDLES_PATH_PART = "/bundles/";
        static const std::string BUNDLE_ITEMS_PATH_PART = "/items/";

        class GAMEKIT_API UserGameplayData : public GameKitFeature, public IUserGameplayDataFeature
        {
            private:
                Authentication::GameKitSessionManager* m_sessionManager;
                std::shared_ptr<UserGameplayDataHttpClient> m_customHttpClient;
                UserGameplayDataClientSettings m_clientSettings;

                void initializeClient();
                void setAuthorizationHeader(std::shared_ptr<Aws::Http::HttpRequest> request);
                void setPaginationLimit(std::shared_ptr<Aws::Http::HttpRequest> request, unsigned int paginationLimit);

                /**
                 * @brief Validates that the passed in bundle item keys are properly formatted and do not contain illegal characters
                 * @return True if the keys are valid, false otherwise
                 *
                 * @param bundleItemKeys a string array of keys
                 * @param numKeys the number of keys in the array
                 * @param tempBuffer a temporary buffer to hold the item keys in the original array that are malformed
                 */
                static bool validateBundleItemKeys(const char* const* bundleItemKeys, int numKeys, std::stringstream& tempBuffer);

                /**
                 * @brief Sets the Low Level Http client to use for this feature. Should be used for testing only.
                 *
                 * @param httpClient Shared pointer to an http client for this feature to use.
                 */
                void setHttpClient(std::shared_ptr<Aws::Http::HttpClient> httpClient);

                friend GameKit::Tests::GameKitUserGameplayDataExportsTestFixture;

            public:

                /**
                 * @brief Constructor, obtains resource handles and initializes clients with default settings.
                 *
                 * @param logCb Callback function for logging information and errors.
                 * @param sessionManager GameKitSessionManager instance that manages tokens and configuration.
                */
                UserGameplayData(Authentication::GameKitSessionManager* sessionManager, FuncLogCallback logCb);

                /**
                 * @brief Applies the settings to the internal clients. Should be called immediately after the constructor and before any other API calls.
                 *
                 * @param settings Client settings.
                */
                void SetClientSettings(const UserGameplayDataClientSettings& settings);

                /**
                 * @brief Destructor, releases resources.
                */
                virtual ~UserGameplayData() override;

                /**
                 * @brief Adds or updates multiple items within a single bundle in the user_gameplay_data_bundle_items AWS DynamoDB table.
                 * A bundle item referenced would be updated in the case that the item already exist.
                 *
                 * @param userGameplayDataBundle Struct holding bundle name, array of bundle item keys, array of bundle item values, and the number of keys.
                 * The bundle keys and item values are treated as char* when being entered and read from the table. Conversion should be done before calling GAMEKIT.
                 * @param unprocessedItemsReceiver A pointer to an instance of a class where the results will be dispatched to.
                 * This instance must have a method signature of void ReceiveResult(const char* responseKey, const char* responseValue)
                 * @param unprocessedItemsCallback A static dispatcher function pointer that receives char* key/value pairs, which contain all items that were not processed.
                 * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
                */
                unsigned int AddUserGameplayData(UserGameplayDataBundle userGameplayDataBundle, DISPATCH_RECEIVER_HANDLE unprocessedItemsReceiver, FuncBundleResponseCallback unprocessedItemsCallback) override;

                /**
                 * @brief Gets gameplay data for all bundles associated with a user.
                 * The playerid stored in the session manager's token is used to get all bundles stored in the user_gameplay_data_bundle_items AWS dynamoDB table.
                 * All items are returned in char* format and can be converted to fit their use case after retrieval from DynamoDB.
                 *
                 * @param receiver A pointer to an instance of a class where the results will be dispatched to.
                 * This instance must have a method signature of void ReceiveResult(const char* bundleName).
                 * @param responseCallback  A static dispatcher function pointer that receives a list of char* bundle names.
                 * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
                */
                unsigned int ListUserGameplayDataBundles(DISPATCH_RECEIVER_HANDLE receiver, FuncListGameplayDataBundlesResponseCallback responseCallback) override;

                /**
                 * @brief Gets all items that are associated with a certain bundle for a user.
                 * All items are returned in char* format and can be converted to fit their use case after retrieval from DynamoDB.
                 *
                 * @param bundleName The name of the bundle that is being retrieved.
                 * @param receiver A pointer to an instance of a class where the results will be dispatched to.
                 * This instance must have a method signature of void ReceiveResult(const char* responseKey, const char* responseValue)
                 * @param responseCallback A static dispatcher function pointer that receives char* key/value pairs.
                 * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
                */
                unsigned int GetUserGameplayDataBundle(char* bundleName, DISPATCH_RECEIVER_HANDLE receiver, FuncBundleResponseCallback responseCallback) override;

                /**
                 * @brief Gets a single item that is associated with a certain bundle for a user.
                 * All items are returned in char* format and can be converted to fit their use case after retrieval from DynamoDB.
                 *
                 * @param userGameplayDataBundleItem Struct holding the bundle name and bundle item that should be retrieved.
                 * @param receiver A pointer to an instance of a class where the results will be dispatched to.
                 * This instance must have a method signature of void ReceiveResult(const char* responseValue)
                 * @param responseCallback A static dispatcher function pointer that receives a single char* value.
                 * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
                */
                unsigned int GetUserGameplayDataBundleItem(UserGameplayDataBundleItem userGameplayDataBundleItem, DISPATCH_RECEIVER_HANDLE receiver, FuncBundleItemResponseCallback responseCallback) override;

                /**
                 * @brief Updates the value of an existing item inside a bundle with new item data.
                 *
                 * @param userGameplayDataBundleItemValue Struct holding the bundle name, bundle item, and item data.
                 * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
                */
                unsigned int UpdateUserGameplayDataBundleItem(UserGameplayDataBundleItemValue userGameplayDataBundleItemValue) override;

                /**
                 * @brief Permanently deletes all bundles associated with a user.
                 *
                 * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
                */
                unsigned int DeleteAllUserGameplayData() override;

                /**
                 * @brief Permanently deletes an entire bundle, along with all corresponding items, associated with a user.
                 *
                 * @param bundleName The name of the bundle to be deleted.
                 * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
                */
                unsigned int DeleteUserGameplayDataBundle(char* bundleName) override;

                /**
                 * @brief Permanently deletes an item inside of a bundle associated with a user.
                 *
                 * @param deleteItemsRequest Struct holding the bundle name and the bundle items that should be deleted.
                 * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
                */
                unsigned int DeleteUserGameplayDataBundleItems(UserGameplayDataDeleteItemsRequest deleteItemsRequest) override;

                /**
                 * @brief Start the Retry background thread.
                */
                void StartRetryBackgroundThread();

                /**
                 * @brief Stop the Retry background thread.
                */
                void StopRetryBackgroundThread();

                /**
                 * @brief Set the callback to invoke when the network state changes.
                 *
                 * @param receiverHandle A pointer to an instance of a class to notify when the network state changes.
                 * @param statusChangeCallback Callback function for notifying network state changes.
                */
                void SetNetworkChangeCallback(NETWORK_STATE_RECEIVER_HANDLE receiverHandle, NetworkStatusChangeCallback statusChangeCallback);

                /**
                 * @brief Set the callback to invoke when the offline cache is finished processing.
                 *
                 * @param receiverHandle A pointer to an instance of a class to notify when the offline cache is finished processing.
                 * @param cacheProcessedCallback Callback function for notifying when the offline cache is finished processing.
                */
                void SetCacheProcessedCallback(CACHE_PROCESSED_RECEIVER_HANDLE receiverHandle, CacheProcessedCallback cacheProcessedCallback);

                /**
                 * @brief Helper that deletes all of the users cached events from the current queues.
                */
                void DropAllCachedEvents();

                /**
                 * @brief Write the pending API calls to cache.
                 * Pending API calls are requests that could not be sent due to network being offline or other failures.
                 * The internal queue of pending calls is cleared. It is recommended to stop the background thread before calling this method.
                 *
                 * @param offlineCacheFile path to the offline cache file.
                 * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
                */
                unsigned int PersistApiCallsToCache(const std::string& offlineCacheFile);

                /**
                 * @brief Read the pending API calls from cache.
                 * The calls will be enqueued and retried as soon as the Retry background thread is started and network connectivity is up.
                 * The cache file is deleted after the contents are loaded.
                 *
                 * @param offlineCacheFile path to the offline cache file.
                 * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
                */
                unsigned int LoadApiCallsFromCache(const std::string& offlineCacheFile);
        };
    }
}
