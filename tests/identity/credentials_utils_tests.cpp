// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "credentials_utils_tests.h"

class GameKit::Tests::CredentialsUtils::GameKitUtilsCredentialsTestFixture : public ::testing::Test
{
public:
    GameKitUtilsCredentialsTestFixture()
    {
    }

    ~GameKitUtilsCredentialsTestFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }
};

using namespace GameKit::Tests::CredentialsUtils;

TEST_F(GameKitUtilsCredentialsTestFixture, UsernameTooShort_IsValidUsername_ReturnsFalse)
{
    // act
    auto const result = GameKit::Utils::CredentialsUtils::IsValidUsername("u");

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitUtilsCredentialsTestFixture, ValidUsername_IsValidUsername_ReturnsTrue)
{
    // act
    auto const result = GameKit::Utils::CredentialsUtils::IsValidUsername("userName1");

    // assert
    ASSERT_TRUE(result);
}

TEST_F(GameKitUtilsCredentialsTestFixture, PasswordTooShort_IsValidPassword_ReturnsFalse)
{
    // act
    auto const result = GameKit::Utils::CredentialsUtils::IsValidPassword("passwrd");

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitUtilsCredentialsTestFixture, PasswordTooLong_IsValidPassword_ReturnsFalse)
{
    // act
    // test password is 99 characters, max allowed 98
    auto const result = GameKit::Utils::CredentialsUtils::IsValidPassword("paswdpaswdpaswdpaswdpaswdpaswdpaswdpaswdpaswdpaswdpaswdpaswdpaswdpaswdpaswdpaswdpaswdpaswdpaswdpasw");

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitUtilsCredentialsTestFixture, PasswordInvalidSpecialCharacters_IsValidPassword_ReturnsFalse)
{
    // act
    auto const result = GameKit::Utils::CredentialsUtils::IsValidPassword("-password+");

    // assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitUtilsCredentialsTestFixture, PasswordValidLength_IsValidPassword_ReturnsTrue)
{
    // act
    auto const result = GameKit::Utils::CredentialsUtils::IsValidPassword("password");

    // assert
    ASSERT_TRUE(result);
}

TEST_F(GameKitUtilsCredentialsTestFixture, PasswordValidSpecialCharacters_IsValidPassword_ReturnsTrue)
{
    // act
    auto const result = GameKit::Utils::CredentialsUtils::IsValidPassword("password^$*.[:;|_]{}()?\"!@#%&/\\,><':;|_~`");

    // assert
    ASSERT_TRUE(result);
}

TEST_F(GameKitUtilsCredentialsTestFixture, PasswordNonAsciiCharacters_IsValidPassword_ReturnsFalse)
{
    // act
    auto const result = GameKit::Utils::CredentialsUtils::IsValidPassword("password¥");

    // assert
    ASSERT_FALSE(result);
}