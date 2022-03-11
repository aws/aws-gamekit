// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard library
#include <iostream>
#include <string>

// AWS SDK
#include <aws/core/auth/AWSAuthSigner.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/utils/UUID.h>
#include <aws/core/utils/base64/Base64.h>
#include <aws/core/utils/json/JsonSerializer.h>

// GameKit
#include <aws/gamekit/achievements/gamekit_achievements_models.h>
#include <aws/gamekit/authentication/gamekit_session_manager.h>
#include <aws/gamekit/core/aws_region_mappings.h>
#include <aws/gamekit/core/exports.h>
#include <aws/gamekit/core/gamekit_feature.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/utils/encoding_utils.h>
#include <aws/gamekit/core/utils/sts_utils.h>

// Boost Forward declarations
namespace boost { namespace filesystem { class path; } }

using namespace GameKit::Logger;

namespace GameKit
{
    class IAchievementsFeature
    {
    public:
        IAchievementsFeature() {};
        virtual ~IAchievementsFeature() {};

        virtual unsigned int ListAchievementsForPlayer(unsigned int pageSize, bool waitForAllPages, DISPATCH_RECEIVER_HANDLE dispatchReceiver, CharPtrCallback responseCallback) = 0;
        virtual unsigned int UpdateAchievementForPlayer(const char* achievementId, unsigned int incrementBy, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback) = 0;
        virtual unsigned int GetAchievementForPlayer(const char* achievementId, DISPATCH_RECEIVER_HANDLE dispatchReceiver, CharPtrCallback responseCallback) = 0;
    };

    namespace Achievements
    {
        class Achievements : GameKitFeature, IAchievementsFeature
        {
        private:
            Authentication::GameKitSessionManager* m_sessionManager;
            std::shared_ptr<Aws::Http::HttpClient> m_httpClient;

            unsigned int processResponse(const std::shared_ptr<Aws::Http::HttpResponse>& response, const std::string& originMethod, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback, Aws::Utils::Json::JsonValue& outJsonValue) const;
        public:
            /**
             * @brief Constructor, obtains resource handles and initializes clients.
             *
             * @param logCb Callback function for logging information and errors.
             * @param sessionManager GameKitSessionManager instance that manages tokens and configuration.
            */
            Achievements(FuncLogCallback logCallback, Authentication::GameKitSessionManager* sessionManager);

            /**
             * @brief Destructor, releases resources.
            */
            ~Achievements();

            /**
             * @brief Passes info on the current player's progress for all achievements to a callback function.
             *
             * @param pageSize The number of dynamo records to scan before the callback is called, max 100.
             * @param waitForAllPages Determines if all achievements should be scanned before calling the callback.
             * @param dispatchReceiver Object that responseCallback is a member of.
             * @param responseCallback Callback method to write decoded JSON response with achievement info to.
             * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
            */
            unsigned int ListAchievementsForPlayer(unsigned int pageSize, bool waitForAllPages, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback) override;

            /**
             * @brief Updates the player's progress for a specific achievement in dynamoDB.
             * @details Stateless achievements (E.g. "Complete Campaign") have a completion requirement of 1 increment, which is the default incrementBy value.
             * If called with an incrementBy value of 4 on an achievement like "Eat 10 bananas," it'll move a previous completion rate of 3/10 to 7/10.
             *
             * @param achievementId Struct containing only an achievements ID
             * @param incrementBy How much to progress the specified achievement by.
             * @param dispatchReceiver Object that responseCallback is a member of.
             * @param responseCallback Callback method to write decoded JSON response with achievement info to.
             * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
            */
            unsigned int UpdateAchievementForPlayer(const char* achievementId, unsigned int incrementBy, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback) override;

            /**
             * @brief Passes info about the progress of a specific achievement for the current player to a callback function.
             *
             * @param achievementId Struct containing only an achievements ID
             * @param dispatchReceiver Object that responseCallback is a member of.
             * @param responseCallback Callback method to write decoded JSON response with specific achievement info to.
             * @return GameKit status code, GAMEKIT_SUCCESS on success else non-zero value. Consult errors.h file for details.
            */
            unsigned int GetAchievementForPlayer(const char* achievementId, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback) override;

            /**
             * @brief Getter for session manager object
             *
             * @return Pointer to session manager
            */
            inline Authentication::GameKitSessionManager* GetSessionManager() const
            {
                return m_sessionManager;
            }

            /**
             * @brief Sets the Http client to use for this feature. Useful for injecting during tests.
             *
             * @param httpClient Shared pointer to an http client for this feature to use.
            */
            inline void SetHttpClient(std::shared_ptr<Aws::Http::HttpClient> httpClient)
            {
                m_httpClient = httpClient;
            }
        };
    }
}
