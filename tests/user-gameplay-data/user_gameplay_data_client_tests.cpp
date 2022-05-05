// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// Standard library
#include <fstream>

// AWS SDK
#include <aws/core/utils/StringUtils.h>

// GameKit
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>
#include "user_gameplay_data_client_tests.h"

using namespace Aws::Utils;
using namespace GameKit::Mocks;
using namespace GameKit::Tests;
using namespace GameKit::UserGameplayData;
using namespace GameKit::Utils::HttpClient;

#define MAX_QUEUE_SIZE  8
#define CACHE_BIN_FILE  "./cache_test.dat"
#define INVALID_FILE    "\0"
#define SERIALIZATION_BIN_FILE  "./gameplay_serialization_test.dat"

UserGameplayDataClientTestFixture::UserGameplayDataClientTestFixture()
{}

UserGameplayDataClientTestFixture::~UserGameplayDataClientTestFixture()
{}

void UserGameplayDataClientTestFixture::SetUp()
{
    testStack.Initialize();
    TestLogger::Clear();

    authSetter = std::bind(&UserGameplayDataClientTestFixture::AuthSetter, this, std::placeholders::_1);
    retryLogic = std::make_shared<ConstantIntervalStrategy>();
}

void UserGameplayDataClientTestFixture::TearDown()
{
    testStack.Cleanup();

    remove(CACHE_BIN_FILE);
}

void UserGameplayDataClientTestFixture::AuthSetter(std::shared_ptr<Aws::Http::HttpRequest> request)
{
    request->SetHeaderValue(HEADER_AUTHORIZATION, "Bearer 123XYZ");
}

void UserGameplayDataClientTestFixture::MockResponseCallback(CallbackContext requestContext, std::shared_ptr<Aws::Http::HttpResponse> response)
{
    Aws::Http::HttpResponseCode* responseCode = static_cast<Aws::Http::HttpResponseCode*>(requestContext);
    *responseCode = response->GetResponseCode();
}

void UserGameplayDataClientTestFixture::NetworkStateChangeCb(NETWORK_STATE_RECEIVER_HANDLE dispatchReceiver, bool isConnectionOnline, const char* connectionClient)
{
    bool* newState = static_cast<bool*>(dispatchReceiver);
    *newState = isConnectionOnline;
}

void UserGameplayDataClientTestFixture::CacheProcessedCb(CACHE_PROCESSED_RECEIVER_HANDLE dispatchReceiver, bool cacheProcessed)
{
    bool* newState = static_cast<bool*>(dispatchReceiver);
    *newState = cacheProcessed;
}

TEST_F(UserGameplayDataClientTestFixture, MakeSingleRequest_ClientOnline_WithBackgroundThread_Success)
{
    // Arrange
    using namespace ::testing;

    std::shared_ptr<Aws::Http::HttpRequest> request = std::make_shared<FakeHttpRequest>(
        Aws::Http::URI("https://123.aws.com/foo"), Aws::Http::HttpMethod::HTTP_POST);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(201));

    std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(response));

    // Act
    UserGameplayDataHttpClient client(mockHttpClient, authSetter, 1, retryLogic, MAX_QUEUE_SIZE, TestLogger::Log);
    client.StartRetryBackgroundThread();

    auto result = client.MakeRequest(UserGameplayDataOperationType::Write,
        false, "Foo", "", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT);

    client.StopRetryBackgroundThread();

    // Assert
    ASSERT_EQ(result.ResultType, RequestResultType::RequestMadeSuccess);
    ASSERT_EQ(result.Response->GetResponseCode(), Aws::Http::HttpResponseCode(201));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(UserGameplayDataClientTestFixture, MakeSingleRequest_ClientOnline_WithoutBackgroundThread_Success)
{
    // Arrange
    using namespace ::testing;

    std::shared_ptr<Aws::Http::HttpRequest> request = std::make_shared<FakeHttpRequest>(
        Aws::Http::URI("https://123.aws.com/foo"), Aws::Http::HttpMethod::HTTP_POST);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(201));

    std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(response));

    // Act
    UserGameplayDataHttpClient client(mockHttpClient, authSetter, 1, retryLogic, MAX_QUEUE_SIZE, TestLogger::Log);

    auto result = client.MakeRequest(UserGameplayDataOperationType::Write,
        false, "Foo", "", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT);

    // Assert
    ASSERT_EQ(result.ResultType, RequestResultType::RequestMadeSuccess);
    ASSERT_EQ(result.Response->GetResponseCode(), Aws::Http::HttpResponseCode(201));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(UserGameplayDataClientTestFixture, MakeSingleRequest_ClientOffline_WithBackgroundThread_Retry)
{
    // Arrange
    using namespace ::testing;

    std::shared_ptr<Aws::Http::HttpRequest> request = std::make_shared<FakeHttpRequest>(
        Aws::Http::URI("https://123.aws.com/foo"), Aws::Http::HttpMethod::HTTP_POST);

    std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    std::shared_ptr<Aws::Http::HttpResponse> notMadeResponse = std::make_shared<FakeHttpResponse>();
    notMadeResponse->SetResponseCode(Aws::Http::HttpResponseCode(-1));

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .Times(4)
        .WillRepeatedly(Return(notMadeResponse));

    // Act
    UserGameplayDataHttpClient client(mockHttpClient, authSetter, 1, retryLogic, MAX_QUEUE_SIZE, TestLogger::Log);
    client.StartRetryBackgroundThread();

    auto result = client.MakeRequest(UserGameplayDataOperationType::Write,
        false, "Foo", "", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT);

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    client.StopRetryBackgroundThread();

    // Assert

    ASSERT_EQ(result.ResultType, RequestResultType::RequestAttemptedAndEnqueued);
    ASSERT_EQ(result.Response->GetResponseCode(), Aws::Http::HttpResponseCode(-1));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(UserGameplayDataClientTestFixture, MakeSingleRequest_ClientOffline_WithoutBackgroundThread_NoRetry)
{
    // Arrange
    using namespace ::testing;

    std::shared_ptr<Aws::Http::HttpRequest> request = std::make_shared<FakeHttpRequest>(
        Aws::Http::URI("https://123.aws.com/foo"), Aws::Http::HttpMethod::HTTP_POST);

    std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    std::shared_ptr<Aws::Http::HttpResponse> notMadeResponse = std::make_shared<FakeHttpResponse>();
    notMadeResponse->SetResponseCode(Aws::Http::HttpResponseCode(-1));

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .Times(1)
        .WillRepeatedly(Return(notMadeResponse));

    // Act
    UserGameplayDataHttpClient client(mockHttpClient, authSetter, 1, retryLogic, MAX_QUEUE_SIZE, TestLogger::Log);

    auto result = client.MakeRequest(UserGameplayDataOperationType::Write,
        false, "Foo", "", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT);

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    // Assert

    ASSERT_EQ(result.ResultType, RequestResultType::RequestMadeFailure);
    ASSERT_EQ(result.Response->GetResponseCode(), Aws::Http::HttpResponseCode(-1));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(UserGameplayDataClientTestFixture, MakeSingleRequest_ClientOfflineThenOnline_WithBackgroundThread_EnqueueRetryAndSuccess)
{
    // Arrange
    using namespace ::testing;

    std::shared_ptr<Aws::Http::HttpRequest> request = std::make_shared<FakeHttpRequest>(
        Aws::Http::URI("https://123.aws.com/foo"), Aws::Http::HttpMethod::HTTP_POST);

    std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    std::shared_ptr<Aws::Http::HttpResponse> notMadeResponse = std::make_shared<FakeHttpResponse>();
    notMadeResponse->SetResponseCode(Aws::Http::HttpResponseCode(-1));

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode(201));

    Aws::Http::HttpResponseCode responseCode = Aws::Http::HttpResponseCode(-1);
    ResponseCallback responseCallback = 
        std::bind(&UserGameplayDataClientTestFixture::MockResponseCallback, this, std::placeholders::_1, std::placeholders::_2);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(notMadeResponse))
        .WillOnce(Return(successResponse));

    // Act
    UserGameplayDataHttpClient client(mockHttpClient, authSetter, 1, retryLogic, MAX_QUEUE_SIZE, TestLogger::Log);
    client.StartRetryBackgroundThread();

    auto result = client.MakeRequest(UserGameplayDataOperationType::Write,
        false, "Foo", "", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT,
        (CallbackContext)(&responseCode), responseCallback);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    client.StopRetryBackgroundThread();

    // Assert
    ASSERT_EQ(result.ResultType, RequestResultType::RequestAttemptedAndEnqueued);
    ASSERT_EQ(result.Response->GetResponseCode(), Aws::Http::HttpResponseCode(-1));
    ASSERT_EQ(responseCode, Aws::Http::HttpResponseCode(201));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(UserGameplayDataClientTestFixture, MakeMultipleRequests_ClientOfflineThenOnline_WithoutBackgroundThread_FailAndSuccess)
{
    // Arrange
    using namespace ::testing;

    std::shared_ptr<Aws::Http::HttpRequest> request = std::make_shared<FakeHttpRequest>(
        Aws::Http::URI("https://123.aws.com/foo"), Aws::Http::HttpMethod::HTTP_POST);

    std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    std::shared_ptr<Aws::Http::HttpResponse> notMadeResponse = std::make_shared<FakeHttpResponse>();
    notMadeResponse->SetResponseCode(Aws::Http::HttpResponseCode(-1));

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode(201));

    Aws::Http::HttpResponseCode responseCode1 = Aws::Http::HttpResponseCode(-1);
    Aws::Http::HttpResponseCode responseCode2 = Aws::Http::HttpResponseCode(201);
    ResponseCallback responseCallback =
        std::bind(&UserGameplayDataClientTestFixture::MockResponseCallback, this, std::placeholders::_1, std::placeholders::_2);

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(notMadeResponse))
        .WillOnce(Return(successResponse));

    // Act
    UserGameplayDataHttpClient client(mockHttpClient, authSetter, 1, retryLogic, MAX_QUEUE_SIZE, TestLogger::Log);
    
    auto result1 = client.MakeRequest(UserGameplayDataOperationType::Write,
        false, "Foo1", "", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT,
        (CallbackContext)(&responseCode1), responseCallback);

    auto result2 = client.MakeRequest(UserGameplayDataOperationType::Write,
        false, "Foo2", "", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT,
        (CallbackContext)(&responseCode2), responseCallback);

    // Assert
    ASSERT_EQ(result1.ResultType, RequestResultType::RequestMadeFailure);
    ASSERT_EQ(result1.Response->GetResponseCode(), Aws::Http::HttpResponseCode(-1));
    ASSERT_EQ(responseCode1, Aws::Http::HttpResponseCode(-1));

    ASSERT_EQ(result2.ResultType, RequestResultType::RequestMadeSuccess);
    ASSERT_EQ(result2.Response->GetResponseCode(), Aws::Http::HttpResponseCode(201));
    ASSERT_EQ(responseCode2, Aws::Http::HttpResponseCode(201));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(UserGameplayDataClientTestFixture, MakeMultipleRequests_ClientOfflineThenOnline_WithBackgroundThread_EnqueueRetryAndSuccess)
{
    // Arrange
    using namespace ::testing;

    std::shared_ptr<Aws::Http::HttpRequest> request = std::make_shared<FakeHttpRequest>(
        Aws::Http::URI("https://123.aws.com/foo"), Aws::Http::HttpMethod::HTTP_POST);

    std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    std::shared_ptr<Aws::Http::HttpResponse> notMadeResponse = std::make_shared<FakeHttpResponse>();
    notMadeResponse->SetResponseCode(Aws::Http::HttpResponseCode(-1));

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode(201));

    Aws::Http::HttpResponseCode responseCode1 = Aws::Http::HttpResponseCode(-1);
    Aws::Http::HttpResponseCode responseCode2 = Aws::Http::HttpResponseCode(-1);
    ResponseCallback successCallback =
        std::bind(&UserGameplayDataClientTestFixture::MockResponseCallback, this, std::placeholders::_1, std::placeholders::_2);

    NetworkStatusChangeCallback networkStateCallback = &UserGameplayDataClientTestFixture::NetworkStateChangeCb;

    EXPECT_CALL(*mockHttpClient, MakeRequest(_, _, _))
        .WillOnce(Return(notMadeResponse))
        .WillOnce(Return(notMadeResponse))
        .WillOnce(Return(successResponse))
        .WillOnce(Return(successResponse));

    bool networkState = true; // start assuming online mode
    
    // Act
    UserGameplayDataHttpClient client(mockHttpClient, authSetter, 1, retryLogic, MAX_QUEUE_SIZE, TestLogger::Log);
    client.SetNetworkChangeCallback(&networkState, networkStateCallback);
    client.StartRetryBackgroundThread();

    auto result1 = client.MakeRequest(UserGameplayDataOperationType::Write,
        false, "Foo", "", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT,
        (CallbackContext)(&responseCode1), successCallback);

    bool state_t_0 = networkState; // state should be in error state (false), as captured by callback

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    auto result2 = client.MakeRequest(UserGameplayDataOperationType::Write,
        false, "Foo", "", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT,
        (CallbackContext)(&responseCode2), successCallback);

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    bool state_t_1 = networkState; // state should be healthy (true), as captured by callback

    client.StopRetryBackgroundThread();

    // Assert
    ASSERT_EQ(result1.ResultType, RequestResultType::RequestAttemptedAndEnqueued);
    ASSERT_EQ(result1.Response->GetResponseCode(), Aws::Http::HttpResponseCode(-1));
    
    ASSERT_EQ(result2.ResultType, RequestResultType::RequestEnqueued);
    ASSERT_EQ(result2.Response.get(), nullptr);

    ASSERT_EQ(responseCode1, Aws::Http::HttpResponseCode(201));
    ASSERT_EQ(responseCode2, Aws::Http::HttpResponseCode(201));

    ASSERT_FALSE(state_t_0);
    ASSERT_TRUE(state_t_1);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(UserGameplayDataClientTestFixture, MakeOperation_BinarySerializeDeserialize_OperationsMatch)
{
    // Arrange
    using namespace Aws::Http;
    using namespace GameKit::UserGameplayData;

    Aws::String uri = "https://domain/path";

    auto request = Aws::Http::CreateHttpRequest(uri, HttpMethod::HTTP_POST, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
    request->SetHeaderValue(HEADER_AUTHORIZATION, "FooAuth123");
    request->AddQueryStringParameter("foo", "bar");

    Aws::Utils::Json::JsonValue payload;
    payload.WithString("Potions", "1");
    payload.WithString("Food", "2");

    std::shared_ptr<Aws::IOStream> payloadStream = Aws::MakeShared<Aws::StringStream>("AddUserGameplayDataBody");
    std::string serialized = ToStdString(payload.View().WriteCompact());
    *payloadStream << serialized;

    request->AddContentBody(payloadStream);
    request->SetContentType("application/json");
    request->SetContentLength(StringUtils::to_string(serialized.size()));

    std::shared_ptr<UserGameplayDataOperation> operation = std::make_shared<UserGameplayDataOperation>(
        UserGameplayDataOperationType::Write, "Inventory", "Items", request, HttpResponseCode::CREATED, 123);

    // Act
    std::ofstream os(SERIALIZATION_BIN_FILE, std::ios::binary);
    bool serializeResult = UserGameplayDataOperation::TrySerializeBinary(os, operation);
    os.close();

    std::ifstream is(SERIALIZATION_BIN_FILE, std::ios::binary);
    std::shared_ptr<UserGameplayDataOperation> deserialized;
    bool deserializeResult = UserGameplayDataOperation::TryDeserializeBinary(is, deserialized);
    is.close();

    // Assert
    ASSERT_TRUE(serializeResult);
    ASSERT_TRUE(deserializeResult);

    ASSERT_EQ(operation->Attempts, deserialized->Attempts);
    ASSERT_STREQ(operation->Bundle.c_str(), deserialized->Bundle.c_str());
    ASSERT_EQ(operation->Discard, deserialized->Discard);
    ASSERT_EQ(operation->ExpectedSuccessCode, deserialized->ExpectedSuccessCode);
    ASSERT_STREQ(operation->ItemKey.c_str(), deserialized->ItemKey.c_str());
    ASSERT_EQ(operation->MaxAttempts, deserialized->MaxAttempts);
    ASSERT_STREQ(operation->OperationUniqueKey.c_str(), deserialized->OperationUniqueKey.c_str());
    ASSERT_EQ(operation->Timestamp, deserialized->Timestamp);
    ASSERT_EQ(operation->Type, deserialized->Type);

    // Inner request serialization is tested in GameKitRequestSerializationTestFixture
}

TEST_F(UserGameplayDataClientTestFixture, MakeMultipleRequests_SerializeToCache_ReloadFromCache)
{
    // Arrange
    using namespace ::testing;

    std::shared_ptr<Aws::Http::HttpRequest> request = Aws::Http::CreateHttpRequest(Aws::String("https://123.aws.com/foo"), Aws::Http::HttpMethod::HTTP_POST, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

    std::shared_ptr<MockHttpClient> mockHttpClient1 = std::make_shared<MockHttpClient>();
    std::shared_ptr<MockHttpClient> mockHttpClient2 = std::make_shared<MockHttpClient>();

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode(201));

    CacheProcessedCallback cacheFinishedCallback = &UserGameplayDataClientTestFixture::CacheProcessedCb;

    auto serializer = static_cast<bool(*)(std::ostream&, const std::shared_ptr<IOperation>, FuncLogCallback)>(&UserGameplayDataOperation::TrySerializeBinary);
    auto deserializer = static_cast<bool(*)(std::istream&, std::shared_ptr<IOperation>&, FuncLogCallback)>(&UserGameplayDataOperation::TryDeserializeBinary);
    
    ON_CALL(*mockHttpClient1, MakeRequest(_, _, _))
        .WillByDefault(Throw<std::exception>(std::runtime_error("MakeRequest should not have been called.")));

    EXPECT_CALL(*mockHttpClient2, MakeRequest(_, _, _))
        .WillOnce(Return(successResponse))
        .WillOnce(Return(successResponse));

    bool cachedCallsFinished = false; // start assuming no operations are in the cache already

    // Act
    // Enqueue requests on a client and persist to disk, then load the requests on another client and process them

    RequestResultType resultType1;
    RequestResultType resultType2;
    bool persistResult = false;
    bool fileExistsAfterPersisting = false;
    bool loadResult = false;
    bool fileExistsAfterLoading = false;
    {
        unsigned int retryIntervalSeconds = 10;
        UserGameplayDataHttpClient client(mockHttpClient1, authSetter, retryIntervalSeconds, retryLogic, MAX_QUEUE_SIZE, TestLogger::Log);
        client.SetCacheProcessedCallback(&cachedCallsFinished, cacheFinishedCallback);
        client.StartRetryBackgroundThread();

        auto result1 = client.MakeRequest(UserGameplayDataOperationType::Write,
            true, "Foo1", "Bar1", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT);
        resultType1 = result1.ResultType;

        auto result2 = client.MakeRequest(UserGameplayDataOperationType::Delete,
            true, "Foo2", "Bar2", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT);
        resultType2 = result2.ResultType;

        // wait some time, but requests shouldn't be sent
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        client.StopRetryBackgroundThread();
        persistResult = client.PersistQueue(CACHE_BIN_FILE, serializer);
        fileExistsAfterPersisting = boost::filesystem::exists(CACHE_BIN_FILE);
    }

    unsigned int retryIntervalSeconds = 1;
    UserGameplayDataHttpClient client2(mockHttpClient2, authSetter, retryIntervalSeconds, retryLogic, MAX_QUEUE_SIZE, TestLogger::Log);
    client2.SetCacheProcessedCallback(&cachedCallsFinished, cacheFinishedCallback);

    bool state_t_1 = cachedCallsFinished;

    loadResult = client2.LoadQueue(CACHE_BIN_FILE, deserializer);
    fileExistsAfterLoading = boost::filesystem::exists(CACHE_BIN_FILE);
    client2.StartRetryBackgroundThread();

    // wait some time, loaded requests should be sent
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    client2.StopRetryBackgroundThread();

    bool state_t_2 = cachedCallsFinished;

    // Assert
    ASSERT_TRUE(persistResult);
    ASSERT_TRUE(fileExistsAfterPersisting);
    ASSERT_TRUE(loadResult);
    ASSERT_FALSE(fileExistsAfterLoading);
    ASSERT_EQ(resultType1, RequestResultType::RequestEnqueued);
    ASSERT_EQ(resultType2, RequestResultType::RequestEnqueued);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient1.get()));

    // client 2 expectations are met if the queue was loaded and processed
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient2.get()));

    // developer is notified that the cache has been successfully processed
    ASSERT_FALSE(state_t_1);
    ASSERT_TRUE(state_t_2);
}

TEST_F(UserGameplayDataClientTestFixture, MakeMultipleRequests_ReloadFromCache_ProcessingCacheFailedCallback)
{
    // Arrange
    using namespace ::testing;

    std::shared_ptr<Aws::Http::HttpRequest> request = Aws::Http::CreateHttpRequest(Aws::String("https://123.aws.com/foo"), Aws::Http::HttpMethod::HTTP_POST, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

    std::shared_ptr<MockHttpClient> mockHttpClient1 = std::make_shared<MockHttpClient>();
    std::shared_ptr<MockHttpClient> mockHttpClient2 = std::make_shared<MockHttpClient>();

    std::shared_ptr<Aws::Http::HttpResponse> notMadeResponse = std::make_shared<FakeHttpResponse>();
    notMadeResponse->SetResponseCode(Aws::Http::HttpResponseCode(-1));

    CacheProcessedCallback cacheFinishedCallback = &UserGameplayDataClientTestFixture::CacheProcessedCb;

    auto serializer = static_cast<bool(*)(std::ostream&, const std::shared_ptr<IOperation>, FuncLogCallback)>(&UserGameplayDataOperation::TrySerializeBinary);
    auto deserializer = static_cast<bool(*)(std::istream&, std::shared_ptr<IOperation>&, FuncLogCallback)>(&UserGameplayDataOperation::TryDeserializeBinary);

    ON_CALL(*mockHttpClient1, MakeRequest(_, _, _))
        .WillByDefault(Throw<std::exception>(std::runtime_error("MakeRequest should not have been called.")));

    EXPECT_CALL(*mockHttpClient2, MakeRequest(_, _, _))
        .WillOnce(Return(notMadeResponse));

    bool cachedCallsFinished = true; // in order to to test for a failure callback, we set this to true

    // Act
    // Enqueue requests on a client and persist to disk, then load the requests on another client, delete the cached requests and make sure none are processed

    RequestResultType resultType1;
    RequestResultType resultType2;
    bool persistResult = false;
    bool loadResult = false;
    {
        unsigned int retryIntervalSeconds = 10;
        UserGameplayDataHttpClient client(mockHttpClient1, authSetter, retryIntervalSeconds, retryLogic, MAX_QUEUE_SIZE, TestLogger::Log);
        client.SetCacheProcessedCallback(&cachedCallsFinished, cacheFinishedCallback);
        client.StartRetryBackgroundThread();

        auto result1 = client.MakeRequest(UserGameplayDataOperationType::Write,
            true, "Foo1", "Bar1", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT);
        resultType1 = result1.ResultType;

        auto result2 = client.MakeRequest(UserGameplayDataOperationType::Delete,
            true, "Foo2", "Bar2", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT);
        resultType2 = result2.ResultType;

        // wait some time, but requests shouldn't be sent due to long interval
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        client.StopRetryBackgroundThread();
        persistResult = client.PersistQueue(CACHE_BIN_FILE, serializer);
    }

    {
        unsigned int retryIntervalSeconds = 1;
        UserGameplayDataHttpClient client2(mockHttpClient2, authSetter, retryIntervalSeconds, retryLogic, MAX_QUEUE_SIZE, TestLogger::Log);
        client2.SetCacheProcessedCallback(&cachedCallsFinished, cacheFinishedCallback);

        loadResult = client2.LoadQueue(CACHE_BIN_FILE, deserializer);
        client2.StartRetryBackgroundThread();
;
        // wait some time, loaded requests should be sent
        std::this_thread::sleep_for(std::chrono::milliseconds(1200));
        client2.StopRetryBackgroundThread();
    }

    // Assert
    ASSERT_FALSE(cachedCallsFinished);  // Developer has been notified that the cached requests have failed

    ASSERT_TRUE(persistResult);
    ASSERT_TRUE(loadResult);
    ASSERT_EQ(resultType1, RequestResultType::RequestEnqueued);
    ASSERT_EQ(resultType2, RequestResultType::RequestEnqueued);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient1.get()));

    // client 2 expectations are met if the queue was loaded and processed
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient2.get()));
}

TEST_F(UserGameplayDataClientTestFixture, MakeMultipleRequests_ReloadFromCache_DeleteCachedOps)
{
    // Arrange
    using namespace ::testing;

    std::shared_ptr<Aws::Http::HttpRequest> request = Aws::Http::CreateHttpRequest(Aws::String("https://123.aws.com/foo"), Aws::Http::HttpMethod::HTTP_POST, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

    std::shared_ptr<MockHttpClient> mockHttpClient1 = std::make_shared<MockHttpClient>();
    std::shared_ptr<MockHttpClient> mockHttpClient2 = std::make_shared<MockHttpClient>();

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode(201));

    CacheProcessedCallback cacheFinishedCallback = &UserGameplayDataClientTestFixture::CacheProcessedCb;

    auto serializer = static_cast<bool(*)(std::ostream&, const std::shared_ptr<IOperation>, FuncLogCallback)>(&UserGameplayDataOperation::TrySerializeBinary);
    auto deserializer = static_cast<bool(*)(std::istream&, std::shared_ptr<IOperation>&, FuncLogCallback)>(&UserGameplayDataOperation::TryDeserializeBinary);

    ON_CALL(*mockHttpClient1, MakeRequest(_, _, _))
        .WillByDefault(Throw<std::exception>(std::runtime_error("MakeRequest should not have been called.")));

    EXPECT_CALL(*mockHttpClient2, MakeRequest(_, _, _))
        .Times(0);

    bool cachedCallsFinished = false; // start assuming no operations are in the cache already

    // Act
    // Enqueue requests on a client and persist to disk, then load the requests on another client, delete the cached requests and make sure none are processed

    RequestResultType resultType1;
    RequestResultType resultType2;
    bool persistResult = false;
    bool loadResult = false;
    bool isAsyncCall = true; // async calls are enqueued by design
    {
        unsigned int retryIntervalSeconds = 10; // long interval to give time to enqueue and persist before making requests
        UserGameplayDataHttpClient client(mockHttpClient1, authSetter, retryIntervalSeconds, retryLogic, MAX_QUEUE_SIZE, TestLogger::Log);
        client.SetCacheProcessedCallback(&cachedCallsFinished, cacheFinishedCallback);

        // If the retry background thread is not running, async calls would be attempted immediately
        client.StartRetryBackgroundThread(); 

        auto result1 = client.MakeRequest(UserGameplayDataOperationType::Write,
            isAsyncCall, "Foo1", "Bar1", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT);
        resultType1 = result1.ResultType;

        auto result2 = client.MakeRequest(UserGameplayDataOperationType::Delete,
            isAsyncCall, "Foo2", "Bar2", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT);
        resultType2 = result2.ResultType;

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        client.StopRetryBackgroundThread();
        persistResult = client.PersistQueue(CACHE_BIN_FILE, serializer);
    }

    {
        unsigned int retryIntervalSeconds = 1;
        UserGameplayDataHttpClient client2(mockHttpClient2, authSetter, 1, retryLogic, MAX_QUEUE_SIZE, TestLogger::Log);
        client2.SetCacheProcessedCallback(&cachedCallsFinished, cacheFinishedCallback);

        loadResult = client2.LoadQueue(CACHE_BIN_FILE, deserializer);
        client2.DropAllCachedEvents();
        client2.StartRetryBackgroundThread();

        // wait some time, there should be no loaded requests since all from cache were deleted
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        client2.StopRetryBackgroundThread();
    }

    // Assert
    ASSERT_TRUE(persistResult);
    ASSERT_TRUE(loadResult);
    ASSERT_EQ(resultType1, RequestResultType::RequestEnqueued);
    ASSERT_EQ(resultType2, RequestResultType::RequestEnqueued);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient1.get()));

    // client 2 expectations are met if the queue was loaded and deleted
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient2.get()));
}

TEST_F(UserGameplayDataClientTestFixture, MakeMultipleRequests_SerializeToInvalidPath_ReturnsFalse)
{
    // Arrange
    using namespace ::testing;

    std::shared_ptr<Aws::Http::HttpRequest> request = Aws::Http::CreateHttpRequest(Aws::String("https://123.aws.com/foo"), Aws::Http::HttpMethod::HTTP_POST, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

    std::shared_ptr<MockHttpClient> mockHttpClient1 = std::make_shared<MockHttpClient>();

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode(201));

    auto serializer = static_cast<bool(*)(std::ostream&, const std::shared_ptr<IOperation>, FuncLogCallback)>(&UserGameplayDataOperation::TrySerializeBinary);
    auto deserializer = static_cast<bool(*)(std::istream&, std::shared_ptr<IOperation>&, FuncLogCallback)>(&UserGameplayDataOperation::TryDeserializeBinary);

    ON_CALL(*mockHttpClient1, MakeRequest(_, _, _))
        .WillByDefault(Throw<std::exception>(std::runtime_error("MakeRequest should not have been called.")));

    // Act
    // Enqueue requests on a client and attempts persist to disk

    RequestResultType resultType1;
    bool persistResult = false;
    {
        unsigned int intervalRetrySeconds = 10;
        UserGameplayDataHttpClient client(mockHttpClient1, authSetter, 10, retryLogic, MAX_QUEUE_SIZE, TestLogger::Log);
        client.StartRetryBackgroundThread();

        auto result1 = client.MakeRequest(UserGameplayDataOperationType::Write,
            true, "Foo1", "Bar1", request, Aws::Http::HttpResponseCode(201), OPERATION_ATTEMPTS_NO_LIMIT);
        resultType1 = result1.ResultType;

        client.StopRetryBackgroundThread();
        persistResult = client.PersistQueue(INVALID_FILE, serializer);
    }

    // Assert
    ASSERT_FALSE(persistResult);
    ASSERT_EQ(resultType1, RequestResultType::RequestEnqueued);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient1.get()));
}

TEST_F(UserGameplayDataClientTestFixture, MakeMultipleRequests_DeserializeFromInvalidPath_ReturnsFalse)
{
    // Arrange
    using namespace ::testing;

    std::shared_ptr<Aws::Http::HttpRequest> request = Aws::Http::CreateHttpRequest(Aws::String("https://123.aws.com/foo"), Aws::Http::HttpMethod::HTTP_POST, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

    std::shared_ptr<MockHttpClient> mockHttpClient1 = std::make_shared<MockHttpClient>();

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode(201));

    auto serializer = static_cast<bool(*)(std::ostream&, const std::shared_ptr<IOperation>, FuncLogCallback)>(&UserGameplayDataOperation::TrySerializeBinary);
    auto deserializer = static_cast<bool(*)(std::istream&, std::shared_ptr<IOperation>&, FuncLogCallback)>(&UserGameplayDataOperation::TryDeserializeBinary);

    ON_CALL(*mockHttpClient1, MakeRequest(_, _, _))
        .WillByDefault(Throw<std::exception>(std::runtime_error("MakeRequest should not have been called.")));

    // Act
    // Enqueue requests on a client and attempts persist to disk
    bool loadResult = false;
    {
        UserGameplayDataHttpClient client2(mockHttpClient1, authSetter, 1, retryLogic, MAX_QUEUE_SIZE, TestLogger::Log);

        loadResult = client2.LoadQueue(INVALID_FILE, deserializer);
    }

    // Assert
    ASSERT_FALSE(loadResult);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient1.get()));
}
