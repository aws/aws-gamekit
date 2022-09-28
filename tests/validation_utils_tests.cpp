// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "validation_utils_tests.h"

class GameKit::Tests::ValidationUtils::GameKitUtilsValidationTestFixture : public ::testing::Test
{
public:
    GameKitUtilsValidationTestFixture()
    {
    }

    ~GameKitUtilsValidationTestFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
        TestExecutionUtils::AbortOnFailureIfEnabled();
    }
};

using namespace GameKit::Tests::ValidationUtils;

TEST_F(GameKitUtilsValidationTestFixture, InvalidString_IsValidString_ReturnsFalse)
{
    // act
    auto const result = GameKit::Utils::ValidationUtils::IsValidString("seven", std::regex("[a-zA-Z0-9]{6,10}"));

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitUtilsValidationTestFixture, ValidString_IsValidString_ReturnsTrue)
{
    // act
    auto const result = GameKit::Utils::ValidationUtils::IsValidString("seven12", std::regex("[a-zA-Z0-9]{6,10}"));

    // assert
    ASSERT_TRUE(result);
}

TEST_F(GameKitUtilsValidationTestFixture, PatternMatchSubstring_TruncateString_ReturnsTruncated)
{
    // act
    auto const result = GameKit::Utils::ValidationUtils::TruncateString("123test6789", std::regex("[0-9]{4}"));

    // assert
    ASSERT_EQ(result, "6789");
}

TEST_F(GameKitUtilsValidationTestFixture, PatternNotMatchSubstring_TruncateString_ReturnsEmpty)
{
    // act
    auto const result = GameKit::Utils::ValidationUtils::TruncateString("123test678", std::regex("[0-9]{4}"));

    // assert
    ASSERT_EQ(result, "");
}

TEST_F(GameKitUtilsValidationTestFixture, PatternMatchSubstring_TruncateString_ReturnsTruncatedInSameCase)
{
    // act
    auto const result = GameKit::Utils::ValidationUtils::TruncateString("Foo123Ba5", std::regex("[a-zA-Z]{3}"));

    // assert
    ASSERT_EQ(result, "Foo");
}

TEST_F(GameKitUtilsValidationTestFixture, PatternMatchSubstring_TruncateAndLower_ReturnsTruncatedInLowerCase)
{
    // act
    auto const result = GameKit::Utils::ValidationUtils::TruncateAndLower("Foo123Ba5", std::regex("[a-zA-Z]{3}"));

    // assert
    ASSERT_EQ(result, "foo");
}

TEST_F(GameKitUtilsValidationTestFixture, PatternNotMatchSubstring_TruncateAndLower_ReturnsEmpty)
{
    // act
    auto const result = GameKit::Utils::ValidationUtils::TruncateAndLower("123test678", std::regex("[0-9]{4}"));

    // assert
    ASSERT_EQ(result, "");
}

TEST_F(GameKitUtilsValidationTestFixture, UrlParamWithRestrictedCharacters_UrlEncode_ReturnsEncodedUrlParam)
{
    // act
    auto const result = GameKit::Utils::ValidationUtils::UrlEncode("?troo_l.y~wer*yu//hello");

    // assert
    ASSERT_EQ(result, "%3Ftroo_l.y~wer%2Ayu%2F%2Fhello");
}

TEST_F(GameKitUtilsValidationTestFixture, UrlParamWithoutRestrictedCharacters_UrlEncode_ReturnsSameUrlParam)
{
    // act
    std::string urlParam{ "param_value-21~7.3" };
    auto const result = GameKit::Utils::ValidationUtils::UrlEncode(urlParam);

    // assert
    ASSERT_EQ(result, urlParam);
}

TEST_F(GameKitUtilsValidationTestFixture, UrlParamWithRestrictedChars_IsValidUrlParam_ReturnsFalse)
{
    // act
    std::string urlParam{ "?test" };
    auto const result = GameKit::Utils::ValidationUtils::IsValidUrlParam(urlParam);

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitUtilsValidationTestFixture, UrlParamWithValidSpecialChars_IsValidUrlParam_ReturnsTrue)
{
    // act
    std::string urlParam{ "t_e-s.t~" };
    auto const result = GameKit::Utils::ValidationUtils::IsValidUrlParam(urlParam);

    // assert
    ASSERT_TRUE(result);
}

TEST_F(GameKitUtilsValidationTestFixture, S3KeyWithInvalidSpecialChars_IsValidS3KeyParam_ReturnsFalse)
{
    // act
    std::string s3KeyParam{ "+keyName1" };
    auto const result = GameKit::Utils::ValidationUtils::IsValidS3KeyParam(s3KeyParam);

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitUtilsValidationTestFixture, S3KeyWithNonAsciiChars_IsValidS3KeyParam_ReturnsFalse)
{
    // act
    std::string s3KeyParam{ "keyName1¥" };
    auto const result = GameKit::Utils::ValidationUtils::IsValidS3KeyParam(s3KeyParam);

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitUtilsValidationTestFixture, S3KeyWithValidSpecialChars_IsValidS3KeyParam_ReturnsTrue)
{
    // act
    std::string s3KeyParam{ "keyName1-_'().*'-" };
    auto const result = GameKit::Utils::ValidationUtils::IsValidS3KeyParam(s3KeyParam);

    // assert
    ASSERT_TRUE(result);
}

TEST_F(GameKitUtilsValidationTestFixture, StringWithAsciiChars_IsValidPrimaryIdentifier_ReturnsTrue)
{
    // act
    const std::string identifier{ "some-identifier._1" };
    auto const result = GameKit::Utils::ValidationUtils::IsValidPrimaryIdentifier(identifier);

    // assert
    ASSERT_TRUE(result);
}

TEST_F(GameKitUtilsValidationTestFixture, StringWithNonAsciiChars_IsValidPrimaryIdentifier_ReturnsFalse)
{
    // act
    const std::string identifier{ "$0me>.!dentifier_#\\/+=~`?" };
    auto const result = GameKit::Utils::ValidationUtils::IsValidPrimaryIdentifier(identifier);

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitUtilsValidationTestFixture, StringWithMultipleMatches_IsValidPrimaryIdentifier_ReturnsFalse)
{
    // act
    const std::string identifier{ "some-identifier._1 some-identifier._2" };
    auto const result = GameKit::Utils::ValidationUtils::IsValidPrimaryIdentifier(identifier);

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitUtilsValidationTestFixture, StringWithTrailingInvalidChars_IsValidPrimaryIdentifier_ReturnsFalse)
{
    // act
    const std::string identifier{ "some-identifier._1_!@#$%^&*()" };
    auto const result = GameKit::Utils::ValidationUtils::IsValidPrimaryIdentifier(identifier);

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitUtilsValidationTestFixture, EmptyString_IsValidPrimaryIdentifier_ReturnsFalse)
{
    // act
    const std::string identifier;
    auto const result = GameKit::Utils::ValidationUtils::IsValidPrimaryIdentifier(identifier);

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitUtilsValidationTestFixture, StringWith513Chars_IsValidPrimaryIdentifier_ReturnsFalse)
{
    // act
    const std::string identifier(513, 'a');
    auto const result = GameKit::Utils::ValidationUtils::IsValidPrimaryIdentifier(identifier);

    // assert
    ASSERT_FALSE(result);
}

