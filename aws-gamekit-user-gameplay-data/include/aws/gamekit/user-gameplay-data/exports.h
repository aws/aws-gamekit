// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

 /** @file
  * @brief The C interface for the User Gameplay Data library.
  *
  * The User Gameplay Data library provides APIs for storing a player's game-related data in the cloud that can be persisted for any number of sessions.
  *
  * ## Singleton
  * The User Gameplay Data library is designed to be used as a singleton. During the life of your program you should only
  * create once instance of the User Gameplay Data class through GameKitUserGameplayDataInstanceCreateWithSessionManager().
  *
  * It's okay to create and initialize another instance of the User Gameplay Data class if your singleton instance is destroyed.
  * Just make sure to initialize the new instance as described below.
  *
  * ## Initialization
  * The User Gameplay Data library must be initialized exactly once by calling GameKitUserGameplayDataInstanceCreateWithSessionManager().
  * This initialization must be done before calling any other Game Saving APIs, and should only be done once per instance of your singleton.
  * Once the instance is properly initialized, any of the APIs in the User Gameplay Data library can be called. 
  *
  * # Bundles
  * Bundle is a construct that lets you create collections of related bundle items. For example if GameKit is being used for a racing game,
  * there may be a Bundle for track times. A single bundle could contain every player's time for each track in the game.
  * Creatively this is up to you as the developer, in this example the Bundle could also be the name of the circuit, "Razzle_Raceway", and contain times for that specific track
  *
  * # Bundle Items
  * A single piece of data consisting of an identifier (Bundle Key) and its saved value (Bundle Value). Each Bundle Item corresponds to an entry for a specific player. 
  * For example, a Bundle may only consist of a single time on a specific racetrack but there will be a corresponding Bundle Item within for each player.
  *
  * # Typing of Bundle Keys in Dynamo DB
  * All Bundle Item Values will be stored in Dynamo DB as a string value, once the value is retrieved from Dynamo DB you are free to convert it back to whatever type fits your needs.
  *
  * # Naming conventions and restrictions
  * Bundle Names and Item Keys must contain between 1 and 512 characters, and may only contain the characters "a - z" and "A - Z", the numbers "0 - 9", and symbols -_.]+
  * Bundle Item Value's only restriction is the value stored must be less than 400 KB (If you are attempting to store a single value that is over 400KB you should consider using the Game Saving feature!) 
  *
  * # Offline Mode
  * If your game is being played without internet, for a long period of time or due to a brief connection error, the User Gameplay Data feature will begin to cache all calls made. 
  * All calls will be stored in a queue and attempted to be made again with an exponential backoff method. If the calls are made successfully they are removed from the queue.
  * PersistToCache() should be called before a user exits the game to ensure that any calls that are left in the queue are saved to a cache file, which will be loaded in next time they play.
  *
  * In order to get offline mode working correctly the following methods must be implement. 
  * LoadFromCache() - Enqueues cached calls and then deletes all calls from the cache file. Retries calls as soon as the retry background thread is started.
  * PersistToCache() - Writes the pending API calls from queue to cache. Should call StopRetryBackgroundThread() before calling PersistToCache() to ensure nothing is being added during the save.
  * StartRetryBackgroundThread() - Starts the background thread that controls when cached calls will be retried. Should be started after loading from cache and before making any API calls.
  * StopRetryBackgroundThread() - Stops the background thread that controls when cached calls will be retried. Should be stopped before modifying the queue, like in PersistToCache().
  *
  * # Successive offline calls to the same Bundle Item
  * If there are calls that should overwrite one another such as two Update calls made to the same Bundle Item. The queue will automatically prune itself and only take the most up to date values.
  */

#pragma once

// GameKit
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/exports.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/utils/gamekit_httpclient_callbacks.h>
#include <aws/gamekit/user-gameplay-data/gamekit_user_gameplay_data_models.h>

 /**
  * @brief GameKit_Gamplay_data instance handle created by calling #GameKitGameplayDataInstanceCreateWithSessionManager()
 */
typedef void* GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE;

typedef void(*FuncListGameplayDataBundlesResponseCallback)(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* responseBundleName);
typedef void(*FuncBundleResponseCallback)(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* responseKey, const char* responseValue);
typedef void(*FuncBundleItemResponseCallback)(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* responseValue);

extern "C"
{
    /**
     * @brief Creates an gameplay data instance, which can be used to access the Gameplay Data API.
     *
     * @details Make sure to call GameKitGameplayDataInstanceRelease to destroy the returned object when finished with it.
     *
     * @param sessionManager Pointer to a session manager object which manages tokens and configuration.
     * @param logCb Callback function for logging information and errors.
     * @return Pointer to the new UserGameplayData instance.
    */
    GAMEKIT_API GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE GameKitUserGameplayDataInstanceCreateWithSessionManager(void* sessionManager, FuncLogCallback logCb);

    /**
     * @brief Applies the settings to the User Gameplay Data Client. Should be called immediately after the instance has been created and before any other API calls.
     *
     * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager().
     * @param settings Configuration for the Gameplay Data API client.
     * @param logCb Callback function for logging information and errors.
    */
    GAMEKIT_API void GameKitSetUserGameplayDataClientSettings(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, GameKit::UserGameplayDataClientSettings settings);

    /**
     * @brief Creates or updates BundleItems within a specific bundle for the calling user.
     *
     * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager().
     * @param userGameplayDataBundle Struct holding the bundle name, bundleItemKeys, bundleItemValues and number of keys in the bundle being added.
     * @param unprocessedItemsReceiver A pointer to an instance of a class where the results will be dispatched to.
     * @param unprocessedItemsCallback A static dispatcher function pointer that receives char* key/value pairs, which contain all items that were not processed.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED: The session manager does not have settings loaded in for the User Gameplay data feature.
     * - GAMEKIT_ERROR_MALFORMED_BUNDLE_NAME: The bundle name of userGameplayDataBundle is malformed. If this error is received, Check the output log for more details on requirements.
     * - GAMEKIT_ERROR_MALFORMED_BUNDLE_ITEM_KEY: At least one of the bundle keys of userGameplayData are malformed. If this error is received, Check the output log for more details on which item keys are not valid.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (AwsGameKitIdentity) before calling this method.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_FAILED: The call made to the backend service has failed.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_DROPPED: The call made to the backend service has been dropped.
     * - GAMEKIT_WARNING_USER_GAMEPLAY_DATA_API_CALL_ENQUEUED: The call made to the backend service has been enqueued as connection may be unhealthy and will automatically be retried.
     * - GAMEKIT_ERROR_GENERAL: The request has failed unknown reason. 
     */
    GAMEKIT_API unsigned int GameKitAddUserGameplayData(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, GameKit::UserGameplayDataBundle userGameplayDataBundle, DISPATCH_RECEIVER_HANDLE unprocessedItemsReceiver, FuncBundleResponseCallback unprocessedItemsCallback);

    /**
     * @brief Gets gameplay data stored for the calling user from all bundles.
     *
     * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager()
     * @param receiver A pointer to an instance of a class where the results will be dispatched to.
     * @param responseCallback A static dispatcher function pointer that receives a list of char* bundle names.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED: The session manager does not have settings loaded in for the User Gameplay data feature.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (AwsGameKitIdentity) before calling this method.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_FAILED: The call made to the backend service has failed.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_DROPPED: The call made to the backend service has been dropped.
     * - GAMEKIT_WARNING_USER_GAMEPLAY_DATA_API_CALL_ENQUEUED: The call made to the backend service has been enqueued as connection may be unhealthy and will automatically be retried.
     * - GAMEKIT_ERROR_PARSE_JSON_FAILED: The response body from the backend could not be parsed successfully
     * - GAMEKIT_ERROR_GENERAL: The request has failed unknown reason.
     */
    GAMEKIT_API unsigned int GameKitListUserGameplayDataBundles(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, DISPATCH_RECEIVER_HANDLE receiver, FuncListGameplayDataBundlesResponseCallback responseCallback);

    /**
     * @brief Gets gameplay data stored for the calling user from a specific bundle.
     *
     * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager().
     * @param bundleName The name of the bundle that should be referenced in DyanmoDB.
     * @param receiver A pointer to an instance of a class where the results will be dispatched to.
     * @param responseCallback A static dispatcher function pointer that receives char* key/value pairs.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED: The session manager does not have settings loaded in for the User Gameplay data feature.
     * - GAMEKIT_ERROR_MALFORMED_BUNDLE_NAME: The bundle name of UserGameplayDataBundleName is malformed. If this error is received, Check the output log for more details on requirements.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (AwsGameKitIdentity) before calling this method.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_FAILED: The call made to the backend service has failed.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_DROPPED: The call made to the backend service has been dropped.
     * - GAMEKIT_WARNING_USER_GAMEPLAY_DATA_API_CALL_ENQUEUED: The call made to the backend service has been enqueued as connection may be unhealthy and will automatically be retried.
     * - GAMEKIT_ERROR_PARSE_JSON_FAILED: The response body from the backend could not be parsed successfully
     * - GAMEKIT_ERROR_GENERAL: The request has failed unknown reason.
     */
    GAMEKIT_API unsigned int GameKitGetUserGameplayDataBundle(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, char* bundleName, DISPATCH_RECEIVER_HANDLE receiver, FuncBundleResponseCallback responseCallback);

    /**
     * @brief Gets a single stored item from a specific bundle for the calling user.
     *
     * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager().
     * @param userGameplayDataBundleItem Struct holding the bundle name and bundle item that should be retrieved.
     * @param receiver A pointer to an instance of a class where the results will be dispatched to.
     * @param responseCallback A static dispatcher function pointer that receives char* key/value pairs.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED: The session manager does not have settings loaded in for the User Gameplay data feature.
     * - GAMEKIT_ERROR_MALFORMED_BUNDLE_NAME: The bundle name in userGameplayDataBundleItem is malformed. If this error is received, Check the output log for more details on requirements.
     * - GAMEKIT_ERROR_MALFORMED_BUNDLE_ITEM_KEY: The bundle key in userGameplayDataBundleItem is malformed. If this error is received, Check the output log for more details on which item keys are not valid.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (AwsGameKitIdentity) before calling this method.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_FAILED: The call made to the backend service has failed.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_DROPPED: The call made to the backend service has been dropped.
     * - GAMEKIT_WARNING_USER_GAMEPLAY_DATA_API_CALL_ENQUEUED: The call made to the backend service has been enqueued as connection may be unhealthy and will automatically be retried.
     * - GAMEKIT_ERROR_GENERAL: The request has failed unknown reason.
     */
    GAMEKIT_API unsigned int GameKitGetUserGameplayDataBundleItem(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, GameKit::UserGameplayDataBundleItem userGameplayDataBundleItem, DISPATCH_RECEIVER_HANDLE receiver, FuncBundleItemResponseCallback responseCallback);

    /**
     * @brief Updates a single item inside of a bundle for the calling user.
     *
     * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager().
     * @param userGameplayDataBundleItemValue Struct holding the bundle name, bundle item, and item data that will replace the existing data.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED: The session manager does not have settings loaded in for the User Gameplay data feature.
     * - GAMEKIT_ERROR_MALFORMED_BUNDLE_NAME: The bundle name in userGameplayDataBundleItem is malformed. If this error is received, Check the output log for more details on requirements.
     * - GAMEKIT_ERROR_MALFORMED_BUNDLE_ITEM_KEY: The bundle key in userGameplayDataBundleItem is malformed. If this error is received, Check the output log for more details on which item keys are not valid.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (AwsGameKitIdentity) before calling this method.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_FAILED: The call made to the backend service has failed.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_DROPPED: The call made to the backend service has been dropped.
     * - GAMEKIT_WARNING_USER_GAMEPLAY_DATA_API_CALL_ENQUEUED: The call made to the backend service has been enqueued as connection may be unhealthy and will automatically be retried.
     * - GAMEKIT_ERROR_GENERAL: The request has failed unknown reason.
     */
    GAMEKIT_API unsigned int GameKitUpdateUserGameplayDataBundleItem(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, GameKit::UserGameplayDataBundleItemValue userGameplayDataBundleItemValue);

    /**
     * @brief Deletes all gameplay data stored for the calling user.
     *
     * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager().
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED: The session manager does not have settings loaded in for the User Gameplay data feature.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (AwsGameKitIdentity) before calling this method.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_FAILED: The call made to the backend service has failed.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_DROPPED: The call made to the backend service has been dropped.
     * - GAMEKIT_WARNING_USER_GAMEPLAY_DATA_API_CALL_ENQUEUED: The call made to the backend service has been enqueued as connection may be unhealthy and will automatically be retried.
     * - GAMEKIT_ERROR_GENERAL: The request has failed unknown reason.
     */
    GAMEKIT_API unsigned int GameKitDeleteAllUserGameplayData(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance);

    /**
     * @brief Deletes all gameplay data stored within a specific bundle for the calling user.
     *
     * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager().
     * @param bundleName The name of the bundle to be deleted.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED: The session manager does not have settings loaded in for the User Gameplay data feature.
     * - GAMEKIT_ERROR_MALFORMED_BUNDLE_NAME: The bundle name in UserGameplayDataBundleName is malformed. If this error is received, Check the output log for more details on requirements.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (AwsGameKitIdentity) before calling this method.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_FAILED: The call made to the backend service has failed.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_DROPPED: The call made to the backend service has been dropped.
     * - GAMEKIT_WARNING_USER_GAMEPLAY_DATA_API_CALL_ENQUEUED: The call made to the backend service has been enqueued as connection may be unhealthy and will automatically be retried.
     * - GAMEKIT_ERROR_GENERAL: The request has failed unknown reason.
     */
    GAMEKIT_API unsigned int GameKitDeleteUserGameplayDataBundle(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, char* bundleName);

    /**
     * @brief Deletes a single gameplay data item for the calling user.
     *
     * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager()
     * @param deleteItemsRequest Struct holding the bundle name and the bundle items that should be deleted.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED: The session manager does not have settings loaded in for the User Gameplay data feature.
     * - GAMEKIT_ERROR_MALFORMED_BUNDLE_NAME: The bundle name in userGameplayDataBundleItemsDeleteRequest is malformed. If this error is received, Check the output log for more details on requirements.
     * - GAMEKIT_ERROR_MALFORMED_BUNDLE_ITEM_KEY: The bundle key in userGameplayDataBundleItemsDeleteRequest is malformed. If this error is received, Check the output log for more details on which item keys are not valid.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (AwsGameKitIdentity) before calling this method.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_FAILED: The call made to the backend service has failed.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_DROPPED: The call made to the backend service has been dropped.
     * - GAMEKIT_WARNING_USER_GAMEPLAY_DATA_API_CALL_ENQUEUED: The call made to the backend service has been enqueued as connection may be unhealthy and will automatically be retried.
     * - GAMEKIT_ERROR_GENERAL: The request has failed unknown reason.
     */
    GAMEKIT_API unsigned int GameKitDeleteUserGameplayDataBundleItems(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, GameKit::UserGameplayDataDeleteItemsRequest deleteItemsRequest);

    /**
    * @brief Destroys the passed in gameplay data instance.
    *
    * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager()
    */
    GAMEKIT_API void GameKitUserGameplayDataInstanceRelease(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance);

    /**
    * @brief Start the Retry background thread.
    *
    * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager()
    */
    GAMEKIT_API void GameKitUserGameplayDataStartRetryBackgroundThread(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance);

    /**
     * @brief Stop the Retry background thread.
     *
     * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager()
     */
    GAMEKIT_API void GameKitUserGameplayDataStopRetryBackgroundThread(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance);

    /**
     * @brief Set the callback to invoke when the network state changes.
     *
     * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager()
     * @param receiverHandle A pointer to an instance of a class to notify when the network state changes.
     * @param statusChangeCallback Callback function for notifying network state changes: Connection Ok (true) or in Error State (false).
     */
    GAMEKIT_API void GameKitUserGameplayDataSetNetworkChangeCallback(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, NETWORK_STATE_RECEIVER_HANDLE receiverHandle, NetworkStatusChangeCallback statusChangeCallback);

    /**
     * @brief Set the callback to invoke when the offline cache has finished processing.
     *
     * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager()
     * @param receiverHandle A pointer to an instance of a class to notify when the offline cache is finished processing.
     * @param cacheProcessedCallback Callback function for notifying when the offline cache has finished processing: Finished Successfully (true) or in Error State (false).
     */
    GAMEKIT_API void GameKitUserGameplayDataSetCacheProcessedCallback(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, CACHE_PROCESSED_RECEIVER_HANDLE receiverHandle, CacheProcessedCallback cacheProcessedCallback);

    /**
     * @brief Helper that deletes all of the users cached events from the current queues.
     *
     * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager()
     */
    GAMEKIT_API void GameKitUserGameplayDataDropAllCachedEvents(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance);

    /**
     * @brief Write the pending API calls to cache.
     * Pending API calls are requests that could not be sent due to network being offline or other failures.
     * The internal queue of pending calls is cleared. It is recommended to stop the background thread before calling this method.
     *
     * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager()
     * @param offlineCacheFile path to the offline cache file.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_CACHE_WRITE_FAILED: There was an issue writing the queue to the offline cache file.
     */
    GAMEKIT_API unsigned int GameKitUserGameplayDataPersistApiCallsToCache(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, const char* offlineCacheFile);

    /**
     * @brief Read the pending API calls from cache.
     * The calls will be enqueued and retried as soon as the Retry background thread is started and network connectivity is up.
     * The contents of the cache are deleted.
     *
     * @param userGameplayDataInstance Pointer to GameKitGameplayData instance created with GameKitGameplayDataInstanceCreateWithSessionManager()
     * @param offlineCacheFile path to the offline cache file.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_USER_GAMEPLAY_DATA_CACHE_READ_FAILED: There was an issue loading the offline cache file to the queue.
     */
    GAMEKIT_API unsigned int GameKitUserGameplayDataLoadApiCallsFromCache(GAMEKIT_USER_GAMEPLAY_DATA_INSTANCE_HANDLE userGameplayDataInstance, const char* offlineCacheFile);
}
