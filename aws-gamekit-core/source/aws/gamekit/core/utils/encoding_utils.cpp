// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// GameKit
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/core/utils/encoding_utils.h>

// AWS SDK
#include <aws/core/utils/base64/Base64.h>

using namespace GameKit::Utils;

#pragma region Public Methods
std::string EncodingUtils::DecimalToBase(const std::string& decimalStr, const int base)
{
    // Base less than 2 or negative input will not work
    if (base < 2 || decimalStr.front() == '-')
    {
        return "";
    }

    // Try...Catch in case the string to ULL conversion fails or any other conversion errors
    try
    {
        // Initialize index of result
        int index = 0;

        // The longest possible string is a 64-bit binary string, there can't be more than 64 output characters plus the terminating null
        char res[65];

        // Convert input number is given base by repeatedly
        // dividing it by base and taking remainder
        unsigned long long inputNum = std::stoull(decimalStr);
        while (inputNum > 0)
        {
            res[index++] = "0123456789abcdefghijklmnopqrstuvwxyz"[inputNum % base];
            inputNum /= base;
        }
        res[index] = '\0';

        // Reverse the result
        std::string resStr(res);
        reverse(resStr.begin(), resStr.end());

        return resStr;
    }
    catch (...)
    {
        return "";
    }
}

std::string GameKit::Utils::EncodingUtils::EncodeBase64(const std::string& str)
{
    const Aws::Utils::Base64::Base64 base64;
    const Aws::Utils::ByteBuffer metadataBuffer((unsigned char*)str.c_str(), str.length());
    return ToStdString(base64.Encode(metadataBuffer));
}

std::string GameKit::Utils::EncodingUtils::DecodeBase64(const std::string& encodedStr)
{
    const Aws::Utils::Base64::Base64 base64;
    const Aws::Utils::ByteBuffer metadataBuffer = base64.Decode(ToAwsString(encodedStr));
    return std::string((char*)metadataBuffer.GetUnderlyingData(), metadataBuffer.GetLength());
}

#pragma endregion
