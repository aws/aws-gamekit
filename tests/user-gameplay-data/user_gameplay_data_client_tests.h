// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../core/test_common.h"
#include "../core/test_log.h"
#include "../core/test_stack.h"
#include <aws/gamekit/user-gameplay-data/gamekit_user_gameplay_data_client.h>
#include <aws/gamekit/user-gameplay-data/gamekit_user_gameplay_data.h>

#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>

namespace GameKit
{
    namespace Tests
    {
        class UserGameplayDataClientTestFixture : public ::testing::Test
        {
        protected:
            std::function<void(std::shared_ptr<Aws::Http::HttpRequest>)> authSetter;
            std::shared_ptr<GameKit::Utils::HttpClient::IRetryStrategy> retryLogic;
            typedef TestLog<UserGameplayDataClientTestFixture> TestLogger;
            TestStackInitializer testStack;

        public:
            UserGameplayDataClientTestFixture();
            ~UserGameplayDataClientTestFixture();

            virtual void SetUp() override;
            virtual void TearDown() override;

            void AuthSetter(std::shared_ptr<Aws::Http::HttpRequest> request);

            void SuccessCallback(GameKit::Utils::HttpClient::CallbackContext, std::shared_ptr<Aws::Http::HttpResponse>);

            static void NetworkStateChangeCb(NETWORK_STATE_RECEIVER_HANDLE dispatchReceiver, bool isConnectionOk, const char* connectionClient);
            static void CacheProcessedCb(NETWORK_STATE_RECEIVER_HANDLE dispatchReceiver, bool cacheProcessed);
        };
    }
}