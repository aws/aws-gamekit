// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// AWS SDK
#include <aws/core/utils/StringUtils.h>

// GameKit
#include "user_gameplay_exports_tests.h"
#include "../core/dispatchers.h"
#include "aws/gamekit/user-gameplay-data/exports.h"
#include "aws/gamekit/user-gameplay-data/gamekit_user_gameplay_data_models.h"
#include "aws/gamekit/authentication/exports.h"

using namespace GameKit::Tests;
using namespace ::testing;

#define TEST_ID_TOKEN   "test_token123"
#define TEST_AUTH_HEADER    "Bearer test_token123"

void* GameKitUserGameplayDataExportsTestFixture::CreateDefault()
{
    return GameKitUserGameplayDataInstanceCreateWithSessionManager(sessionManagerInstance, nullptr);
}

void GameKitUserGameplayDataExportsTestFixture::SetMocks(void* handle, std::shared_ptr<Aws::Http::HttpClient> mockHttpClient)
{
    UserGameplayData::UserGameplayData* userGameplayDataInstance = static_cast<GameKit::UserGameplayData::UserGameplayData*>(handle);
    userGameplayDataInstance->setHttpClient(mockHttpClient);
}

bool GameKitUserGameplayDataExportsTestFixture::ValidateItemKeysProxy(const char* const* bundleItemKeys, int numKeys, std::stringstream& tempBuffer)
{
    // Need to wrap the method under test here because the tests created with the TEST_F macro are different classes.
    return UserGameplayData::UserGameplayData::validateBundleItemKeys(bundleItemKeys, numKeys, tempBuffer);
}

void GameKitUserGameplayDataExportsTestFixture::SetUp()
{
    TestLogger::Clear();
    testStack.Initialize();

    sessionManagerInstance = GameKitSessionManagerInstanceCreate("../core/test_data/sampleplugin/instance/testgame/dev/awsGameKitClientConfig.yml", nullptr);
    static_cast<Authentication::GameKitSessionManager*>(sessionManagerInstance)->SetToken(TokenType::IdToken, TEST_ID_TOKEN);
}

void GameKitUserGameplayDataExportsTestFixture::TearDown()
{
    GameKitSessionManagerInstanceRelease(sessionManagerInstance);

    testStack.Cleanup();
}

TEST_F(GameKitUserGameplayDataExportsTestFixture, TestCreate_Success)
{
    // act
    void* instance = GameKitUserGameplayDataInstanceCreateWithSessionManager(sessionManagerInstance, nullptr);

    // assert
    ASSERT_NE(instance, nullptr);

    GameKitUserGameplayDataInstanceRelease(instance);
}

/*
Test below verify that Gameplay data requests are well formed and that the responses are parsed as expected.
Background thread and network state callback are tested by UserGameplayDataClientTestFixture in user_gameplay_data_client_tests.cpp.
*/

TEST_F(GameKitUserGameplayDataExportsTestFixture, TestAddBundle_RequestIsWellFormed_Success)
{
    // arrange
    GameKit::UserGameplayDataBundle bundle;
    bundle.bundleName = "TestBundle";
    bundle.numKeys = 2;
    const char* keys[2] = { "k1", "k2" };
    bundle.bundleItemKeys = keys;
    const char* values[2] = { "v1", "v2" };
    bundle.bundleItemValues = values;

    void* instance = CreateDefault();
    const std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    SetMocks(instance, mockHttpClient);

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode::CREATED);
    static_cast<FakeHttpResponse*>(successResponse.get())->SetResponseBody(
        "{\"data\":{\"unprocessed_items\":[]}}");


    std::shared_ptr<Aws::Http::HttpRequest> actualRequest;

    auto saveRequestAndReturnResponse = [&actualRequest, &successResponse](const std::shared_ptr<Aws::Http::HttpRequest>& request)
    {
        actualRequest = request;

        return successResponse;
    };

    EXPECT_CALL(
        *mockHttpClient,
        MakeRequest(_, _, _)).
        WillOnce(WithArg<0>(saveRequestAndReturnResponse));

    std::map<std::string, std::string> retrievedPairs;
    auto unprocessedItemsSetter = [&retrievedPairs](const char* key, const char* value)
    {
        retrievedPairs[key] = value;
    };
    typedef LambdaDispatcher<decltype(unprocessedItemsSetter), void, const char*, const char*> UnprocessedItemsSetter;

    // act
    unsigned int result = GameKitAddUserGameplayData(instance, bundle, &unprocessedItemsSetter, UnprocessedItemsSetter::Dispatch);
    GameKitUserGameplayDataInstanceRelease(instance);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);
    
    ASSERT_STREQ("https://domain.tld/usergamedata/bundles/TestBundle", actualRequest->GetURIString().c_str());
    ASSERT_EQ(Aws::Http::HttpMethod::HTTP_POST, actualRequest->GetMethod());
    ASSERT_STREQ(TEST_AUTH_HEADER, actualRequest->GetAuthorization().c_str());
    ASSERT_STRCASEEQ("application/json", actualRequest->GetContentType().c_str());
    std::stringstream bodyStream;
    bodyStream << actualRequest->GetContentBody()->rdbuf();
    ASSERT_STRCASEEQ("{\"k1\":\"v1\",\"k2\":\"v2\"}", bodyStream.str().c_str());

    // assert we have no unprocessed values
    ASSERT_TRUE(retrievedPairs.empty());

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(GameKitUserGameplayDataExportsTestFixture, TestAddBundles_RequestIsWellFormed_FailedProcessingSome)
{
    // arrange
    GameKit::UserGameplayDataBundle bundle;
    bundle.bundleName = "TestBundle";
    bundle.numKeys = 2;
    const char* keys[2] = { "k1", "k2" };
    bundle.bundleItemKeys = keys;
    const char* values[2] = { "v1", "v2" };
    bundle.bundleItemValues = values;

    // arrange
    void* instance = CreateDefault();
    const std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    SetMocks(instance, mockHttpClient);

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode::CREATED);
    static_cast<FakeHttpResponse*>(successResponse.get())->SetResponseBody(
        "{\"data\":{\"unprocessed_items\":[{\"bundle_item_key\": \"k2\", \"bundle_item_value\": \"v2\"}]}}");

    std::shared_ptr<Aws::Http::HttpRequest> actualRequest;

    auto saveRequestAndReturnResponse = [&actualRequest, &successResponse](const std::shared_ptr<Aws::Http::HttpRequest>& request)
    {
        actualRequest = request;

        return successResponse;
    };

    EXPECT_CALL(
        *mockHttpClient,
        MakeRequest(_, _, _)).
        WillOnce(WithArg<0>(saveRequestAndReturnResponse));

    std::map<std::string, std::string> retrievedPairs;
    auto unprocessedItemsSetter = [&retrievedPairs](const char* key, const char* value)
    {
        retrievedPairs[key] = value;
    };
    typedef LambdaDispatcher<decltype(unprocessedItemsSetter), void, const char*, const char*> UnprocessedItemsSetter;

    // act
    unsigned int result = GameKitAddUserGameplayData(instance, bundle, &unprocessedItemsSetter, UnprocessedItemsSetter::Dispatch);
    GameKitUserGameplayDataInstanceRelease(instance);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_USER_GAMEPLAY_DATA_UNPROCESSED_ITEMS);

    ASSERT_STREQ("https://domain.tld/usergamedata/bundles/TestBundle", actualRequest->GetURIString().c_str());
    ASSERT_EQ(Aws::Http::HttpMethod::HTTP_POST, actualRequest->GetMethod());
    ASSERT_STREQ(TEST_AUTH_HEADER, actualRequest->GetAuthorization().c_str());
    ASSERT_STRCASEEQ("application/json", actualRequest->GetContentType().c_str());
    std::stringstream bodyStream;
    bodyStream << actualRequest->GetContentBody()->rdbuf();
    ASSERT_STRCASEEQ("{\"k1\":\"v1\",\"k2\":\"v2\"}", bodyStream.str().c_str());

    // assert we have the expected unprocessed values
    ASSERT_EQ(1, retrievedPairs.size());
    ASSERT_EQ(1, retrievedPairs.count("k2"));
    ASSERT_STREQ("v2", retrievedPairs["k2"].c_str());

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(GameKitUserGameplayDataExportsTestFixture, TestListBundles_RequestIsWellFormed_Success)
{
    // arrange
    std::string bundle = "TestBundle";
    void* instance = CreateDefault();
    const std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    SetMocks(instance, mockHttpClient);

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode::OK);
    static_cast<FakeHttpResponse*>(successResponse.get())->SetResponseBody(
        "{\"data\":{\"bundle_names\":[{\"bundle_name\":\"b1\"},{\"bundle_name\":\"b2\"}]}}");

    std::shared_ptr<Aws::Http::HttpRequest> actualRequest;

    auto saveRequestAndReturnResponse = [&actualRequest, &successResponse](const std::shared_ptr<Aws::Http::HttpRequest>& request)
    {
        actualRequest = request;

        return successResponse;
    };

    EXPECT_CALL(
        *mockHttpClient,
        MakeRequest(_, _, _)).
        WillOnce(WithArg<0>(saveRequestAndReturnResponse));

    std::vector<std::string> retrievedNames;
    auto bundleNamesSetter = [&retrievedNames](const char* bundleName)
    {
        retrievedNames.push_back(bundleName);
    };
    typedef LambdaDispatcher<decltype(bundleNamesSetter), void, const char*> BundleNamesSetter;

    // act
    const unsigned int result = GameKitListUserGameplayDataBundles(instance, &bundleNamesSetter, BundleNamesSetter::Dispatch);
    GameKitUserGameplayDataInstanceRelease(instance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    ASSERT_STREQ("b1", retrievedNames[0].c_str());
    ASSERT_STREQ("b2", retrievedNames[1].c_str());

    ASSERT_STREQ("https://domain.tld/usergamedata/bundles?limit=100", actualRequest->GetURIString().c_str());
    ASSERT_EQ(Aws::Http::HttpMethod::HTTP_GET, actualRequest->GetMethod());
    ASSERT_STREQ(TEST_AUTH_HEADER, actualRequest->GetAuthorization().c_str());

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(GameKitUserGameplayDataExportsTestFixture, TestGetBundle_RequestIsWellFormed_Success)
{
    // arrange
    char* bundle = "TestBundle";
    void* instance = CreateDefault();
    const std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    SetMocks(instance, mockHttpClient);

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode::OK);
    static_cast<FakeHttpResponse*>(successResponse.get())->SetResponseBody(
        "{\"data\":{\"bundle_items\":[{\"bundle_item_key\":\"k1\",\"bundle_item_value\":\"v1\"},{\"bundle_item_key\":\"k2\",\"bundle_item_value\":\"v2\"}]}}");

    std::shared_ptr<Aws::Http::HttpRequest> actualRequest;

    auto saveRequestAndReturnResponse = [&actualRequest, &successResponse](const std::shared_ptr<Aws::Http::HttpRequest>& request)
    {
        actualRequest = request;

        return successResponse;
    };

    EXPECT_CALL(
        *mockHttpClient,
        MakeRequest(_, _, _)).
        WillOnce(WithArg<0>(saveRequestAndReturnResponse));

    std::map<std::string, std::string> retrievedPairs;
    auto bundleSetter = [&retrievedPairs](const char* key, const char* value)
    {
        retrievedPairs[key] = value;
    };
    typedef LambdaDispatcher<decltype(bundleSetter), void, const char*, const char*> BundleSetter;

    // act
    unsigned int result = GameKitGetUserGameplayDataBundle(instance, bundle, &bundleSetter, BundleSetter::Dispatch);
    GameKitUserGameplayDataInstanceRelease(instance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    ASSERT_STREQ("v1", retrievedPairs["k1"].c_str());
    ASSERT_STREQ("v2", retrievedPairs["k2"].c_str());

    ASSERT_STREQ("https://domain.tld/usergamedata/bundles/TestBundle?limit=100", actualRequest->GetURIString().c_str());
    ASSERT_EQ(Aws::Http::HttpMethod::HTTP_GET, actualRequest->GetMethod());
    ASSERT_STREQ(TEST_AUTH_HEADER, actualRequest->GetAuthorization().c_str());

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(GameKitUserGameplayDataExportsTestFixture, TestGetBundleItem_RequestIsWellFormed_Success)
{
    // arrange
    GameKit::UserGameplayDataBundleItem bundleItem;
    bundleItem.bundleName = "TestBundle";
    bundleItem.bundleItemKey = "k1";
    void* instance = CreateDefault();
    std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    SetMocks(instance, mockHttpClient);

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode::OK);
    static_cast<FakeHttpResponse*>(successResponse.get())->SetResponseBody(
        "{\"data\":{\"bundle_item_value\":\"123\"}}");

    std::shared_ptr<Aws::Http::HttpRequest> actualRequest;

    auto saveRequestAndReturnResponse = [&actualRequest, &successResponse](const std::shared_ptr<Aws::Http::HttpRequest>& request)
    {
        actualRequest = request;

        return successResponse;
    };

    EXPECT_CALL(
        *mockHttpClient,
        MakeRequest(_, _, _)).
        WillOnce(WithArg<0>(saveRequestAndReturnResponse));

    std::string retrievedValue;
    auto valueSetter = [&retrievedValue](const char* value)
    {
        retrievedValue = value;
    };
    typedef LambdaDispatcher<decltype(valueSetter), void, const char*> ValueSetter;

    // act
    const unsigned int result = GameKitGetUserGameplayDataBundleItem(instance, bundleItem, &valueSetter, ValueSetter::Dispatch);
    GameKitUserGameplayDataInstanceRelease(instance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    ASSERT_STREQ("123", retrievedValue.c_str());

    ASSERT_STREQ("https://domain.tld/usergamedata/bundles/TestBundle/items/k1", actualRequest->GetURIString().c_str());
    ASSERT_EQ(Aws::Http::HttpMethod::HTTP_GET, actualRequest->GetMethod());
    ASSERT_STREQ(TEST_AUTH_HEADER, actualRequest->GetAuthorization().c_str());

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(GameKitUserGameplayDataExportsTestFixture, TestUpdateBundleItem_RequestIsWellFormed_Success)
{
    // arrange
    GameKit::UserGameplayDataBundleItemValue bundleItemValue;
    bundleItemValue.bundleName = "TestBundle";
    bundleItemValue.bundleItemKey = "k123";
    bundleItemValue.bundleItemValue = "v123.1";
    void* instance = CreateDefault();
    const std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    SetMocks(instance, mockHttpClient);

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode::NO_CONTENT);

    std::shared_ptr<Aws::Http::HttpRequest> actualRequest;

    auto saveRequestAndReturnResponse = [&actualRequest, &successResponse](const std::shared_ptr<Aws::Http::HttpRequest>& request)
    {
        actualRequest = request;

        return successResponse;
    };

    EXPECT_CALL(
        *mockHttpClient,
        MakeRequest(_, _, _)).
        WillOnce(WithArg<0>(saveRequestAndReturnResponse));

    // act
    unsigned int result = GameKitUpdateUserGameplayDataBundleItem(instance, bundleItemValue);
    GameKitUserGameplayDataInstanceRelease(instance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);

    ASSERT_STREQ("https://domain.tld/usergamedata/bundles/TestBundle/items/k123", actualRequest->GetURIString().c_str());
    ASSERT_EQ(Aws::Http::HttpMethod::HTTP_PUT, actualRequest->GetMethod());
    ASSERT_STREQ(TEST_AUTH_HEADER, actualRequest->GetAuthorization().c_str());
    ASSERT_STRCASEEQ("application/json", actualRequest->GetContentType().c_str());
    std::stringstream bodyStream;
    bodyStream << actualRequest->GetContentBody()->rdbuf();
    ASSERT_STRCASEEQ("{\"bundle_item_value\":\"v123.1\"}", bodyStream.str().c_str());

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(GameKitUserGameplayDataExportsTestFixture, TestDeleteAll_RequestIsWellFormed_Success)
{
    // arrange
    void* instance = CreateDefault();
    const std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    SetMocks(instance, mockHttpClient);

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode::NO_CONTENT);

    std::shared_ptr<Aws::Http::HttpRequest> actualRequest;

    auto saveRequestAndReturnResponse = [&actualRequest, &successResponse](const std::shared_ptr<Aws::Http::HttpRequest>& request)
    {
        actualRequest = request;

        return successResponse;
    };

    EXPECT_CALL(
        *mockHttpClient,
        MakeRequest(_, _, _)).
        WillOnce(WithArg<0>(saveRequestAndReturnResponse));

    // act
    unsigned int result = GameKitDeleteAllUserGameplayData(instance);
    GameKitUserGameplayDataInstanceRelease(instance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);

    ASSERT_STREQ("https://domain.tld/usergamedata", actualRequest->GetURIString().c_str());
    ASSERT_EQ(Aws::Http::HttpMethod::HTTP_DELETE, actualRequest->GetMethod());
    ASSERT_STREQ(TEST_AUTH_HEADER, actualRequest->GetAuthorization().c_str());

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(GameKitUserGameplayDataExportsTestFixture, TestDeleteBundle_RequestIsWellFormed_Success)
{
    // arrange
    char* bundle = "TestBundle";
    void* instance = CreateDefault();
    const std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    SetMocks(instance, mockHttpClient);

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode::NO_CONTENT);

    std::shared_ptr<Aws::Http::HttpRequest> actualRequest;

    auto saveRequestAndReturnResponse = [&actualRequest, &successResponse](const std::shared_ptr<Aws::Http::HttpRequest>& request)
    {
        actualRequest = request;

        return successResponse;
    };

    EXPECT_CALL(
        *mockHttpClient,
        MakeRequest(_, _, _)).
        WillOnce(WithArg<0>(saveRequestAndReturnResponse));

    // act
    unsigned int result = GameKitDeleteUserGameplayDataBundle(instance, bundle);
    GameKitUserGameplayDataInstanceRelease(instance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    ASSERT_STREQ("https://domain.tld/usergamedata/bundles/TestBundle", actualRequest->GetURIString().c_str());
    ASSERT_EQ(Aws::Http::HttpMethod::HTTP_DELETE, actualRequest->GetMethod());
    ASSERT_STREQ(TEST_AUTH_HEADER, actualRequest->GetAuthorization().c_str());

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(GameKitUserGameplayDataExportsTestFixture, TestDeleteBundleItems_RequestIsWellFormed_Success)
{
    // arrange
    GameKit::UserGameplayDataDeleteItemsRequest bundleItems;
    bundleItems.bundleName = "TestBundle";
    const char* keys[2] = { "k1", "k2" };
    bundleItems.bundleItemKeys = keys;
    bundleItems.numKeys = 2;
    void* instance = CreateDefault();
    const std::shared_ptr<MockHttpClient> mockHttpClient = std::make_shared<MockHttpClient>();
    SetMocks(instance, mockHttpClient);

    std::shared_ptr<Aws::Http::HttpResponse> successResponse = std::make_shared<FakeHttpResponse>();
    successResponse->SetResponseCode(Aws::Http::HttpResponseCode::NO_CONTENT);

    std::shared_ptr<Aws::Http::HttpRequest> actualRequest;

    auto saveRequestAndReturnResponse = [&actualRequest, &successResponse](const std::shared_ptr<Aws::Http::HttpRequest>& request)
    {
        actualRequest = request;

        return successResponse;
    };

    EXPECT_CALL(
        *mockHttpClient,
        MakeRequest(_, _, _)).
        WillOnce(WithArg<0>(saveRequestAndReturnResponse));

    // act
    unsigned int result = GameKitDeleteUserGameplayDataBundleItems(instance, bundleItems);
    GameKitUserGameplayDataInstanceRelease(instance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);

    ASSERT_STREQ("https://domain.tld/usergamedata/bundles/TestBundle", actualRequest->GetURIString(false).c_str());
    ASSERT_EQ(Aws::Http::HttpMethod::HTTP_DELETE, actualRequest->GetMethod());
    ASSERT_STREQ(TEST_AUTH_HEADER, actualRequest->GetAuthorization().c_str());
    ASSERT_FALSE(actualRequest->HasContentType());
    ASSERT_FALSE(actualRequest->HasContentLength());
    
    Aws::Utils::Json::JsonValue payload;
    bundleItems.ToJson(payload);
    Aws::String serialized = payload.View().WriteCompact();
    Aws::String urlEncoded = Aws::Utils::StringUtils::URLEncode(serialized.c_str());
    Aws::Http::QueryStringParameterCollection params = actualRequest->GetQueryStringParameters();
    ASSERT_EQ(1, params.size());
    ASSERT_EQ(1, params.count("payload"));
    ASSERT_STREQ(urlEncoded.c_str(), params.find("payload")->second.c_str());
    
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mockHttpClient.get()));
}

TEST_F(GameKitUserGameplayDataExportsTestFixture, TestValidateItemKeys_ValidKeys_ReturnsTrue)
{
    // arrange
    const char* keys[3] = { "Valid", "Another-Valid", "This.one_too"};

    // act
    std::stringstream buffer;
    const bool valid = ValidateItemKeysProxy(keys, 3, buffer);

    // assert
    ASSERT_TRUE(valid);
    ASSERT_STREQ("", buffer.str().c_str());
}
TEST_F(GameKitUserGameplayDataExportsTestFixture, TestValidateItemKeys_InvalidKeys_ReturnsFalse)
{
    // arrange
    const char* keys[4] = { "Valid", "not valid", "Another-Valid", "~not>valid" };

    // act
    std::stringstream buffer;
    const bool valid = ValidateItemKeysProxy(keys, 4, buffer);

    // assert
    ASSERT_FALSE(valid);
    ASSERT_STREQ("not valid, ~not>valid", buffer.str().c_str());
}