// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// GameKit
#include <aws/gamekit/core/awsclients/default_clients.h>
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/enums.h>
#include <aws/gamekit/core/feature_resources.h>
#include <aws/gamekit/core/gamekit_feature.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/identity/exports.h>
#include <aws/gamekit/identity/facebook_identity_provider.h>
#include <aws/gamekit/identity/federated_identity_provider.h>
#include <aws/gamekit/identity/gamekit_identity.h>
#include <aws/gamekit/identity/gamekit_identity_models.h>

using namespace GameKit::Logger;
using namespace GameKit::Identity;

GAMEKIT_IDENTITY_INSTANCE_HANDLE GameKitIdentityInstanceCreateWithSessionManager(void* sessionManager, FuncLogCallback logCb)
{
    Logging::Log(logCb, Level::Info, "GameKitIdentityInstanceCreateWithSessionManager()");
    GameKit::Authentication::GameKitSessionManager* sessMgr = (GameKit::Authentication::GameKitSessionManager*)sessionManager;
    GameKit::Identity::Identity* identity = new Identity(logCb, sessMgr);
    return (GameKit::GameKitFeature*)identity;
}

unsigned int GameKitIdentityRegister(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::UserRegistration userRegistration)
{
    return ((Identity*)((GameKit::GameKitFeature*)identityInstance))->Register(userRegistration);
}

unsigned int GameKitIdentityConfirmRegistration(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::ConfirmRegistrationRequest request)
{
    return ((Identity*)((GameKit::GameKitFeature*)identityInstance))->ConfirmRegistration(request);
}

unsigned int GameKitIdentityResendConfirmationCode(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::ResendConfirmationCodeRequest request)
{
    return ((Identity*)((GameKit::GameKitFeature*)identityInstance))->ResendConfirmationCode(request);
}

unsigned int GameKitIdentityLogin(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::UserLogin userLogin)
{
    return ((Identity*)((GameKit::GameKitFeature*)identityInstance))->Login(userLogin);
}

unsigned int GameKitIdentityLogout(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance)
{
    return ((Identity*)((GameKit::GameKitFeature*)identityInstance))->Logout();
}

unsigned int GameKitIdentityForgotPassword(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::ForgotPasswordRequest request)
{
    return ((Identity*)((GameKit::GameKitFeature*)identityInstance))->ForgotPassword(request);
}

unsigned int GameKitIdentityConfirmForgotPassword(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::ConfirmForgotPasswordRequest request)
{
    return ((Identity*)((GameKit::GameKitFeature*)identityInstance))->ConfirmForgotPassword(request);
}

unsigned int GameKitIdentityGetUser(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const GameKit::FuncIdentityGetUserResponseCallback responseCallback)
{
    return ((Identity*)((GameKit::GameKitFeature*)identityInstance))->GetUser(dispatchReceiver, responseCallback);
}

void GameKitIdentityInstanceRelease(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance)
{
    delete((Identity*)((GameKit::GameKitFeature*)identityInstance));
}

unsigned int GameKitGetFederatedLoginUrl(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::FederatedIdentityProvider identityProvider, DISPATCH_RECEIVER_HANDLE dispatchReceiver, KeyValueCharPtrCallbackDispatcher responseCallback)
{
    LoginUrlResponseInternal loginResponseInternal;

    if (identityProvider == GameKit::FederatedIdentityProvider::Facebook)
    {
        return ((Identity*)((GameKit::GameKitFeature*)identityInstance))->GetFacebookLoginUrl(dispatchReceiver, responseCallback);
    }

    return GameKit::GAMEKIT_ERROR_INVALID_FEDERATED_IDENTITY_PROVIDER;
}

GAMEKIT_API unsigned int GameKitPollAndRetrieveFederatedTokens(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::FederatedIdentityProvider identityProvider, const char* requestId, int timeout)
{
    Identity* instance = (Identity*)(GameKit::GameKitFeature*)identityInstance;
    std::string encryptedLocation;
    unsigned int result;

    if (identityProvider == GameKit::FederatedIdentityProvider::Facebook)
    {
        result = instance->PollFacebookLoginCompletion(requestId, timeout, encryptedLocation);
        if (encryptedLocation == "" || result != GameKit::GAMEKIT_SUCCESS)
        {
            return result;
        }

        return instance->RetrieveFacebookTokens(encryptedLocation);
    }

    return GameKit::GAMEKIT_ERROR_INVALID_FEDERATED_IDENTITY_PROVIDER;
}

GAMEKIT_API unsigned int GameKitGetFederatedIdToken(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::FederatedIdentityProvider identityProvider, DISPATCH_RECEIVER_HANDLE dispatchReceiver, CharPtrCallback responseCallback)
{
    Identity* instance = (Identity*)(GameKit::GameKitFeature*)identityInstance;

    if (identityProvider == GameKit::FederatedIdentityProvider::Facebook)
    {
        auto accessToken = instance->GetSessionManager()->GetToken(GameKit::TokenType::IdToken);
        if (!(dispatchReceiver == nullptr) && !(responseCallback == nullptr))
        {
            responseCallback(dispatchReceiver, accessToken.c_str());
            return GameKit::GAMEKIT_SUCCESS;
        }
    }

    return GameKit::GAMEKIT_ERROR_INVALID_FEDERATED_IDENTITY_PROVIDER;
}
