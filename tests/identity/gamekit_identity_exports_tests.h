// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../core/test_common.h"
#include <aws/gamekit/identity/exports.h>
#include "aws/gamekit/identity/gamekit_identity.h"
#include "aws/gamekit/identity/facebook_identity_provider.h"
#include "../core/mocks/fake_http_client.h"
#include "../core/test_stack.h"
#include "../core/test_log.h"

namespace GameKit
{
    namespace Tests
    {
        namespace IdentityExports
        {

            class Dispatcher
            {
            public:
                Dispatcher() = default;
                DISPATCH_RECEIVER_HANDLE get()
                {
                    return this;
                }
                std::string email;
                std::string userName;
                std::string userId;
                void CallbackHandler(const GameKit::GetUserResponse* res);
            };

            class GameKitIdentityExportsTestFixture : public ::testing::Test
            {
            public:
                GameKitIdentityExportsTestFixture();
                ~GameKitIdentityExportsTestFixture();
                virtual void SetUp() override;
                virtual void TearDown() override;
                std::string GetCognitoGetUserApiResponse();
                std::string GetIdentityLambdaGetUserApiResponse();

            protected:
                typedef TestLog<GameKitIdentityExportsTestFixture> TestLogger;

                TestStackInitializer testStack;

                void* createIdentityInstance();
                void* createIdentityInstanceWithNoSessionManagerTokens();
                void setIdentityMocks(void* instance);
                Aws::UniquePtr<GameKit::Mocks::MockCognitoIdentityProviderClient> cognitoMock;
                std::shared_ptr<MockHttpClient> mockHttpClient;

                template <typename Result, typename Outcome>
                Outcome SuccessOutcome()
                {
                    Result result;
                    Outcome outcome(result);
                    return outcome;
                }
            };
        }
    }
}