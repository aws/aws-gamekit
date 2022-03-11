// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/user-gameplay-data/gamekit_user_gameplay_data_client.h>

using namespace GameKit::Utils::HttpClient;
using namespace GameKit::Utils::Serialization;
using namespace GameKit::UserGameplayData;

#pragma region UserGameplayData Public Methods
bool GameKit::UserGameplayData::OperationTimestampCompare(const std::shared_ptr<IOperation> lhs, const std::shared_ptr<IOperation> rhs)
{
    return lhs->Timestamp < rhs->Timestamp;
}
#pragma endregion

#pragma region UserGameplayDataOperation Public Methods
bool UserGameplayDataOperation::TrySerializeBinary(std::ostream& os, const std::shared_ptr<IOperation> operation, FuncLogCallback logCb)
{
    auto gameplayOperation = std::static_pointer_cast<UserGameplayDataOperation>(operation);

    return UserGameplayDataOperation::TrySerializeBinary(os, gameplayOperation, logCb);
}

bool UserGameplayDataOperation::TrySerializeBinary(std::ostream& os, const std::shared_ptr<UserGameplayDataOperation> operation, FuncLogCallback logCb)
{
    try
    {
        os.exceptions(std::ostream::failbit); // throw on failure

        BinWrite(os, operation->Type);
        BinWrite(os, operation->Bundle);
        BinWrite(os, operation->ItemKey);
        BinWrite(os, operation->MaxAttempts);
        BinWrite(os, operation->ExpectedSuccessCode);
        BinWrite(os, operation->Timestamp.count());

        return TrySerializeRequestBinary(os, operation->Request, logCb);
    }
    catch (const std::ios_base::failure& failure)
    {
        std::string message = "Could not serialize UserGameplayDataOperation, " + std::string(failure.what());
        Logging::Log(logCb, Level::Error, message.c_str());
    }

    return false;
}

bool UserGameplayDataOperation::TryDeserializeBinary(std::istream& is, std::shared_ptr<IOperation>& outOperation, FuncLogCallback logCb)
{
    auto outGameplayOperation = std::static_pointer_cast<UserGameplayDataOperation>(outOperation);

    if (GameKit::UserGameplayData::UserGameplayDataOperation::TryDeserializeBinary(is, outGameplayOperation, logCb))
    {
        outOperation = std::static_pointer_cast<IOperation>(outGameplayOperation);
        return true;
    }

    return false;
}

bool UserGameplayDataOperation::TryDeserializeBinary(std::istream& is, std::shared_ptr<UserGameplayDataOperation>& outOperation, FuncLogCallback logCb)
{
    UserGameplayDataOperationType type;
    std::string bundle;
    std::string item;

    int maxAttempts;
    Aws::Http::HttpResponseCode expectedCode;
    long long milliseconds;

    try
    {
        is.exceptions(std::istream::failbit); // throw on failure

        BinRead(is, type);
        BinRead(is, bundle);
        BinRead(is, item);
        BinRead(is, maxAttempts);
        BinRead(is, expectedCode);
        BinRead(is, milliseconds);

        std::shared_ptr<Aws::Http::HttpRequest> request;
        if (TryDeserializeRequestBinary(is, request))
        {
            outOperation = std::make_shared<UserGameplayDataOperation>(type, bundle, item, request, expectedCode, maxAttempts, std::chrono::milliseconds(milliseconds));

            return true;
        }
    }
    catch (const std::ios_base::failure& failure)
    {
        std::string message = "Could not deserialize UserGameplayDataOperation, " + std::string(failure.what());
        Logging::Log(logCb, Level::Error, message.c_str());
    }

    return false;
}
#pragma endregion

#pragma region UserGameplayDataHttpClient Public Methods
RequestResult UserGameplayDataHttpClient::MakeRequest(UserGameplayDataOperationType operationType,
    bool isAsync,
    const char* bundle,
    const char* itemKey,
    std::shared_ptr<Aws::Http::HttpRequest> request,
    Aws::Http::HttpResponseCode successCode,
    unsigned int maxAttempts,
    CallbackContext callbackContext,
    ResponseCallback successCallback,
    ResponseCallback failureCallback)
{
    std::shared_ptr<IOperation> operation = std::make_shared<UserGameplayDataOperation>(
        operationType, bundle, itemKey, request, successCode, maxAttempts);

    operation->CallbackContext = callbackContext;
    operation->SuccessCallback = successCallback;
    operation->FailureCallback = failureCallback;

    auto result = this->makeOperationRequest(operation, isAsync, false);

    std::string message = "UserGameplayDataHttpClient::MakeRequest with operation " + std::to_string((int)operationType) +
        ", async " + std::to_string(isAsync) + ", bundle " + bundle + ", item " + itemKey + result.ToString();
    Logging::Log(m_logCb, Level::Verbose, message.c_str());

    return result;
}
#pragma endregion

#pragma region UserGameplayDataHttpClient Private/Protected Methods
void UserGameplayDataHttpClient::filterQueue(OperationQueue* queue, OperationQueue* filtered)
{
    Logging::Log(m_logCb, Level::Verbose, "UserGameplayDataHttpClient::FilterQueue");
    std::map<std::string, std::deque<UserGameplayDataOperation*>> temp;
    unsigned int operationsDiscarded = 0;

    // Sort queue based on timestamp. Order is important for User Gameplay Data filtering
    std::sort(queue->begin(), queue->end(), OperationTimestampCompare);

    for (auto operationIt = queue->begin(); operationIt != queue->end(); ++operationIt)
    {
        UserGameplayDataOperation* operation = static_cast<UserGameplayDataOperation*>((*operationIt).get());

        auto& queueWithSameKey = temp[operation->OperationUniqueKey];

        if (queueWithSameKey.empty())
        {
            queueWithSameKey.push_back(operation);
        }
        else
        {
            auto& previousOperation = queueWithSameKey.back();

            // If keys don't match, keep both.
            // Some rare coincidence caused two operations to be mapped to the same queue
            if (operation->Bundle != previousOperation->Bundle ||
                operation->ItemKey != previousOperation->ItemKey)
            {
                Logging::Log(m_logCb, Level::Warning, "UserGameplayDataOperation key mismatch, keeping both operations.");
                queueWithSameKey.push_back(operation);
            }

            // If this is an item-level operation, most recent one is kept
            // If this is a bundle-level operation, or global,
            // and if most recent is delete, keep delete, else keep both

            if (!operation->ItemKey.empty() && !previousOperation->ItemKey.empty())
            {
                Logging::Log(m_logCb, Level::Verbose, "Discarding previous item operation, newer operation overwrites data.");
                previousOperation->Discard = true;
                queueWithSameKey.pop_back();
            }
            else if (operation->Type == UserGameplayDataOperationType::Delete)
            {
                Logging::Log(m_logCb, Level::Verbose, "Discarding previous bundle operation, newer operation overwrites data.");
                previousOperation->Discard = true;
                queueWithSameKey.pop_back();
            }

            queueWithSameKey.push_back(operation);
        }
    }

    // Enqueue non discarded operations
    for (auto& operation : *queue)
    {
        if (!operation->Discard)
        {
            filtered->push_back(operation);
        }
    }

    std::string message = "UserGameplayDataHttpClient::FilterQueue. Discarded " + std::to_string(operationsDiscarded) + " operations.";
    Logging::Log(m_logCb, Level::Info, message.c_str());
}

bool UserGameplayDataHttpClient::shouldEnqueueWithUnhealthyConnection(const std::shared_ptr<IOperation> operation) const
{
    auto ugpdOperation = static_cast<const UserGameplayDataOperation*>(operation.get());

    return ugpdOperation->Type != UserGameplayDataOperationType::Get;
}

bool UserGameplayDataHttpClient::isOperationRetryable(const std::shared_ptr<IOperation> operation,
    std::shared_ptr<const Aws::Http::HttpResponse> response) const
{
    auto ugpdOperation = static_cast<const UserGameplayDataOperation*>(operation.get());

    bool attemptsExhausted = ugpdOperation->MaxAttempts != OPERATION_ATTEMPTS_NO_LIMIT && ugpdOperation->Attempts > ugpdOperation->MaxAttempts;
    bool isResponseRetryable = BaseHttpClient::isResponseCodeRetryable(response->GetResponseCode());

    std::string message = "UserGameplayDataHttpClient::IsOperationRetryable: Attempts exhausted " + std::to_string(attemptsExhausted) +
        ", Type " + std::to_string(int(ugpdOperation->Type)) + ", IsResponseCodeRetryable " + std::to_string(isResponseRetryable);
    Logging::Log(m_logCb, Level::Verbose, message.c_str());

    return !attemptsExhausted &&
        ugpdOperation->Type != UserGameplayDataOperationType::Get &&
        isResponseRetryable;
}
#pragma endregion
