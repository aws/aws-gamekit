// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "encoding_utils_tests.h"

class GameKit::Tests::EncodingUtils::GameKitUtilsEncodingTestFixture : public ::testing::Test
{
public:
    GameKitUtilsEncodingTestFixture()
    {
    }

    ~GameKitUtilsEncodingTestFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

using namespace GameKit::Tests::EncodingUtils;

TEST_F(GameKitUtilsEncodingTestFixture, DecimalToBase_ValidDecimalToBase36_ReturnsValidBase36)
{
    // arrange
    std::string inputStr = "097036240017";

    // act
    const std::string result = GameKit::Utils::EncodingUtils::DecimalToBase(inputStr, GameKit::Utils::BASE_36);

    // assert
    ASSERT_STRCASEEQ("18ksvdzl", result.c_str());
}

TEST_F(GameKitUtilsEncodingTestFixture, DecimalToBase_ValidDecimalToBase16_ReturnsValidBase16)
{
    // arrange
    std::string inputStr = "097036240017";

    // act
    const std::string result = GameKit::Utils::EncodingUtils::DecimalToBase(inputStr, GameKit::Utils::BASE_16);

    // assert
    ASSERT_STRCASEEQ("1697cf8491", result.c_str());
}

TEST_F(GameKitUtilsEncodingTestFixture, DecimalToBase_ValidDecimalToBase2_ReturnsValidBase2)
{
    // arrange
    std::string inputStr = "097036240017";

    // act
    const std::string result = GameKit::Utils::EncodingUtils::DecimalToBase(inputStr, GameKit::Utils::BASE_2);

    // assert
    ASSERT_STRCASEEQ("1011010010111110011111000010010010001", result.c_str());
}

TEST_F(GameKitUtilsEncodingTestFixture, DecimalToBase_ValidDecimalToBase0_ReturnsEmptyString)
{
    // arrange
    std::string inputStr = "097036240017";

    // act
    const std::string result = GameKit::Utils::EncodingUtils::DecimalToBase(inputStr,0);

    // assert
    ASSERT_STRCASEEQ("", result.c_str());
}

TEST_F(GameKitUtilsEncodingTestFixture, DecimalToBase_ValidDecimalToBase1_ReturnsEmptyString)
{
    // arrange
    std::string inputStr = "097036240017";

    // act
    const std::string result = GameKit::Utils::EncodingUtils::DecimalToBase(inputStr, 1);

    // assert
    ASSERT_STRCASEEQ("", result.c_str());
}

TEST_F(GameKitUtilsEncodingTestFixture, DecimalToBase_InvalidDecimalInputToBase36_ReturnsEmptyString)
{
    // arrange
    std::string inputStr = "ASKJHkjhsd6^&";

    // act
    const std::string result = GameKit::Utils::EncodingUtils::DecimalToBase(inputStr, GameKit::Utils::BASE_36);

    // assert
    ASSERT_STRCASEEQ("", result.c_str());
}

TEST_F(GameKitUtilsEncodingTestFixture, DecimalToBase_NegativeDecimalInputToBase36_ReturnsEmptyString)
{
    // arrange
    std::string inputStr = "-097036240017";

    // act
    const std::string result = GameKit::Utils::EncodingUtils::DecimalToBase(inputStr, GameKit::Utils::BASE_36);

    // assert
    ASSERT_STRCASEEQ("", result.c_str());
}