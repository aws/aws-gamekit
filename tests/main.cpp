// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

// Main test entrypoint. Must be defined when gtest and gmock are linked.
int main(int argc, char** argv)
{
    // Disable EC2 metadata requests
    putenv("AWS_EC2_METADATA_DISABLED=true");
#ifdef _WIN32
    putenv("AWS_SHARED_CREDENTIALS_FILE=..\\core\\test_data\\testFiles\\credentialsTests\\test_credentials");
#else
    putenv("AWS_SHARED_CREDENTIALS_FILE=../core/test_data/testFiles/credentialsTests/test_credentials");
#endif

    // Parse command line and run all the tests
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
