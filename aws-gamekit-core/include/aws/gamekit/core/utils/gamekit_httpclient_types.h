// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <algorithm>
#include <string>
#include <deque>
#include <map>
#include <chrono>
#include <sstream>
#include <iostream>

// GameKit
#include <aws/gamekit/core/errors.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/utils/ticker.h>

// AWS SDK Forward Declarations
namespace Aws {
  namespace Http {
    class HttpRequest;
    class HttpResponse;
    enum class HttpResponseCode;
  }
}

using namespace GameKit::Logger;

#define OPERATION_ATTEMPTS_NO_LIMIT 0

namespace GameKit
{
    namespace Utils
    {
        namespace Serialization
        {
            template <typename T, size_t N>
            GAMEKIT_API std::ostream& BinWrite(std::ostream& os, const T (&t)[N])
            {
                size_t length = N;
                os.write((char*)&length, sizeof(size_t));
                os.write((char*)t, N);

                return os;
            }

            template <typename T>
            GAMEKIT_API std::ostream& BinWrite(std::ostream& os, const T& t)
            {
                os.write((char*)&t, sizeof(T));

                return os;
            }

#if __ANDROID__
            // Specialization for Aws::String. In Android std::string and Aws::String are different types
            template <>
            GAMEKIT_API std::ostream& BinWrite<Aws::String>(std::ostream& os, const Aws::String& s);
#endif

            template <>
            GAMEKIT_API std::ostream& BinWrite<std::string>(std::ostream& os, const std::string& s);

            template <typename T>
            GAMEKIT_API std::istream& BinRead(std::istream& is, T& t)
            {
                is.read((char*)&t, sizeof(T));

                return is;
            }

#if __ANDROID__
            // Specialization for Aws::String. In Android std::string and Aws::String are different types
            template <>
            GAMEKIT_API std::istream& BinRead<Aws::String>(std::istream& is, Aws::String& s);
#endif
            template <>
            GAMEKIT_API std::istream& BinRead<std::string>(std::istream& is, std::string& s);

            GAMEKIT_API unsigned int GetCRC(const std::string& s);

            GAMEKIT_API unsigned int GetCRC(const char* s, size_t length);
        }

        namespace HttpClient
        {
            // Wildcard object for response callback context
            typedef void* CallbackContext;

            // Callback to be called when a request has received a response.
            typedef std::function<void(CallbackContext requestContext, std::shared_ptr<Aws::Http::HttpResponse>)> ResponseCallback;

            // Callback to be called before sending a request. Used to update/modify request headers such as authorization.
            typedef std::function<void(std::shared_ptr<Aws::Http::HttpRequest>)> RequestModifier;

            GAMEKIT_API bool TrySerializeRequestBinary(std::ostream& os, const std::shared_ptr<Aws::Http::HttpRequest> request, FuncLogCallback logCb = nullptr);
            GAMEKIT_API bool TryDeserializeRequestBinary(std::istream& is, std::shared_ptr<Aws::Http::HttpRequest>& outRequest, FuncLogCallback logCb = nullptr);

            // Base struct for retryable client operations
            struct GAMEKIT_API IOperation
            {
                const std::chrono::milliseconds Timestamp;
                unsigned int Attempts;
                const unsigned int MaxAttempts;
                bool Discard;
                bool FromCache = false;

                std::shared_ptr<Aws::Http::HttpRequest> Request;
                const Aws::Http::HttpResponseCode ExpectedSuccessCode;

                CallbackContext CallbackContext;
                ResponseCallback SuccessCallback;
                ResponseCallback FailureCallback;

                IOperation(unsigned int maxAttempts,
                    bool discard,
                    std::shared_ptr<Aws::Http::HttpRequest> request,
                    const Aws::Http::HttpResponseCode expectedCode,
                    std::chrono::milliseconds timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()));
            };

            typedef std::deque<std::shared_ptr<IOperation>> OperationQueue;

            // Types of request results
            enum class RequestResultType
            {
                RequestMadeSuccess = 0,
                RequestMadeFailure,
                RequestDropped,
                RequestEnqueued,
                RequestAttemptedAndEnqueued
            };

            std::string GAMEKIT_API RequestResultTypeToString(RequestResultType resultType);

            // Result of a client request
            struct GAMEKIT_API RequestResult
            {
                RequestResult(RequestResultType type, std::shared_ptr<Aws::Http::HttpResponse> response)
                    : ResultType(type), Response(response)
                {}

                RequestResultType ResultType;
                std::shared_ptr<Aws::Http::HttpResponse> Response;

                std::string ToString() const;

                unsigned int ToErrorCode() const;
            };

            // Base interface for retry strategies
            class GAMEKIT_API IRetryStrategy
            {
            public:
                IRetryStrategy() {}
                virtual ~IRetryStrategy() {}

                virtual void IncreaseThreshold() = 0;
                virtual bool ShouldRetry() = 0;
                virtual void Reset() = 0;
            };

            // Constant Interval Strategy. With this strategy, operations are always retried in each interval.
            class GAMEKIT_API ConstantIntervalStrategy : public IRetryStrategy
            {
            public:
                ConstantIntervalStrategy() {}
                virtual ~ConstantIntervalStrategy() {}

                virtual void IncreaseThreshold() override { /* No-op by design */ };
                virtual bool ShouldRetry() override { /* Always true by design */  return true; };
                virtual void Reset() override { /* No-op by design */ };
            };

            // Binary Exponential Backoff Strategy. With this strategy, retries are spaced out at exponential intervals.
            class GAMEKIT_API ExponentialBackoffStrategy : public IRetryStrategy
            {
            private:
                unsigned int tickCounter;
                unsigned int maxAttempts;
                unsigned int currentStep;
                unsigned int retryThreshold;
                FuncLogCallback logCb = nullptr;

            public:
                ExponentialBackoffStrategy(unsigned int maxAttempts, FuncLogCallback logCb = nullptr);
                virtual ~ExponentialBackoffStrategy();

                virtual void IncreaseThreshold() override;
                virtual bool ShouldRetry() override;
                virtual void Reset() override;
            };

            enum class StrategyType
            {
                ExponentialBackoff = 0,
                ConstantInterval
            };
        }
    }
}
