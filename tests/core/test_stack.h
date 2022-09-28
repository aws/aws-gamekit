// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// GameKit
#include "mocks/fake_http_client.h"
#include "custom_test_flags.h"

// GTest
#include <gtest/gtest.h>

/*
Fake HTTP client that always responds 418
*/
class SameResponseClient : public FakeHttpClient
{
public:
    std::shared_ptr<Aws::Http::HttpResponse> MakeRequest(const std::shared_ptr<Aws::Http::HttpRequest>& request,
        Aws::Utils::RateLimits::RateLimiterInterface* readLimiter = nullptr,
        Aws::Utils::RateLimits::RateLimiterInterface* writeLimiter = nullptr)
        const override
    {
        std::shared_ptr<Aws::Http::HttpResponse> dummyResponse = std::make_shared<FakeHttpResponse>();
        dummyResponse->SetResponseCode(Aws::Http::HttpResponseCode::IM_A_TEAPOT);

        return dummyResponse;
    }
};

/*
Helper class to initialize the AWS Stack. Initializes the HTTP stack with Mock HTTP Clients and the Crypto stack.
*/
class TestStackInitializer
{
private:
    std::shared_ptr<MockHttpClientFactory> testMockFactory;
    std::shared_ptr<Aws::Http::HttpClient> testFakeClient;

public:

    // Initialize the HTTP stack with a mock http client (retrieve it with GetMockHttpClientFactory()) and the Crypto stack.
    void Initialize();

    // Initialize the HTTP stack with the given mock http client (retrieve it with GetMockHttpClientFactory()) and the Crypto stack.
    // Use this method if you want to reuse the same client for ALL AWS calls.
    void Initialize(std::shared_ptr<Aws::Http::HttpClient> mockHttpClient);

    // Reset the HTTP and Crypto stacks
    void Cleanup();

    // Reset the HTTP and Crypto stacks and write test log in case of test failures.
    template <typename TestLogger_t>
    void CleanupAndLog()
    {
        Cleanup();

        // Verify test is ends on a clean state. If the AwsApiInitializer is still in an
        // Initialized state verify that the test released all its GameKit handles
        EXPECT_FALSE(GameKit::AwsApiInitializer::IsInitialized())
            << "GameKit::AwsApiInitializer must not be in an Initialized state after a test ends.";

        TestLogger_t::DumpToConsoleIfTestFailed();
        TestLogger_t::Clear();
    }

    // Get the Mock factory.
    std::shared_ptr<MockHttpClientFactory> GetMockHttpClientFactory() const;
};