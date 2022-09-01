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
  * @brief GameKitAchievements instance handle created by calling GameKitAchievementsInstanceCreateWithSessionManager()
 */
typedef void* GAMEKIT_ACHIEVEMENTS_INSTANCE_HANDLE;

extern "C"
{
    /**
     * @brief Creates an achievements instance, which can be used to access the Achievements API.
     *
     * @details Make sure to call GameKitAchievementsInstanceRelease to destroy the returned object when finished with it.
     *
     * @param sessionManager Pointer to a session manager object which manages tokens and configuration.
     * @param logCb Callback function for logging information and errors.
     * @return Pointer to the new Achievements instance.
    */
    GAMEKIT_API GAMEKIT_ACHIEVEMENTS_INSTANCE_HANDLE GameKitAchievementsInstanceCreateWithSessionManager(void* sessionManager, FuncLogCallback logCb);

    /**
     * @brief Passes info on the current player's progress for all achievements to a callback function.
     *
     * @param achievementsInstance Pointer to GameKit::Achievements instance created with GameKitAchievementsInstanceCreateWithSessionManager()
     * @param pageSize The number of dynamo records to scan before the callback is called, max 100.
     * @param waitForAllPages Determines if all achievements should be scanned before calling the callback.
     * @param dispatchReceiver Object that responseCallback is a member of.
     * @param responseCallback Callback method to write decoded JSON response with achievement info to.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (AwsGameKitIdentity) before calling this method.
     * - GAMEKIT_ERROR_HTTP_REQUEST_FAILED: The backend HTTP request failed. Check the logs to see what the HTTP response code was.
     * - GAMEKIT_ERROR_PARSE_JSON_FAILED: The backend returned a malformed JSON payload. This should not happen. If it does, it indicates there is a bug in the backend code.
     * - GAMEKIT_ERROR_SETTINGS_MISSING: One or more settings required for calling the backend are missing and the backend wasn't called. Verify the feature is deployed and the config is correct.
    */
    GAMEKIT_API unsigned int GameKitListAchievements(GAMEKIT_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance, unsigned int pageSize, bool waitForAllPages, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback);

    /**
     * @brief Updates the player's progress for a specific achievement in dynamoDB.
     * @details Stateless achievements have a completion requirement of 1 increment which is the default increment value E.g. "Complete Campaign."
     * If called with an increment value of 4 on an achievement like "Eat 10 bananas," it'll move it's a previous completion rate of 3/10 to 7/10.
     *
     * @param achievementsInstance Pointer to GameKit::Achievements instance created with GameKitAchievementsInstanceCreateWithSessionManager()
     * @param achievementsId Struct containing only an achievements ID
     * @param incrementBy How much to progress the specified achievement by.
     * @param dispatchReceiver Object that responseCallback is a member of.
     * @param responseCallback Callback method to write decoded JSON response with achievement info to.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (AwsGameKitIdentity) before calling this method.
     * - GAMEKIT_ERROR_HTTP_REQUEST_FAILED: The backend HTTP request failed. Check the logs to see what the HTTP response code was.
     * - GAMEKIT_ERROR_PARSE_JSON_FAILED: The backend returned a malformed JSON payload. This should not happen. If it does, it indicates there is a bug in the backend code.
     * - GAMEKIT_ERROR_SETTINGS_MISSING: One or more settings required for calling the backend are missing and the backend wasn't called. Verify the feature is deployed and the config is correct.
    */
    GAMEKIT_API unsigned int GameKitUpdateAchievement(GAMEKIT_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance, const char* achievementsId, unsigned int incrementBy, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback);

    /**
     * @brief Passes info about the progress of a specific achievement for the current player to a callback function.
     *
     * @param achievementsInstance Pointer to GameKit::Achievements instance created with GameKitAchievementsInstanceCreateWithSessionManager()
     * @param achievementsId Struct containing only an achievements ID
     * @param dispatchReceiver Object that responseCallback is a member of.
     * @param responseCallback Callback method to write decoded JSON response with specific achievement info to.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in. You must login the player through the Identity & Authentication feature (AwsGameKitIdentity) before calling this method.
     * - GAMEKIT_ERROR_HTTP_REQUEST_FAILED: The backend HTTP request failed. Check the logs to see what the HTTP response code was.
     * - GAMEKIT_ERROR_PARSE_JSON_FAILED: The backend returned a malformed JSON payload. This should not happen. If it does, it indicates there is a bug in the backend code.
     * - GAMEKIT_ERROR_ACHIEVEMENTS_INVALID_ID: The Achievement ID given is empty or malformed.
     * - GAMEKIT_ERROR_SETTINGS_MISSING: One or more settings required for calling the backend are missing and the backend wasn't called. Verify the feature is deployed and the config is correct.
    */
    GAMEKIT_API unsigned int GameKitGetAchievement(GAMEKIT_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance, const char* achievementId,
        const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback);

    /**
     * @brief Retrieve base url for achievement icons.
     * @param achievementsInstance Pointer to GameKit::Achievements instance created with GameKitAchievementsInstanceCreateWithSessionManager()
     * @param dispatchReceiver Object that responseCallback is a member of.
     * @param responseCallback Callback method to write the results
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
    */
    GAMEKIT_API unsigned int GameKitGetAchievementIconsBaseUrl(GAMEKIT_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback);

    /**
     * @brief Destroys the passed in achievements instance.
     *
     * @param achievementsInstance Pointer to GameKit::Achievements instance created with GameKitAchievementsInstanceCreateWithSessionManager()
    */
    GAMEKIT_API void GameKitAchievementsInstanceRelease(GAMEKIT_ACHIEVEMENTS_INSTANCE_HANDLE achievementsInstance);
}
