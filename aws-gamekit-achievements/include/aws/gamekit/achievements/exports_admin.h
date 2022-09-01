// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

/** @file
 * @brief The C interface for the Achievements library.
 */

#pragma once

// GameKit
#include <aws/gamekit/achievements/gamekit_achievements_models.h>
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/exports.h>
#include <aws/gamekit/core/model/account_info.h>
#include <aws/gamekit/core/enums.h>
#include <aws/gamekit/core/logging.h>

 /**
  * @brief GameKitAdminAchievements instance handle created by calling GameKitAdminAchievementsInstanceCreateWithSessionManager()
 */
typedef void* GAMEKIT_ADMIN_ACHIEVEMENTS_INSTANCE_HANDLE;

extern "C"
{
    /**
     * @brief Creates an achievements instance, which can be used to access the Achievements API.
     *
     * @details Make sure to call GameKitAchievementsInstanceRelease to destroy the returned object when finished with it.
     *
     * @param sessionManager Pointer to a session manager object which manages tokens and configuration.
     * @param cloudResourcesPath Root path for plugin resources to read config files.
     * @param accountCredentials Struct containing AWS access key, secret access key, region, and accountID.
     * @param accountInfo Struct containing environment and game name
     * @param logCb Callback function for logging information and errors.
     * @return Pointer to the new Achievements instance.
    */
    GAMEKIT_API GAMEKIT_ADMIN_ACHIEVEMENTS_INSTANCE_HANDLE GameKitAdminAchievementsInstanceCreateWithSessionManager(void* sessionManager, const char* cloudResourcesPath, const GameKit::AccountCredentials accountCredentials, const GameKit::AccountInfo accountInfo, FuncLogCallback logCb);

    /**
     * @brief Passes all the metadata for every achievement for the current game and environment to a callback function.
     *
     * @param achievementsInstance Pointer to GameKit::Achievements instance created with GameKitAdminAchievementsInstanceCreateWithSessionManager()
     * @param pageSize The number of dynamo records to scan before the callback is called, max 100.
     * @param waitForAllPages Determines if all achievements should be scanned before calling the callback.
     * @param dispatchReceiver Object that responseCallback is a member of.
     * @param responseCallback Callback function to write decoded JSON response with achievement info to.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_REGION_CODE_CONVERSION_FAILED: The current region isn't in our template of shorthand region codes, unknown if the region is supported.
     * - GAMEKIT_ERROR_SIGN_REQUEST_FAILED: Was unable to sign the internal http request with account credentials and info, possibly because they do not have sufficient permissions.
     * - GAMEKIT_ERROR_HTTP_REQUEST_FAILED: The backend HTTP request failed. Check the logs to see what the HTTP response code was.
     * - GAMEKIT_ERROR_PARSE_JSON_FAILED: The backend returned a malformed JSON payload. This should not happen. If it does, it indicates there is a bug in the backend code.
    */
    GAMEKIT_API unsigned int GameKitAdminListAchievements(GAMEKIT_ADMIN_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance,unsigned int pageSize, bool waitForAllPages, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback);

    /**
     * @brief Adds or updates the achievements table in dynamoDB for the current game and environment to have new metadata items.
     *
     * @details Achievement icons are directly uploaded to AWS S3 from this SDK. When an icon is updated, old icon versions will
     * be removed automatically by the backing lambda function.
     *
     * @param achievementsInstance Pointer to GameKit::Achievements instance created with GameKitAdminAchievementsInstanceCreateWithSessionManager()
     * @param achievements Array of structs containing all the fields and values of an achievements item in dynamoDB.
     * @param batchSize The number of items that achievementsMetadata contains.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_ACHIEVEMENTS_ICON_UPLOAD_FAILED: Was unable to take the local path given of an image and upload it to S3.
     * - GAMEKIT_ERROR_REGION_CODE_CONVERSION_FAILED: The current region isn't in our template of shorthand region codes, unknown if the region is supported.
     * - GAMEKIT_ERROR_SIGN_REQUEST_FAILED: Was unable to sign the internal http request with account credentials and info, possibly because they do not have sufficient permissions.
     * - GAMEKIT_ERROR_HTTP_REQUEST_FAILED: The backend HTTP request failed. Check the logs to see what the HTTP response code was.
     * - GAMEKIT_ERROR_PARSE_JSON_FAILED: The backend returned a malformed JSON payload. This should not happen. If it does, it indicates there is a bug in the backend code.
    */
    GAMEKIT_API unsigned int GameKitAdminAddAchievements(GAMEKIT_ADMIN_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance, const GameKit::Achievement* achievements, unsigned int batchSize);

    /**
     * @brief Deletes the achievements in the table in dynamoDB for the current game and environment specified ID's
     *
     * @param achievementsInstance Pointer to GameKit::Achievements instance created with GameKitAdminAchievementsInstanceCreateWithSessionManager()
     * @param achievementIdentifiers Array of structs containing only the achievement ID, which is used as the partion key in dynamoDB.
     * @param batchSize The number of items achievementIdentifiers contains.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_REGION_CODE_CONVERSION_FAILED: The current region isn't in our template of shorthand region codes, unknown if the region is supported.
     * - GAMEKIT_ERROR_SIGN_REQUEST_FAILED: Was unable to sign the internal http request with account credentials and info, possibly because they do not have sufficient permissions.
     * - GAMEKIT_ERROR_HTTP_REQUEST_FAILED: The backend HTTP request failed. Check the logs to see what the HTTP response code was.
     * - GAMEKIT_ERROR_PARSE_JSON_FAILED: The backend returned a malformed JSON payload. This should not happen. If it does, it indicates there is a bug in the backend code.
     * - GAMEKIT_ERROR_ACHIEVEMENTS_PAYLOAD_TOO_LARGE: The argument list is too large to pass as a query string parameter.
    */
    GAMEKIT_API unsigned int GameKitAdminDeleteAchievements(GAMEKIT_ADMIN_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance, const char* const* achievementIdentifiers, unsigned int batchSize);

    /**
     * @brief Changes the credentials used to sign requests and retrieve session tokens for admin requests.
     *
     * @param accountCredentials Struct containing Aws account credentials.
     * @param accountInfo Struct containing information about Aws account, and your game.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_REGION_CODE_CONVERSION_FAILED: The current region isn't in our template of shorthand region codes, unknown if the region is supported.
    */
    GAMEKIT_API unsigned int GameKitAdminCredentialsChanged(GAMEKIT_ADMIN_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance, const GameKit::AccountCredentials accountCredentials, const GameKit::AccountInfo accountInfo);

    /**
     * @brief Returns whether the achievement ID as invalid characters or length
     *
     * @param achievementId The ID to check.
     * @return Returns true if the achievement ID is valid else false.
    */
    GAMEKIT_API bool GameKitIsAchievementIdValid(const char* achievementId);

    /**
     * @brief Destroys the passed in achievements instance.
     *
     * @param achievementsInstance Pointer to GameKit::AdminAchievements instance created with GameKitAdminAchievementsInstanceCreateWithSessionManager()
    */
    GAMEKIT_API void GameKitAdminAchievementsInstanceRelease(GAMEKIT_ADMIN_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance);
}
