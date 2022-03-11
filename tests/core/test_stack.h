// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// GameKit
#include "mocks/fake_http_client.h"

// GTest
#include <gtest/gtest.h>

/*
Helper class to initialize the AWS Stack. Initializes the HTTP stack with Mock HTTP Clients and the Crypto stack.
*/
class TestStackInitializer
{
private:
    std::shared_ptr<MockHttpClientFactory> mockFactory;

public:

    // Initialize the HTTP stack with a mock http client (retrieve it with GetMockHttpClientFactory()) and the Crypto stack.
    void Initialize();

    // Initialize the HTTP stack with the given mock http client (retrieve it with GetMockHttpClientFactory()) and the Crypto stack.
    // Use this method if you want to reuse the same client for ALL AWS calls.
    void Initialize(std::shared_ptr<MockHttpClient> mockHttpClient);

    // Reset the HTTP and Crypto stacks
    void Cleanup();

    // Get the Mock factory.
    std::shared_ptr<MockHttpClientFactory> GetMockHttpClientFactory() const;
};