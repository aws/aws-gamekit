// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "aws_region_mappings_test.h"
#include "test_log.h"
#include "custom_test_flags.h"

class GameKit::Tests::GameKitAwsRegionMappingsTestFixture : public ::testing::Test
{
protected:
    typedef TestLog<GameKitAwsRegionMappingsTestFixture> TestLogger;

public:
    GameKitAwsRegionMappingsTestFixture()
    {
    }

    ~GameKitAwsRegionMappingsTestFixture()
    {
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
        TestLogger::DumpToConsoleIfTestFailed();
        TestLogger::Clear();
        TestExecutionUtils::AbortOnFailureIfEnabled();
    }
};

using namespace GameKit::Tests;

TEST_F(GameKitAwsRegionMappingsTestFixture, TestGetFiveLetterRegionCode_ValidRegion_ValidShortCode)
{
    // act
    GameKit::AwsRegionMappings& instance = GameKit::AwsRegionMappings::getInstance("../core/test_data/testFiles/regionMappingsTests", TestLogger::Log);
    std::string fiveLetterCode = instance.getFiveLetterRegionCode(std::string("us-east-1"));

    // assert
    ASSERT_EQ("usea1", fiveLetterCode);
}

TEST_F(GameKitAwsRegionMappingsTestFixture, TestGetFiveLetterRegionCode_InvalidRegion_EmptyShortCode)
{
    // act
    GameKit::AwsRegionMappings& instance = GameKit::AwsRegionMappings::getInstance("../core/test_data/testFiles/regionMappingsTests", TestLogger::Log);
    std::string fiveLetterCode = instance.getFiveLetterRegionCode(std::string("wrong-region-1"));

    // assert
    ASSERT_EQ("", fiveLetterCode);
}

TEST_F(GameKitAwsRegionMappingsTestFixture, TestGetFiveLetterRegionCode_EmptyRegion_EmptyShortCode)
{
    // act
    GameKit::AwsRegionMappings& instance = GameKit::AwsRegionMappings::getInstance("../core/test_data/testFiles/regionMappingsTests", TestLogger::Log);
    std::string fiveLetterCode = instance.getFiveLetterRegionCode(std::string(""));

    // assert
    ASSERT_EQ("", fiveLetterCode);
}

TEST_F(GameKitAwsRegionMappingsTestFixture, TestGetFiveLetterRegionCode_UnitializedRegionString_EmptyShortCode)
{
    // act
    GameKit::AwsRegionMappings& instance = GameKit::AwsRegionMappings::getInstance("../core/test_data/testFiles/regionMappingsTests", TestLogger::Log);
    std::string region;
    std::string fiveLetterCode = instance.getFiveLetterRegionCode(region);

    // assert
    ASSERT_EQ("", fiveLetterCode);
}