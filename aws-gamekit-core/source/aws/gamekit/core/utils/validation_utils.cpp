// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/core/utils/validation_utils.h>

using namespace GameKit::Utils;

#pragma region Public Methods
std::string ValidationUtils::UrlEncode(const std::string& urlParameter) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = urlParameter.begin(), n = urlParameter.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum((unsigned char) c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char)c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}

std::string ValidationUtils::TruncateString(const std::string& str, const std::regex& pattern)
{
    auto rbegin = std::sregex_iterator(str.begin(), str.end(), pattern);
    auto rend = std::sregex_iterator();
    std::string matchStr;
    for (std::sregex_iterator i = rbegin; i != rend; ++i)
    {
        std::smatch match = *i;
        matchStr = match.str();
    }

    return matchStr;
}

std::string ValidationUtils::TruncateAndLower(const std::string& str, const std::regex& pattern)
{
    return boost::algorithm::to_lower_copy(ValidationUtils::TruncateString(str, pattern));
}

bool ValidationUtils::IsValidString(const std::string& str, const std::regex& pattern)
{
    return std::regex_match(str, pattern);
}

bool ValidationUtils::IsValidUrlParam(const std::string& urlParam)
{
    if (urlParam.length() < MIN_URL_PARAM_CHARS || urlParam.length() > MAX_URL_PARAM_CHARS)
    {
        return false;
    }
    return ValidationUtils::IsValidString(urlParam, std::regex("[a-zA-Z0-9-_.~]+"));
}

bool ValidationUtils::IsValidS3KeyParam(const std::string& s3KeyParam)
{
    if (s3KeyParam.length() < MIN_S3_PARAM_CHARS || s3KeyParam.length() > MAX_S3_PARAM_CHARS)
    {
        return false;
    }
    return ValidationUtils::IsValidString(s3KeyParam, std::regex("[a-zA-Z0-9-_.*'()]+"));
}

bool ValidationUtils::IsValidPrimaryIdentifier(const std::string& identifier)
{
    if (identifier.length() < MIN_PRIMARY_IDENTIFIER_CHARS || identifier.length() > MAX_PRIMARY_IDENTIFIER_CHARS)
    {
        return false;
    }

    return ValidationUtils::IsValidString(identifier, std::regex(PRIMARY_IDENTIFIER_REGEX));
}

#pragma endregion
