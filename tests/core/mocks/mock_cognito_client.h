// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <gmock/gmock.h>

#include <aws/cognito-idp/CognitoIdentityProviderClient.h>

namespace GameKit
{
    namespace Mocks
    {
        class MockCognitoIdentityProviderClient : public Aws::CognitoIdentityProvider::CognitoIdentityProviderClient
        {
        public:
            MockCognitoIdentityProviderClient() {}
            ~MockCognitoIdentityProviderClient() {}

            MOCK_METHOD(Aws::CognitoIdentityProvider::Model::RevokeTokenOutcome, RevokeToken, (const Aws::CognitoIdentityProvider::Model::RevokeTokenRequest&), (const, override));
            MOCK_METHOD(Aws::CognitoIdentityProvider::Model::SignUpOutcome, SignUp, (const Aws::CognitoIdentityProvider::Model::SignUpRequest&), (const, override));
            MOCK_METHOD(Aws::CognitoIdentityProvider::Model::ConfirmSignUpOutcome, ConfirmSignUp, (const Aws::CognitoIdentityProvider::Model::ConfirmSignUpRequest&), (const, override));
            MOCK_METHOD(Aws::CognitoIdentityProvider::Model::ResendConfirmationCodeOutcome, ResendConfirmationCode, (const Aws::CognitoIdentityProvider::Model::ResendConfirmationCodeRequest&), (const, override));
            MOCK_METHOD(Aws::CognitoIdentityProvider::Model::InitiateAuthOutcome, InitiateAuth, (const Aws::CognitoIdentityProvider::Model::InitiateAuthRequest&), (const, override));
            MOCK_METHOD(Aws::CognitoIdentityProvider::Model::ForgotPasswordOutcome, ForgotPassword, (const Aws::CognitoIdentityProvider::Model::ForgotPasswordRequest&), (const, override));
            MOCK_METHOD(Aws::CognitoIdentityProvider::Model::ConfirmForgotPasswordOutcome, ConfirmForgotPassword, (const Aws::CognitoIdentityProvider::Model::ConfirmForgotPasswordRequest&), (const, override));
            MOCK_METHOD(Aws::CognitoIdentityProvider::Model::GetUserOutcome, GetUser, (const Aws::CognitoIdentityProvider::Model::GetUserRequest&), (const, override));
        };
    }
}