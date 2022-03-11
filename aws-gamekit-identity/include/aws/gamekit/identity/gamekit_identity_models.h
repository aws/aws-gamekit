// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace GameKit
{
    /**
     * @brief The request object for GameKitIdentityRegister().
     */
    struct UserRegistration
    {
        /**
         * @brief The username the player wants to have. The player must type this in whenever they log in.
         *
         * @details This has certain character restrictions, which will be shown in the Output Log if an invalid username is provided.
         */
        const char* userName;

        /**
         * @brief The password the player wants to use.
         *
         * @details This has certain character restrictions, which will be shown in the Output Log if an invalid username is provided.
         */
        const char* password;

        /**
         * @brief The player's email address.
         */
        const char* email;

        /**
         * @brief Do not use. This field will be used in the future to allow guest registration.
         */
        const char* userId;

        /**
         * @brief Do not use. This field will be used in the future to allow guest registration.
         */
        const char* userIdHash;
    };

    /**
     * @brief The request object for GameKitIdentityConfirmRegistration().
     */
    struct ConfirmRegistrationRequest
    {
        /**
         * @brief The username of the player to confirm.
         */
        const char* userName;

        /**
         * @brief The registration confirmation code that was emailed to the player.
         */
        const char* confirmationCode;
    };

    /**
     * @brief The request object for GameKitIdentityResendConfirmationCode().
     */
    struct ResendConfirmationCodeRequest
    {
        /**
         * @brief The username of the player to email the new confirmation code.
         */
        const char* userName;
    };

    /**
     * @brief The request object for GameKitIdentityLogin().
     */
    struct UserLogin
    {
        /**
         * @brief The username of the player that is logging in.
         */
        const char* userName;

        /**
         * @brief The player's password.
         */
        const char* password;
    };

    /**
     * @brief The request object for GameKitIdentityForgotPassword().
     */
    struct ForgotPasswordRequest
    {
        /**
         * @brief The username of the player to email the reset password code to.
         */
        const char* userName;
    };

    /**
     * @brief The request object for GameKitIdentityConfirmForgotPassword().
     */
    struct ConfirmForgotPasswordRequest
    {
        /**
         * @brief The username of the player to set a new password for.
         */
        const char* userName;

        /**
         * @brief The new password the player wants to use.
         */
        const char* newPassword;

        /**
         * @brief The password reset code that was emailed to the player.
         */
        const char* confirmationCode;
    };

    /**
     * @brief The response object for GameKitIdentityGetUser().
     */
    struct GetUserResponse
    {
        /**
         * @brief Unique Id generated for a user on registration.
         *
         * @details This is a uid created by cognito to uniquely identiy the user.
         */
        const char* userId;

        /**
         * @brief Timestamp of when user was last updated.
         */
        const char* updatedAt;

        /**
         * @brief Timestamp of when user was created.
         */
        const char* createdAt;

        /**
         * @brief Players Facebook external id.
         */
        const char* facebookExternalId;

        /**
         * @brief Players Facebook refrence id.
         */
        const char* facebookRefId;

        /**
         * @brief Players user name.
         */
        const char* userName;

        /**
         * @brief Players email address.
         */
        const char* email;
    };

    /**
     * @brief A static dispatcher function pointer that receives key/value pairs, one invocation at a time.
     *
     * @param dispatchReceiver A pointer to an instance of a class where the results will be dispatched to.
     * This instance must have have a method signature of void ReceiveResult(const char* charKey, const char* charValue);
     * @param GetUserResponse GetUser response struct.
    */
    typedef void(*FuncIdentityGetUserResponseCallback)(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const GameKit::GetUserResponse* getUserResponse);
}