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
    std::shared_ptr<MockHttpClient> mockClient = std::make_shared<MockHttpClient>();

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