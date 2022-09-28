// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// GameKit
#include "test_stack.h"
#include <aws/gamekit/core/awsclients/api_initializer.h>

// AWS C++ SDK
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/utils/crypto/Factories.h>

void TestStackInitializer::Initialize()
{
    // Make the default mock client return 418 MakeRequest(), without this the response would be an
    // invalid object, causing tests to crash.
    // With this change, tests that don't set an explicit mock would fail.
    // Tests using http client should create their own mock and add expected requests and responses.
    std::shared_ptr<Aws::Http::HttpClient> fakeClient = std::make_shared<SameResponseClient>();

    // We don't need to verify that the expectation when the object is destructed, it is only
    // meant to prevent outgoing requests.
    Initialize(fakeClient);
}

void TestStackInitializer::Initialize(std::shared_ptr<Aws::Http::HttpClient> mock)
{
    // Verify test is running on a clean state. If the AwsApiInitializer is already in an
    // Initialized state verify that previous tests released all their GameKit handles
    EXPECT_FALSE(GameKit::AwsApiInitializer::IsInitialized())
        << "GameKit::AwsApiInitializer must not be in Initialized state before a test starts.";

    testMockFactory = std::make_shared<MockHttpClientFactory>();
    testFakeClient = mock;
    testMockFactory->SetClient(testFakeClient);

    Aws::Http::SetHttpClientFactory(testMockFactory);
    Aws::Http::InitHttp();
    Aws::Utils::Crypto::InitCrypto();
}

void TestStackInitializer::Cleanup()
{
    Aws::Http::CleanupHttp();
    Aws::Utils::Crypto::CleanupCrypto();
    testFakeClient.reset();
    testMockFactory.reset();

    if (!TestExecutionSettings::Settings.InitialFileCount.empty())
    {
        std::map<std::string, std::ptrdiff_t> fileCountPerDir = TestFileSystemUtils::CountFilesInDirectories(TestExecutionSettings::Settings.DirectoriesToWatch);

        for (const std::string& dir : TestExecutionSettings::Settings.DirectoriesToWatch)
        {
            EXPECT_EQ(TestExecutionSettings::Settings.InitialFileCount[dir], fileCountPerDir[dir]) <<
                "Test directory " << dir << " contains test artifacts! Make sure that all tests revert their changes to the filesystem.";
        }
    }
}

std::shared_ptr<MockHttpClientFactory> TestStackInitializer::GetMockHttpClientFactory() const
{
    return testMockFactory;
}