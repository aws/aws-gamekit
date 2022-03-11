// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/identity/utils/credentials_utils.h>

using namespace GameKit::Utils;

#pragma region Public Methods
bool CredentialsUtils::IsValidUsername(const std::string& username)
{
    // max length defined by cognito
    return (username.length() >= MIN_USERNAME_CHARS && username.length() < MAX_USERNAME_CHARS);
}

bool CredentialsUtils::IsValidPassword(const std::string& password)
{
    // Password regex is based on min requirements of Cognito + GameKit user pool settings
    if (password.length() < MIN_PASSWORD_CHARS || password.length() > MAX_PASSWORD_CHARS)
    {
        return false;
    }
    std::regex passwordRegex{ PASSWORD_REGEX };
    return ValidationUtils::IsValidString(password, passwordRegex);
}
#pragma endregion
