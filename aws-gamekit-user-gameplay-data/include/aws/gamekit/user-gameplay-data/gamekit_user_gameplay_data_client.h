// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <algorithm>
#include <chrono>
#include <deque>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

// AWS SDK
#include <aws/cognito-idp/CognitoIdentityProviderClient.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>

// GameKit
#include <aws/gamekit/core/exports.h>
#include <aws/gamekit/core/gamekit_feature.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/awsclients/api_initializer.h>
#include <aws/gamekit/core/awsclients/default_clients.h>
#include <aws/gamekit/core/utils/gamekit_httpclient.h>
#include <aws/gamekit/core/utils/gamekit_httpclient.h>
#include <aws/gamekit/core/utils/ticker.h>

using namespace GameKit::Logger;
using namespace GameKit::Utils::HttpClient;

namespace GameKit
{
    namespace UserGameplayData
    {
        enum class UserGameplayDataOperationType
        {
            Write = 0, // Used for Add and Update API
            Delete, // Delete API
            Get // Get API
        };

        bool OperationTimestampCompare(const std::shared_ptr<IOperation> lhs, const std::shared_ptr<IOperation> rhs);

        struct GAMEKIT_API UserGameplayDataOperation : public IOperation
        {
            UserGameplayDataOperation(UserGameplayDataOperationType type, const std::string& bundle, const std::string& item,
                std::shared_ptr<Aws::Http::HttpRequest> request, Aws::Http::HttpResponseCode expected, unsigned int maxAttempts = OPERATION_ATTEMPTS_NO_LIMIT,
                std::chrono::milliseconds timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())) :
                IOperation(maxAttempts, false, request, expected, timestamp),
                Type(type), Bundle(bundle), ItemKey(item), OperationUniqueKey(bundle + "/" + item)
            {}

            const UserGameplayDataOperationType Type;
            const std::string Bundle;
            const std::string ItemKey;

            const std::string OperationUniqueKey;

            static bool TrySerializeBinary(std::ostream& os, const std::shared_ptr<IOperation> operation, FuncLogCallback logCb = nullptr);
            static bool TrySerializeBinary(std::ostream& os, const std::shared_ptr<UserGameplayDataOperation> operation, FuncLogCallback logCb = nullptr);
            static bool TryDeserializeBinary(std::istream& is, std::shared_ptr<IOperation>& outOperation, FuncLogCallback logCb = nullptr);
            static bool TryDeserializeBinary(std::istream& is, std::shared_ptr<UserGameplayDataOperation>& outOperation, FuncLogCallback logCb = nullptr);
        };
       
        // User Gameplay Data client with retry logic and support for unhealthy connectivity with an internal request queue.
        // Uses custom rules to deal with User Gameplay Data APIs:
        // 1. In Healthy mode, all calls are synchronous by default. Calls can be made async with flag and can provide success/failure callbacks.
        // 2. In Unhealthy mode, Add, Update and Delete API calls are kept in an internal queue. Get API calls are rejected.
        // 3. In Unhealthy mode, accumulated requests are preprocessed so that when multiple Add/Update and Delete operations for a single
        //    have been enqueued for a unique bundle-item combination, the most recent is kept and the old are discarded.
        // 4. Calls are retried in order from oldest to newest, user provided callbacks are invoked on success. 
        // 5. Default Unhealthy retry strategy is Exponential Backoff.
        class GAMEKIT_API UserGameplayDataHttpClient : public BaseHttpClient
        {
        private:

        protected:
            virtual void filterQueue(OperationQueue* queue, OperationQueue* filtered) override;
            virtual bool shouldEnqueueWithUnhealthyConnection(const std::shared_ptr<IOperation> operation) const override;
            virtual bool isOperationRetryable(const std::shared_ptr<IOperation> operation, std::shared_ptr<const Aws::Http::HttpResponse> response) const override;\

        public:
            UserGameplayDataHttpClient(std::shared_ptr<Aws::Http::HttpClient> client, RequestModifier authSetter,
                unsigned int retryIntervalSeconds, std::shared_ptr<IRetryStrategy> retryStrategy, size_t maxQueueSize, FuncLogCallback logCb) : 
                BaseHttpClient("UserGameplayData", client, authSetter, retryIntervalSeconds, retryStrategy, maxQueueSize, logCb)
            {}

            virtual ~UserGameplayDataHttpClient() override {}

            RequestResult MakeRequest(UserGameplayDataOperationType operationType, bool isAsync, const char* bundle, const char* itemKey, std::shared_ptr<Aws::Http::HttpRequest> request,
                Aws::Http::HttpResponseCode successCode, unsigned int maxAttempts, CallbackContext callbackContext = nullptr, ResponseCallback successCallback = nullptr, ResponseCallback failureCallback = nullptr);
        };
    }
}
