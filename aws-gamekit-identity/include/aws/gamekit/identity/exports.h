// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

/** @file
 * @brief The C interface for the Identity library.
 *
 * This library provides APIs for signing players into your game.
 *
 * **Important:** The Identity & Authentication feature is a prerequisite for all other features.
 *
 * You must sign in a player before most GameKit APIs will work. After signing in, GameKit will internally store and
 * refresh the player's access tokens, and pass the access tokens to all API calls that require authentication.
 *
 * ## Login Mechanisms
 * Players can log in through either of two mechanisms:
 * - Email and password, by calling GameKitIdentityLogin().
 * - A federated identity provider's webpage, by calling GameKitGetFederatedLoginUrl() followed by GameKitPollAndRetrieveFederatedTokens().
 *
 * A player is free to switch between either login mechanism. It doesn't matter whether they first register through email and
 * password, or through a federated identity provider.
 *
 * ### Email and Password
 * The following methods support email and password based sign in:
 * - GameKitIdentityRegister()
 * - GameKitIdentityConfirmRegistration()
 * - GameKitIdentityResendConfirmationCode()
 * - GameKitIdentityLogin()
 * - GameKitIdentityLogout()
 * - GameKitIdentityForgotPassword()
 * - GameKitIdentityConfirmForgotPassword()
 *
 * ### Federated Identity Providers
 * The following methods support sign in through a federated identity provider:
 * - GameKitGetFederatedLoginUrl()
 * - GameKitPollAndRetrieveFederatedTokens()
 * - GameKitGetFederatedIdToken()
 * - GameKitIdentityLogout()
 *
 * Note that by signing into the federated identity provider at the webpage provided by GameKitGetFederatedLoginUrl(),
 * the player automatically is registered and confirmed in the Identity & Authentication feature.
 */

#pragma once

// Standard Library
#include <exception>
#include <string>

// GameKit
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/enums.h>
#include <aws/gamekit/core/exports.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/identity/gamekit_identity_models.h>

/**
 * @brief A pointer to an Identity instance created with GameKitIdentityInstanceCreateWithSessionManager().
 */
typedef void* GAMEKIT_IDENTITY_INSTANCE_HANDLE;

extern "C"
{
    /**
     * @brief Create an instance of the Identity class, which can be used to access the other Identity APIs.
     *
     * @details Make sure to call GameKitIdentityInstanceRelease() to destroy the returned object when finished with it, otherwise you'll have a memory leak.
     *
     * @param sessionManager Pointer to a SessionManager instance created with GameKitSessionManagerInstanceCreate().
     * @param logCb A callback function which the Identity instance can use to log information and errors.
     * @return A pointer to the new Identity instance.
    */
    GAMEKIT_API GAMEKIT_IDENTITY_INSTANCE_HANDLE GameKitIdentityInstanceCreateWithSessionManager(void* sessionManager, FuncLogCallback logCb);

    /**
     * @brief Register a new player for email and password based sign in.
     *
     * @details After calling this method, you must call GameKitIdentityConfirmRegistration() to confirm the player's identity.
     *
     * @param identityInstance A pointer to an Identity instance created with GameKitIdentityInstanceCreateWithSessionManager().
     * @param userRegistration A struct containing all parameters required to call this method.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_MALFORMED_USERNAME: The provided userName is malformed. Check the logs to see what the required format is.
     * - GAMEKIT_ERROR_MALFORMED_PASSWORD: The provided password is malformed. Check the logs to see what the required format is.
     * - GAMEKIT_ERROR_METHOD_NOT_IMPLEMENTED: You attempted to register a guest, which is not yet supported. To fix, make sure the request's GameKit::UserRegistration::userId field is empty.
     * - GAMEKIT_ERROR_REGISTER_USER_FAILED: The backend web request failed. Check the logs to see what the error was.
     */
    GAMEKIT_API unsigned int GameKitIdentityRegister(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::UserRegistration userRegistration);

    /**
     * @brief Confirm registration of a new player that was registered through GameKitIdentityRegister().
     *
     * @details The confirmation code is sent to the player's email and can be re-sent by calling GameKitIdentityConfirmRegistration().
     *
     * @param identityInstance A pointer to an Identity instance created with GameKitIdentityInstanceCreateWithSessionManager().
     * @param request A struct containing all parameters required to call this method.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_MALFORMED_USERNAME: The provided userName is malformed. Check the logs to see what the required format is.
     * - GAMEKIT_ERROR_CONFIRM_REGISTRATION_FAILED: The backend web request failed. Check the logs to see what the error was.
     */
    GAMEKIT_API unsigned int GameKitIdentityConfirmRegistration(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::ConfirmRegistrationRequest request);

    /**
     * @brief Resend the registration confirmation code to the player's email.
     *
     * @details This resends the confirmation code that was sent by calling GameKitIdentityRegister() or GameKitIdentityResendConfirmationCode().
     *
     * @param identityInstance A pointer to an Identity instance created with GameKitIdentityInstanceCreateWithSessionManager().
     * @param request A struct containing all parameters required to call this method.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_MALFORMED_USERNAME: The provided userName is malformed. Check the logs to see what the required format is.
     * - GAMEKIT_ERROR_RESEND_CONFIRMATION_CODE_FAILED: The backend web request failed. Check the logs to see what the error was.
     */
    GAMEKIT_API unsigned int GameKitIdentityResendConfirmationCode(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::ResendConfirmationCodeRequest request);

    /**
     * @brief Sign in the player through email and password.
     *
     * @details After calling this method, the player will be signed in and you'll be able to call the other GameKit APIs.
     * This method stores the player's authorized access tokens in the SessionManager, and automatically refreshes them before they expire.
     *
     * @param identityInstance A pointer to an Identity instance created with GameKitIdentityInstanceCreateWithSessionManager().
     * @param userLogin A struct containing all parameters required to call this method.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     */
    GAMEKIT_API unsigned int GameKitIdentityLogin(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::UserLogin userLogin);

    /**
     * @brief Sign out the currently logged in player.
     *
     * @details This revokes the player's access tokens and clears them from the SessionManager.
     *
     * @param identityInstance A pointer to an Identity instance created with GameKitIdentityInstanceCreateWithSessionManager().
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     */
    GAMEKIT_API unsigned int GameKitIdentityLogout(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance);

    /**
     * @brief Send a password reset code to the player's email.
     *
     * @details After calling this method, you must call GameKitIdentityConfirmForgotPassword() to complete the password reset.
     *
     * @param identityInstance A pointer to an Identity instance created with GameKitIdentityInstanceCreateWithSessionManager().
     * @param request A struct containing all parameters required to call this method.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_MALFORMED_USERNAME: The provided userName is malformed. Check the logs to see what the required format is.
     * - GAMEKIT_ERROR_FORGOT_PASSWORD_FAILED: The backend web request failed. Check the logs to see what the error was.
     */
    GAMEKIT_API unsigned int GameKitIdentityForgotPassword(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::ForgotPasswordRequest request);

    /**
     * @brief Set the player's new password.
     *
     * @param identityInstance A pointer to an Identity instance created with GameKitIdentityInstanceCreateWithSessionManager().
     * @param request A struct containing all parameters required to call this method.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_MALFORMED_USERNAME: The provided userName is malformed. Check the logs to see what the required format is.
     * - GAMEKIT_ERROR_MALFORMED_PASSWORD: The provided password is malformed. Check the logs to see what the required format is.
     * - GAMEKIT_ERROR_CONFIRM_FORGOT_PASSWORD_FAILED: The backend web request failed. Check the logs to see what the error was.
     */
    GAMEKIT_API unsigned int GameKitIdentityConfirmForgotPassword(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::ConfirmForgotPasswordRequest request);

    /**
     * @brief Get information about the currently logged in player.
     *
     * @details The response is a JSON string containing the following information (or an empty string if the call failed):
     * - The date time when the player was registered.
     * - The date time of the last time the player's identity information was modified.
     * - The player's GameKit ID.
     *
     * @param identityInstance A pointer to an Identity instance created with GameKitIdentityInstanceCreateWithSessionManager().
     * @param dispatchReceiver (Optional) This pointer will be passed to the callback function as the `dispatchReceiver`.
     * @param responseCallback The callback function to invoke if the method completes successfully. The callback returns the JSON string.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_NO_ID_TOKEN: The player is not logged in.
     * - GAMEKIT_ERROR_HTTP_REQUEST_FAILED: The backend HTTP request failed. Check the output logs to see what the HTTP response code was
     * - GAMEKIT_ERROR_PARSE_JSON_FAILED: The backend returned a malformed JSON payload. This should not happen. If it does, it indicates there is a bug in the backend code.
     */
    GAMEKIT_API unsigned int GameKitIdentityGetUser(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const GameKit::FuncIdentityGetUserResponseCallback responseCallback);

    /**
     * @brief Get a login/signup URL for the specified federated identity provider.
     *
     * @details Players will be able to register and/or sign in when the URL is opened in a web browser.
     *
     * @details You should call PollAndRetrieveFederatedTokens() afterward to sign the player into GameKit.
     *
     * @param identityInstance A pointer to an Identity instance created with GameKitIdentityInstanceCreateWithSessionManager().
     * @param identityProvider The federated identity provider to get the login URL for.
     * @param dispatchReceiver (Optional) This pointer will be passed to the callback function as the `dispatchReceiver`.
     * @param responseCallback The callback function to invoke if the method completes successfully. The callback is invoked twice: first to return the unique request ID which
     * should be passed to GameKitPollAndRetrieveFederatedTokens(), second to return the login URL.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_INVALID_FEDERATED_IDENTITY_PROVIDER: The specified federated identity provider is invalid or is not yet supported.
     */
    GAMEKIT_API unsigned int GameKitGetFederatedLoginUrl(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::FederatedIdentityProvider identityProvider, DISPATCH_RECEIVER_HANDLE dispatchReceiver, KeyValueCharPtrCallbackDispatcher responseCallback);

    /**
     * @brief Continually check if the player has completed signing in with the federated identity provider, then store their access tokens in the SessionManager.
     *
     * @details After calling this method, the player will be signed in and you'll be able to call the other GameKit APIs.
     * This method stores the player's authorized access tokens in the SessionManager, which automatically refreshes them before they expire.
     *
     * @details To call this method, you must first call GameKitGetFederatedLoginUrl() to get a unique request ID.
     *
     * @details This method will timeout after the specified limit, in which case the player is not logged in.
     * You can call GameKitGetFederatedAccessToken() to check if the login was successful.
     *
     * @param identityInstance A pointer to an Identity instance created with GameKitIdentityInstanceCreateWithSessionManager().
     * @param identityProvider The federated identity provider to get the login URL for.
     * @param requestId The unique request identifier returned in the callback function of GameKitGetFederatedLoginUrl().
     * @param timeout The number of seconds before the method will stop polling and will return with failure.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_INVALID_FEDERATED_IDENTITY_PROVIDER: The specified federated identity provider is invalid or is not yet supported.
     * - GAMEKIT_ERROR_REQUEST_TIMED_OUT: PollForCompletion timed out waiting for login completion.
     * - GAMEKIT_ERROR_HTTP_REQUEST_FAILED: Http request to get Federated Token failed.
     */
    GAMEKIT_API unsigned int GameKitPollAndRetrieveFederatedTokens(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::FederatedIdentityProvider identityProvider, const char* requestId, int timeout);

    /**
     * @brief Get the player's authorized Id token for the specified federated identity provider.
     *
     * @details The returned Id token will be empty if the player is not logged in with the federated identity provider.
     *
     * @param identityInstance A pointer to an Identity instance created with GameKitIdentityInstanceCreateWithSessionManager().
     * @param identityProvider The federated identity provider to get the login URL for.
     * @param dispatchReceiver (Optional) This pointer will be passed to the callback function as the `dispatchReceiver`.
     * @param responseCallback The callback function to invoke if the method completes successfully. The callback returns the player's authorized Id token,
     * or an empty string if the player is not logged in with the federated identity provider.
     * @return A GameKit status code indicating the result of the API call. Status codes are defined in errors.h. This method's possible status codes are listed below:
     * - GAMEKIT_SUCCESS: The API call was successful.
     * - GAMEKIT_ERROR_INVALID_FEDERATED_IDENTITY_PROVIDER: The specified federated identity provider is invalid or is not yet supported.
     */
    GAMEKIT_API unsigned int GameKitGetFederatedIdToken(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance, GameKit::FederatedIdentityProvider identityProvider, DISPATCH_RECEIVER_HANDLE dispatchReceiver, CharPtrCallback responseCallback);

    /**
     * @brief Destroy the passed in Identity instance.
     *
     * @details Make sure to destroy every Identity instance when finished with it in order to prevent a memory leak.
     *
     * @param identityInstance A pointer to an Identity instance created with GameKitIdentityInstanceCreateWithSessionManager().
     */
    GAMEKIT_API void GameKitIdentityInstanceRelease(GAMEKIT_IDENTITY_INSTANCE_HANDLE identityInstance);
}
