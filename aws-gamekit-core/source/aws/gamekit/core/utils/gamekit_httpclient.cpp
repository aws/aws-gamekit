// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#ifdef __ANDROID__
#include <resolv.h>
#endif

#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>
#include <aws/gamekit/core/utils/file_utils.h>
#include <aws/gamekit/core/utils/gamekit_httpclient.h>

using namespace GameKit::Utils::HttpClient;
using namespace GameKit::Utils;

#pragma region Constructor/Deconstructor
BaseHttpClient::BaseHttpClient(
    const std::string& clientName,
    std::shared_ptr<Aws::Http::HttpClient> client,
    RequestModifier authSetter,
    unsigned int retryIntervalSeconds,
    std::shared_ptr<IRetryStrategy> retryStrategy,
    size_t maxPendingQueueSize,
    FuncLogCallback logCb) :
    m_clientName(clientName),
    m_httpClient(client),
    m_authorizationHeaderSetter(authSetter),
    m_attempsCount(0),
    m_isConnectionOk(true),
    m_stopProcessingOnError(true),
    m_errorDuringProcessing(false),
    m_maxPendingQueueSize(maxPendingQueueSize),
    m_secondsInterval(retryIntervalSeconds),
    m_retryStrategy(retryStrategy),
    m_logCb(logCb),
    m_requestPump(m_secondsInterval, std::bind(&BaseHttpClient::preProcessQueue, this), logCb),
    m_abortProcessingRequested(false),
    m_activeQueue(),
    m_pendingQueue(),
    m_stateReceiverHandle(nullptr),
    m_statusCb(nullptr),
    m_cachedProcessedReceiverHandle(nullptr),
    m_cachedProcessedCb(nullptr)
{}

BaseHttpClient::~BaseHttpClient()
{
    StopRetryBackgroundThread();
    std::lock_guard<std::mutex> lock(m_queueProcessingMutex);
    m_httpClient->DisableRequestProcessing();

    if (!m_activeQueue.empty())
    {
        Logging::Log(m_logCb, Level::Warning, "~BaseHttpClient: Active queue not empty.");
    }

    if (!m_pendingQueue.empty())
    {
        Logging::Log(m_logCb, Level::Warning, "~BaseHttpClient: Pending queue not empty.");
    }
}
#pragma endregion

#pragma region Public Methods
void BaseHttpClient::SetNetworkChangeCallback(NETWORK_STATE_RECEIVER_HANDLE receiverHandle, NetworkStatusChangeCallback statusChangeCallback)
{
    m_stateReceiverHandle = receiverHandle;
    m_statusCb = statusChangeCallback;
}

void BaseHttpClient::SetCacheProcessedCallback(CACHE_PROCESSED_RECEIVER_HANDLE receiverHandle, CacheProcessedCallback cacheProcessedCallback)
{
    m_cachedProcessedReceiverHandle = receiverHandle;
    m_cachedProcessedCb = cacheProcessedCallback;
}

void BaseHttpClient::StartRetryBackgroundThread()
{
    if (!m_requestPump.IsRunning())
    {
        std::string message = "Starting request pump thread with " + std::to_string(m_secondsInterval) + " seconds interval";
        Logging::Log(m_logCb, Level::Info, message.c_str());
        m_retryStrategy->Reset();
        m_requestPump.Start();
    }
}

void BaseHttpClient::StopRetryBackgroundThread()
{
    if (m_requestPump.IsRunning())
    {
        std::string message = "Stopping request pump thread";
        Logging::Log(m_logCb, Level::Info, message.c_str());
        m_abortProcessingRequested = true;
        m_requestPump.Stop();
        m_abortProcessingRequested = false;
    }
}

bool BaseHttpClient::PersistQueue(const std::string& file, std::function<bool(std::ostream&, const std::shared_ptr<IOperation>, FuncLogCallback)> serializer, bool clearQueue)
{
    std::string message = "Persisting queues to: " + file;
    Logging::Log(m_logCb, Level::Info, message.c_str());

    if (m_requestPump.IsRunning())
    {
        message = "Queue cannot be persisted while request pump is running, stop the request pump first.";
        Logging::Log(m_logCb, Level::Error, message.c_str());
        return false;
    }

    auto nativePath = GameKit::Utils::FileUtils::PathFromUtf8(file);

    std::lock_guard<std::mutex> requestLock(m_requestMutex);

    size_t operationCount = m_activeQueue.size() + m_pendingQueue.size();
    if (operationCount == 0)
    {
        Logging::Log(m_logCb, Level::Info, "Nothing to persist, queues are empty.");
        return true;
    }

    std::ofstream outputFile(nativePath, std::ios::binary);
    if (outputFile.fail())
    {
        message = "Failed to open file " + file + " for write.";
        Logging::Log(m_logCb, Level::Error, message.c_str());
        return false;
    }

    std::lock_guard<std::mutex> queueLock(m_queueProcessingMutex);
    try
    {
        outputFile.exceptions(std::ostream::failbit | std::ostream::badbit); // throw on failure

        GameKit::Utils::Serialization::BinWrite(outputFile, operationCount);

        for (auto& operation : m_activeQueue)
        {
            if (!serializer(outputFile, operation, m_logCb))
            {
                Logging::Log(m_logCb, Level::Error, "Could not persist active queue.");
                outputFile.close();
                return false;
            }
        }

        for (auto& operation : m_pendingQueue)
        {
            if (!serializer(outputFile, operation, m_logCb))
            {
                Logging::Log(m_logCb, Level::Error, "Could not persist pending queue.");
                outputFile.close();
                return false;
            }
        }

        outputFile.close();
    }
    catch (const std::exception& e)
    {
        message = "Could not persist data to " + file;
        Logging::Log(m_logCb, Level::Error, message.c_str());
        return false;
    }

    if (clearQueue)
    {
        m_activeQueue.clear();
        m_pendingQueue.clear();
    }

    message = "Wrote " + std::to_string(operationCount) + " operations to: " + file;
    Logging::Log(m_logCb, Level::Info, message.c_str());

    return true;
}

bool BaseHttpClient::LoadQueue(const std::string& file, std::function<bool(std::istream&, std::shared_ptr<IOperation>&, FuncLogCallback)> deserializer, bool deleteFileAfterLoading)
{
    std::string message = "Loading queue from: " + file;
    Logging::Log(m_logCb, Level::Info, message.c_str());

    if (m_requestPump.IsRunning())
    {
        message = "Queue cannot be loaded while request pump is running, stop the request pump first.";
        Logging::Log(m_logCb, Level::Error, message.c_str());
        return false;
    }

    FileUtils::PlatformPathString nativePath = FileUtils::PathFromUtf8(file);

    std::lock_guard<std::mutex> requestLock(m_requestMutex);

    size_t operationCount = 0;
    std::ifstream inputFile(nativePath, std::ios::binary);

    if (inputFile.fail())
    {
        message = "Failed to open file " + file + " for read.";
        Logging::Log(m_logCb, Level::Error, message.c_str());
        return false;
    }

    if (inputFile.peek() == std::ifstream::traits_type::eof())
    {
        message = "File " + file + " is empty.";
        Logging::Log(m_logCb, Level::Error, message.c_str());
        return false;
    }

    try
    {
        inputFile.exceptions(std::istream::failbit | std::istream::badbit); // throw on failure

        GameKit::Utils::Serialization::BinRead(inputFile, operationCount);

        std::lock_guard<std::mutex> queueLock(m_queueProcessingMutex);
        for (size_t i = 0; i < operationCount; ++i)
        {
            std::shared_ptr<IOperation> operation;
            if (!deserializer(inputFile, operation, m_logCb))
            {
                Logging::Log(m_logCb, Level::Error, "Could not deserialize queue.");
                inputFile.close();
                return false;
            }

            operation->FromCache = true;
            m_pendingQueue.push_back(operation);
        }

        inputFile.close();

        if (deleteFileAfterLoading)
        {
            message = "Deleting file: " + file;
            Logging::Log(m_logCb, Level::Info, message.c_str());

#if __ANDROID__
            // Workaround for Android's "Not implemented" error when calling boost::filesystem::remove()
            int result = remove(nativePath.c_str());
            if (result != 0)
            {
                message = "Could not delete, result: " + std::to_string(result) + ", errno: " + std::to_string(errno);
                Logging::Log(m_logCb, Level::Error, message.c_str());
            }
#else
            boost::system::error_code error;
            if (!boost::filesystem::remove(nativePath, error))
            {
                message = "Could not delete, error: " + error.message();
                Logging::Log(m_logCb, Level::Error, message.c_str());
            }
#endif
        }
    }
    catch (const std::exception& e)
    {
        message = "Could not load data from " + file + ", " + e.what();
        Logging::Log(m_logCb, Level::Error, message.c_str());
        return false;
    }

    message = "Read " + std::to_string(operationCount) + " operations from: " + file;
    Logging::Log(m_logCb, Level::Info, message.c_str());

    if (operationCount != 0)
    {
        m_cachedOperationsRemaining = operationCount;
    }

    return true;
}

void BaseHttpClient::removeCachedFromQueue(OperationQueue* queue, OperationQueue* filtered) const
{
    Logging::Log(m_logCb, Level::Verbose, "UserGameplayDataHttpClient::RemoveCachedFromQueue");
    unsigned int operationsDiscarded = 0;

    for (auto operationIt = queue->begin(); operationIt != queue->end(); ++operationIt)
    {
        IOperation* operation = (*operationIt).get();

        if (operation->FromCache)
        {
            operation->Discard = true;
            operationsDiscarded++;
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

    std::string message = "UserGameplayDataHttpClient::RemoveCachedFromQueue. Discarded " + std::to_string(operationsDiscarded) + " operations.";
    Logging::Log(m_logCb, Level::Info, message.c_str());
}

void BaseHttpClient::SetLowLevelHttpClient(std::shared_ptr<Aws::Http::HttpClient> client)
{
    m_httpClient = client;
}
#pragma endregion

#pragma region Private/Protected Methods
bool BaseHttpClient::enqueuePending(std::shared_ptr<IOperation> operation)
{
    std::lock_guard<std::mutex> lock(m_queueProcessingMutex);
    if (!m_requestPump.IsRunning())
    {
        Logging::Log(m_logCb, Level::Warning, "Retry background thread is not running, request will not be enqueued.");
        return false;
    }

    if (isPendingQueueBelowLimit())
    {
        m_pendingQueue.push_back(operation);
        std::string message = "Pending queue size: " + std::to_string(m_pendingQueue.size());
        Logging::Log(m_logCb, Level::Verbose, message.c_str());
        return true;
    } // else, the request is dropped and an error has been logged

    return false;
}

void BaseHttpClient::preProcessQueue()
{
    // Add active and pending operations to a single queue.
    // Filter queue and process the remaining operations.

    {
        std::lock_guard<std::mutex> lock(m_queueProcessingMutex);

        size_t activeCount = m_activeQueue.size();
        size_t pendingCount = m_pendingQueue.size();

        if ((activeCount + pendingCount) == 0)
        {
            Logging::Log(m_logCb, Level::Verbose, "Queues are empty, nothing to process.");

            if (!m_isConnectionOk)
            {
                Logging::Log(m_logCb, Level::Info, "Reset connection state to \"Healthy\".");
                m_isConnectionOk = true;
                notifyNetworkStateChange();
            }

            m_errorDuringProcessing = false;

            return;
        }

        if (!m_retryStrategy->ShouldRetry())
        {
            Logging::Log(m_logCb, Level::Info, "Skipped processing operations due to retry strategy.");
            return;
        }

        std::string message = "Processing " + std::to_string(activeCount) + " operations in active queue, " + std::to_string(pendingCount) + " operations in pending queue";
        Logging::Log(m_logCb, Level::Info, message.c_str());

        // Append operations from active to pending queue to preserve order
        std::move(m_activeQueue.begin(), m_activeQueue.end(), std::back_inserter(m_pendingQueue));
        m_activeQueue.clear();

        // Filter pending queue, using active queue as target.
        filterQueue(&m_pendingQueue, &m_activeQueue);
        m_pendingQueue.clear();
    }

    // At this point we've determined that there are events to process in the active queue, 
    // otherwise we would've returned earlier.
    processActiveQueue();
}

void BaseHttpClient::processActiveQueue()
{
    // Send requests for each operation in the active queue. Stop sending events when failure occurs.

    std::string message = "Processing active queue with " + std::to_string(m_activeQueue.size()) + " items";
    Logging::Log(m_logCb, Level::Info, message.c_str());
    bool overrideConnectionStatus = true;

    do
    {
        auto operation = m_activeQueue.front();
        m_activeQueue.pop_front();

        auto result = makeOperationRequest(operation, false, overrideConnectionStatus);

        if (operation->FromCache)
        {
            if (result.ResultType == RequestResultType::RequestMadeSuccess)
            {
                m_cachedOperationsRemaining--;
            }
            else if (!m_skipCacheProcessedCallback)
            {
                notifyCachedOperationsProcessed(false);
                m_skipCacheProcessedCallback = true;
            }

            if (m_cachedOperationsRemaining == 0)
            {
                notifyCachedOperationsProcessed(true);
            }
        }

        if (result.ResultType == RequestResultType::RequestMadeSuccess)
        {
            // Override connection state to keep processing items and flush the queue
            Logging::Log(m_logCb, Level::Info, "Request succeeded, continue processing.");
            overrideConnectionStatus = true;
        }
        else
        {
            // Hit a failure, stop making requests. Operations will be retried in the next tick.
            Logging::Log(m_logCb, Level::Warning, "Will stop making requests");
            overrideConnectionStatus = false;

#if defined(ANDROID) || defined(__ANDROID__)
            // In Android, getaddrinfo() will keep failing even after the connection is restored 
            // so we need to call res_init() to resolve hosts again.
            std::lock_guard<std::mutex> requestLock(m_requestMutex);
            Logging::Log(m_logCb, Level::Warning, "Calling res_init()");
            res_init();
#endif
            // Rewind request content body buffer, otherwise requests will be invalid
            if (operation->Request->HasContentType() || operation->Request->HasContentLength())
            {
                operation->Request->GetContentBody()->clear();
                operation->Request->GetContentBody()->seekg(0);
            }
        }

    } while (overrideConnectionStatus && !m_activeQueue.empty() && !m_abortProcessingRequested);

    if (overrideConnectionStatus && m_activeQueue.empty() && !m_abortProcessingRequested)
    {
        // all items in the active queue were sent, let's flush the pending queue
        // in case new items arrived while processing
        Logging::Log(m_logCb, Level::Info, "All items sent, flushing remaining items");

        preProcessQueue();
    }
    else
    {
        // not all items were sent, return and wait for next invocation
        Logging::Log(m_logCb, Level::Warning, "Not all items in the queue were sent, items will be retried.");
    }
}

void BaseHttpClient::DropAllCachedEvents()
{
    if (m_requestPump.IsRunning())
    {
        std::string message = "Cached Events cannot be dropped while request pump is running, stop the request pump first.";
        Logging::Log(m_logCb, Level::Error, message.c_str());
        return;
    }

    std::lock_guard<std::mutex> lock(m_queueProcessingMutex);

    // append operations from active to pending queue
    std::move(m_activeQueue.begin(), m_activeQueue.end(), std::back_inserter(m_pendingQueue));
    m_activeQueue.clear();

    // Filter pending queue, using active as target queue.
    removeCachedFromQueue(&m_pendingQueue, &m_activeQueue);
    m_pendingQueue.clear();

    // Pending queue should be empty by now and active queue should now have all non cached operations
    if (!m_pendingQueue.empty())
    {
        Logging::Log(m_logCb, Level::Error, "Pending queue is not empty, this is not expected.");
    }
}

void BaseHttpClient::notifyNetworkStateChange() const
{
    if (m_statusCb != nullptr)
    {
        m_statusCb(m_stateReceiverHandle, m_isConnectionOk, m_clientName.c_str());
    }
}

void BaseHttpClient::notifyCachedOperationsProcessed(bool cacheProcessingSucceeded) const
{
    if (m_cachedProcessedCb != nullptr)
    {
        m_cachedProcessedCb(m_cachedProcessedReceiverHandle, cacheProcessingSucceeded);
    }
}

void BaseHttpClient::setStopProcessingOnError(bool stopProcessingOnError)
{
    m_stopProcessingOnError = stopProcessingOnError;
}

bool BaseHttpClient::isResponseCodeRetryable(Aws::Http::HttpResponseCode responseCode)
{
    return responseCode == Aws::Http::HttpResponseCode::REQUEST_NOT_MADE ||
        Aws::Http::IsRetryableHttpResponseCode(responseCode);
}

bool BaseHttpClient::isPendingQueueBelowLimit() const
{
    // no need to lock, the mutex was locked by the caller
    bool belowLimit = m_pendingQueue.size() <= m_maxPendingQueueSize;

    if (!belowLimit)
    {
        Logging::Log(m_logCb, Level::Error, "Size of internal pending queue is above limit. New requests will be dropped.");
    }

    return belowLimit;
}

RequestResult BaseHttpClient::makeOperationRequest(std::shared_ptr<IOperation> operation, bool isAsyncOperation, bool overrideConnectionStatus)
{
    Logging::Log(m_logCb, Level::Verbose, "MakeOperationRequest outgoing request");

    m_authorizationHeaderSetter(operation->Request);

    // Operations set as Async are enqueued for later processing if the queue is running, otherwise they are
    // executed immediately.
    if (isAsyncOperation && m_requestPump.IsRunning())
    {
        // Enqueue
        Logging::Log(m_logCb, Level::Verbose, "Async operation, adding request to queue.");
        
        if (enqueuePending(operation))
        {
            return RequestResult(RequestResultType::RequestEnqueued, std::shared_ptr<Aws::Http::HttpResponse>());
        }
        else
        {
            return RequestResult(RequestResultType::RequestDropped, std::shared_ptr<Aws::Http::HttpResponse>());
        }
    }

    // If connection is healthy or the request pump is not running, make request immediately
    overrideConnectionStatus |= !m_requestPump.IsRunning();
    if ((m_isConnectionOk && !(m_stopProcessingOnError && m_errorDuringProcessing)) || overrideConnectionStatus)
    {
        std::lock_guard<std::mutex> requestLock(m_requestMutex);
        operation->Attempts++;

        // refresh authorization header and send request
        if (m_authorizationHeaderSetter != nullptr)
        {
            m_authorizationHeaderSetter(operation->Request);
        }

        auto requestStart = std::chrono::steady_clock::now();

        auto response = m_httpClient->MakeRequest(operation->Request);

        auto requestEnd = std::chrono::steady_clock::now();
        auto latencyMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(requestEnd - requestStart).count();

        std::string message = "Made request for Operation with timestamp " + std::to_string(operation->Timestamp.count()) + ", Attempts " +
            std::to_string(operation->Attempts) + ", Client-side latency (ms): " + std::to_string(latencyMilliseconds);
        Logging::Log(m_logCb, Level::Verbose, message.c_str());

        // Handle success
        if (response->GetResponseCode() == operation->ExpectedSuccessCode)
        {
            std::string message = "Request succeeded in attempt " + std::to_string(operation->Attempts);
            Logging::Log(m_logCb, Level::Verbose, message.c_str());

            m_retryStrategy->Reset();

            if (operation->SuccessCallback != nullptr)
            {
                operation->SuccessCallback(operation->CallbackContext, response);
            }

            return RequestResult(RequestResultType::RequestMadeSuccess, response);
        }
        else if (isOperationRetryable(operation, response) && m_requestPump.IsRunning())
        {
            // Handle transient error and set network status
            std::string message = "Request failed, setting connection status to \"Unhealthy\".";
            Logging::Log(m_logCb, Level::Warning, message.c_str());
            bool previousConnectionState = m_isConnectionOk;
            m_isConnectionOk = !(response->GetResponseCode() == Aws::Http::HttpResponseCode::REQUEST_NOT_MADE);
            m_errorDuringProcessing = response->GetResponseCode() != Aws::Http::HttpResponseCode::REQUEST_NOT_MADE;

            if (previousConnectionState != m_isConnectionOk)
            {
                notifyNetworkStateChange();
            }

            m_retryStrategy->IncreaseThreshold();

            // Enqueue
            if (enqueuePending(operation))
            {
                Logging::Log(m_logCb, Level::Warning, "Added request to retry queue.");
                return RequestResult(RequestResultType::RequestAttemptedAndEnqueued, response);
            }
            else
            {
                return RequestResult(RequestResultType::RequestDropped, response);
            }
        }
        else
        {
            // Handle permanent error
            Logging::Log(m_logCb, Level::Warning, "Not retryable request failed.");

            // Request failed and is not retryable, return failure
            if (operation->FailureCallback != nullptr)
            {
                operation->FailureCallback(operation->CallbackContext, response);
            }

            return RequestResult(RequestResultType::RequestMadeFailure, response);
        }
    }
    else
    {
        // Connection is unhealthy. If allowed, enqueue for later processing
        Logging::Log(m_logCb, Level::Info, "Connection is Unhealthy, adding operation to pending queue.");
        if (this->shouldEnqueueWithUnhealthyConnection(operation) && enqueuePending(operation))
        {
            return RequestResult(RequestResultType::RequestEnqueued, std::shared_ptr<Aws::Http::HttpResponse>());
        }
        else
        {
            // Connection is unhealthy and enqueueing is not allowed, drop the request.
            Logging::Log(m_logCb, Level::Info, "Connection is Unhealthy, not enqueueing operation.");
            return RequestResult(RequestResultType::RequestDropped, std::shared_ptr<Aws::Http::HttpResponse>());
        }
    }
}
#pragma endregion
