// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// Standard Library
#include <fstream>

// AWS SDK
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/StringUtils.h>

// GameKit
#include <aws/gamekit/core/internal/platform_string.h>
#include "../core/test_common.h"
#include "request_serialization_test.h"

using namespace Aws::Utils;
using namespace GameKit::Tests::Utils;
using namespace GameKit::Utils::HttpClient;

#define SERIALIZATION_BIN_FILE  "./request_serialization_test.dat"

GameKitRequestSerializationTestFixture::GameKitRequestSerializationTestFixture()
{}

GameKitRequestSerializationTestFixture::~GameKitRequestSerializationTestFixture()
{}

void GameKitRequestSerializationTestFixture::SetUp()
{
    TestLogger::Clear();
    testStack.Initialize();
}

void GameKitRequestSerializationTestFixture::TearDown()
{
    testStack.Cleanup();

    remove(SERIALIZATION_BIN_FILE);
}

TEST_F(GameKitRequestSerializationTestFixture, HttpRequest_BinarySerializeDeserialize_RequestsMatch)
{
    // Arrange
    using namespace Aws::Http;

    auto request = std::make_shared<FakeHttpRequest>(
        Aws::Http::URI("https://123.aws.com/foo"), Aws::Http::HttpMethod::HTTP_POST);

    request->SetHeaderValue("authorization", "FooAuth123");
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

    // Act
    std::ofstream os(SERIALIZATION_BIN_FILE, std::ios::binary);
    bool serializeResult = TrySerializeRequestBinary(os, request, TestLogger::Log);
    os.close();

    std::shared_ptr<HttpRequest> deserialized;
    std::ifstream is(SERIALIZATION_BIN_FILE, std::ios::binary);
    bool deserializeResult = TryDeserializeRequestBinary(is, deserialized, TestLogger::Log);
    is.close();

    // Assert
    ASSERT_TRUE(serializeResult);
    ASSERT_TRUE(deserializeResult);

    ASSERT_STREQ(request->GetUri().GetURIString().c_str(), deserialized->GetUri().GetURIString().c_str());
    ASSERT_EQ(request->GetMethod(), deserialized->GetMethod());
    ASSERT_EQ(request->GetQueryStringParameters().size(), deserialized->GetQueryStringParameters().size());
    ASSERT_EQ(request->GetHeaders().size(), deserialized->GetHeaders().size());
    ASSERT_STREQ(deserialized->GetHeaders()["authorization"].c_str(), "~");
    ASSERT_STREQ(request->GetContentType().c_str(), deserialized->GetContentType().c_str());
    ASSERT_STREQ(request->GetContentLength().c_str(), deserialized->GetContentLength().c_str());
    auto requestBodyStream = std::ostringstream();
    requestBodyStream << request->GetContentBody()->rdbuf();
    auto deserializedBodyStream = std::ostringstream();
    deserializedBodyStream << deserialized->GetContentBody()->rdbuf();
    ASSERT_STREQ(requestBodyStream.str().c_str(), deserializedBodyStream.str().c_str());
}

TEST_F(GameKitRequestSerializationTestFixture, HttpRequest_BinarySerializeDeserialize_InvalidLength_Fail)
{
    // Arrange
    using namespace Aws::Http;

    auto request = std::make_shared<FakeHttpRequest>(
        Aws::Http::URI("https://123.aws.com/foo"), Aws::Http::HttpMethod::HTTP_POST);

    request->SetHeaderValue("authorization", "FooAuth123");
    request->AddQueryStringParameter("foo", "bar");

    std::shared_ptr<Aws::IOStream> payloadStream = Aws::MakeShared<Aws::StringStream>("AddUserGameplayDataBody");
    std::string serialized = "{\"a\":\"1\"}";
    *payloadStream << serialized;

    request->AddContentBody(payloadStream);
    request->SetContentType("application/json");

    // set an invalid content length
    request->SetContentLength(StringUtils::to_string(serialized.size() + 1));

    // Act
    std::ofstream os(SERIALIZATION_BIN_FILE, std::ios::binary);
    bool serializeResult = TrySerializeRequestBinary(os, request, TestLogger::Log);
    os.close();

    std::shared_ptr<HttpRequest> deserialized;
    std::ifstream is(SERIALIZATION_BIN_FILE, std::ios::binary);
    bool deserializeResult = TryDeserializeRequestBinary(is, deserialized, TestLogger::Log);
    is.close();

    // Assert
    ASSERT_TRUE(serializeResult);
    ASSERT_FALSE(deserializeResult);
}

TEST_F(GameKitRequestSerializationTestFixture, HttpRequest_BinarySerializeDeserialize_InvalidCRC_Fail)
{
    // Arrange
    using namespace Aws::Http;

    auto request = std::make_shared<FakeHttpRequest>(
        Aws::Http::URI("http://a"), Aws::Http::HttpMethod::HTTP_POST);

    std::shared_ptr<Aws::IOStream> payloadStream = Aws::MakeShared<Aws::StringStream>("AddUserGameplayDataBody");
    std::string serialized = "{\"a\":\"1\"}";
    *payloadStream << serialized;

    request->AddContentBody(payloadStream);
    request->SetContentType("application/json");
    request->SetContentLength(StringUtils::to_string(serialized.size()));

    // Act
    std::ofstream os(SERIALIZATION_BIN_FILE, std::ios::binary);
    bool serializeResult = TrySerializeRequestBinary(os, request, TestLogger::Log);

    // tamper with the serialized stream by setting the value of 'a' in the json to '2'
    size_t length = os.tellp();
    size_t offset = sizeof(unsigned int);
    os.seekp(length - offset - 3);
    os.write("2", 1); // json is now {"a":"2"}
    os.seekp(length);
    os.close();

    std::ifstream is(SERIALIZATION_BIN_FILE, std::ios::binary);
    std::shared_ptr<HttpRequest> deserialized;
    bool deserializeResult = TryDeserializeRequestBinary(is, deserialized, TestLogger::Log);
    is.close();

    // Assert
    ASSERT_TRUE(serializeResult);
    ASSERT_FALSE(deserializeResult);
}
TEST_F(GameKitRequestSerializationTestFixture, HttpRequest_BinarySerializeDeserialize_InvalidJson_Fail)
{
    // Arrange
    using namespace Aws::Http;

    auto request = std::make_shared<FakeHttpRequest>(
        Aws::Http::URI("https://123.aws.com/foo"), Aws::Http::HttpMethod::HTTP_POST);

    request->SetHeaderValue("authorization", "FooAuth123");
    request->AddQueryStringParameter("foo", "bar");

    std::shared_ptr<Aws::IOStream> payloadStream = Aws::MakeShared<Aws::StringStream>("AddUserGameplayDataBody");
    // set body to invalid json
    std::string serialized = "{'this': 'is invalid', { json ]}}";
    *payloadStream << serialized;

    request->AddContentBody(payloadStream);
    request->SetContentType("application/json");
    request->SetContentLength(StringUtils::to_string(serialized.size()));

    // Act
    std::ofstream os(SERIALIZATION_BIN_FILE, std::ios::binary);
    bool serializeResult = TrySerializeRequestBinary(os, request, TestLogger::Log);
    os.close();

    std::shared_ptr<HttpRequest> deserialized;
    std::ifstream is(SERIALIZATION_BIN_FILE, std::ios::binary);
    bool deserializeResult = TryDeserializeRequestBinary(is, deserialized, TestLogger::Log);
    is.close();

    // Assert
    ASSERT_TRUE(serializeResult);
    ASSERT_FALSE(deserializeResult);
}

