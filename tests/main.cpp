// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// Gtest
#include <gtest/gtest.h>

// Test
#include "core/custom_test_flags.h"

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

    // InitGoogleTest removes Google test arguments, now parse our custom arguments
    // arg[0] is the executable name. Supported arguments:
    // --filesystem_check: Count number of files inside a set of directories before and after tests run. 
    //                     Use this to verify that temporary test artifacts are removed.
    // --abort_on_failure: If a test fails, execute its TearDown() logic and then abort the process. 
    //                    This is different from --gtest_break_on_failure because the latter will stop the test
    //                    as soon as it fails and won't execute TearDown(). We want TearDown() because GameKit tests
    //                    write logs on failure and we need as much diagnostic information as possible.

    if (argc > 1)
    {
        TestExecutionSettings::CustomTestExecutionSettings customSettings;
        
        for (size_t i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--filesystem_check")
            {
                std::cout << "Recognized argument: " << arg << std::endl;
                customSettings.DirectoriesToWatch.push_back("../core/test_data");
                customSettings.InitialFileCount = TestFileSystemUtils::CountFilesInDirectories(customSettings.DirectoriesToWatch);
            }
            else if (arg == "--abort_on_failure")
            {
                std::cout << "Recognized argument: " << arg << std::endl;
                customSettings.AbortOnFailure = true;
            }
            else
            {
                std::cout << "Argument not recognized: " << arg << std::endl;
            }
        }

        TestExecutionSettings::Settings = customSettings;
    }

    return RUN_ALL_TESTS();
}
