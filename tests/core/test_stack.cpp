// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// GameKit
#include "test_stack.h"

// AWS C++ SDK
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/utils/crypto/Factories.h>

void TestStackInitializer::Initialize()
{
    using namespace ::testing;

    std::shared_ptr<MockHttpClient> mockClient = std::make_shared<MockHttpClient>();

    // Make the default mock client return 418 MakeRequest(), without this the response would be an
    // invalid object, causing tests to crash.
    // With this change, tests that don't set an explicit mock would fail.
    // Tests using http client should create their own mock and add expected requests and responses.
    std::shared_ptr<Aws::Http::HttpResponse> dummyResponse = std::make_shared<FakeHttpResponse>();
    dummyResponse->SetResponseCode(Aws::Http::HttpResponseCode::IM_A_TEAPOT);
    ON_CALL(*mockClient, MakeRequest(_, _, _)).WillByDefault(Return(dummyResponse));

    Initialize(mockClient);
}

void TestStackInitializer::Initialize(std::shared_ptr<MockHttpClient> mockClient)
{
    mockFactory = std::make_shared<MockHttpClientFactory>();
    mockFactory->SetClient(mockClient);

    Aws::Http::SetHttpClientFactory(mockFactory);
    Aws::Http::InitHttp();
    Aws::Utils::Crypto::InitCrypto();
}

void TestStackInitializer::Cleanup()
{
    Aws::Http::CleanupHttp();
    Aws::Utils::Crypto::CleanupCrypto();
    mockFactory.reset();
}

std::shared_ptr<MockHttpClientFactory> TestStackInitializer::GetMockHttpClientFactory() const
{
    return mockFactory;
}