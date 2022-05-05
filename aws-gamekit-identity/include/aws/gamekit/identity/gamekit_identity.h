// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <iostream>
#include <string>

// AWS SDK
#include <aws/cognito-idp/CognitoIdentityProviderClient.h>
#include <aws/cognito-idp/CognitoIdentityProviderErrors.h>
#include <aws/cognito-idp/model/ConfirmForgotPasswordRequest.h>
#include <aws/cognito-idp/model/ConfirmSignUpRequest.h>
#include <aws/cognito-idp/model/ForgotPasswordRequest.h>
#include <aws/cognito-idp/model/GetUserRequest.h>
#include <aws/cognito-idp/model/GetUserResult.h>
#include <aws/cognito-idp/model/InitiateAuthRequest.h>
#include <aws/cognito-idp/model/ResendConfirmationCodeRequest.h>
#include <aws/cognito-idp/model/RevokeTokenRequest.h>
#include <aws/cognito-idp/model/SignUpRequest.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/utils/UUID.h>
#include <aws/core/utils/base64/Base64.h>
#include <aws/core/utils/json/JsonSerializer.h>

// GameKit
#include <aws/gamekit/authentication/gamekit_session_manager.h>
#include "aws/gamekit/core/exports.h"
#include <aws/gamekit/core/gamekit_feature.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/awsclients/api_initializer.h>
#include <aws/gamekit/core/awsclients/default_clients.h>
#include <aws/gamekit/identity/facebook_identity_provider.h>
#include <aws/gamekit/identity/federated_identity_provider.h>
#include <aws/gamekit/identity/gamekit_identity_models.h>
#include <aws/gamekit/identity/utils/credentials_utils.h>

using namespace GameKit::Logger;
using namespace Aws::Utils::Json;

typedef void(*FuncResponseCallback)(const char* responsePayload, unsigned int size);

namespace GameKit
{
    class IIdentityFeature
    {
    public:
        IIdentityFeature() {};
        virtual ~IIdentityFeature() {};
        virtual unsigned int ConfirmRegistration(ConfirmRegistrationRequest confirmationRequest) = 0;
        virtual unsigned int ResendConfirmationCode(ResendConfirmationCodeRequest resendConfirmationRequest) = 0;
        virtual unsigned int Login(UserLogin userLogin) = 0;
        virtual unsigned int ForgotPassword(ForgotPasswordRequest forgotPasswordRequest) = 0;
        virtual unsigned int ConfirmForgotPassword(ConfirmForgotPasswordRequest confirmForgotPasswordRequest) = 0;
        virtual unsigned int GetUser(const DISPATCH_RECEIVER_HANDLE receiver, const GameKit::FuncIdentityGetUserResponseCallback responseCallback) = 0;
    };

    namespace Identity
    {
        static const Aws::String ATTR_EMAIL = "email";
        static const Aws::String ATTR_CUSTOM_GAMEKIT_USER_ID = "custom:gk_user_id";
        static const Aws::String ATTR_CUSTOM_GAMEKIT_USER_HASH_KEY = "custom:gk_user_hash_key";
        static const Aws::String KEY_FEDERATED_LOGIN_URL_REQUEST_ID = "requestId";
        static const Aws::String KEY_FEDERATED_LOGIN_URL = "loginUrl";

        static const Aws::String USER_ID = "gk_user_id";
        static const Aws::String USER_CREATED_AT = "created_at";
        static const Aws::String USER_UPDATED_AT = "created_at";
        static const Aws::String USER_FB_EXTERNAL_ID = "facebook_external_id";
        static const Aws::String USER_FB_REF_ID = "facebook_ref_id";
        static const Aws::String USER_NAME = "user_name";
        static const Aws::String USER_EMAIL = "email";

        /**
         * @brief See identity/exports.h for most of the documentation.
         */
        class Identity : GameKitFeature, IIdentityFeature
        {
        private:
            Aws::CognitoIdentityProvider::CognitoIdentityProviderClient* m_cognitoClient = nullptr;
            Authentication::GameKitSessionManager* m_sessionManager = nullptr;
            bool m_awsClientsInitializedInternally;
            std::shared_ptr<Aws::Http::HttpClient> m_httpClient;

        public:
            Identity(FuncLogCallback logCallback, Authentication::GameKitSessionManager* sessionManager);
            ~Identity();

            unsigned int Register(UserRegistration userRegistration);
            unsigned int ConfirmRegistration(ConfirmRegistrationRequest confirmationRequest);
            unsigned int ResendConfirmationCode(ResendConfirmationCodeRequest resendConfirmationRequest);
            unsigned int Login(UserLogin userLogin);
            unsigned int Logout();
            unsigned int ForgotPassword(ForgotPasswordRequest forgotPasswordRequest);
            unsigned int ConfirmForgotPassword(ConfirmForgotPasswordRequest confirmForgotPasswordRequest);
            unsigned int GetUser(const DISPATCH_RECEIVER_HANDLE receiver, const GameKit::FuncIdentityGetUserResponseCallback responseCallback);

            /**
             * @brief Gets a Facebook URL, users will be able to sign in when the URL is opened in a browser.
             *
             * @return Struct containing unique request ID and a login URL
            */
            unsigned int GetFacebookLoginUrl(DISPATCH_RECEIVER_HANDLE dispatchReceiver, KeyValueCharPtrCallbackDispatcher responseCallback);

            /**
             * @brief Continually checks if the user has completed signing in at the URL from GetFacebookLoginUrl()
             *
             * @param requestId Unique request identifier from GetFacebookLoginUrl()
             * @param timeout Amount of time in seconds before the request expires.
             * @param encryptedLocation (Out Parameter) that holds path to an AWS S3 bucket that contains encrypted tokens
             * @return The result code of the operation.
             * - GAMEKIT_SUCCESS: The API call was successful.
             * - GAMEKIT_ERROR_REQUEST_TIMED_OUT: PollForCompletion timed out waiting for Facebook login completion.
            */
            unsigned int PollFacebookLoginCompletion(const std::string& requestId, int timeout, std::string& encryptedLocation);

            /**
             * @brief Retrieves and stores authorized tokens from the facebook identity provider in the session manager.
             *
             * @param location Path to an AWS S3 bucket that contains the tokens.
             * @return The result code of the operation.
             * - GAMEKIT_SUCCESS: The API call was successful.
             * - GAMEKIT_ERROR_HTTP_REQUEST_FAILED: Http request to get Facebook Token failed.
            */
            unsigned int RetrieveFacebookTokens(const std::string& location);

            /**
             * @brief Initializes AWS clients as attributes of this object.
             * Clients initialized with this method will be deleted on ~Identity().
            */
            void InitializeDefaultAwsClients();

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
             * @brief Sets the cognito client to utilize for this feature.
             * It's the caller's responsibility to call delete on the instance passed to this method.
             *
             * @param client Pointer to the cognito client you want this feature to utilize.
            */
            inline void SetCognitoClient(Aws::CognitoIdentityProvider::CognitoIdentityProviderClient* client)
            {
                m_awsClientsInitializedInternally = false;
                m_cognitoClient = client;
            }

            /**
             * @brief Sets the Http client to use for this feature.
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
