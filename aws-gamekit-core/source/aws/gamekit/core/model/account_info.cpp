// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// GameKit
#include <aws/gamekit/core/model/account_info.h>

// Boost
#include <boost/algorithm/string/case_conv.hpp>

std::string GameKit::TruncateAndLower(const std::string& str, const std::regex& pattern)
{
    auto rbegin = std::sregex_iterator(str.begin(), str.end(), pattern);
    auto rend = std::sregex_iterator();
    std::string matchStr;
    for (std::sregex_iterator i = rbegin; i != rend; ++i)
    {
        std::smatch match = *i;
        matchStr = match.str();
    }
    return boost::algorithm::to_lower_copy(matchStr);
}

GameKit::AccountInfoCopy GameKit::CreateAccountInfoCopy(const GameKit::AccountInfo accountInfo)
{
    AccountInfoCopy acctCopy;
    acctCopy.environment = ResourceEnvironment(accountInfo.environment),
    acctCopy.accountId = accountInfo.accountId;
    acctCopy.companyName = accountInfo.companyName;
    acctCopy.gameName = accountInfo.gameName;

    // using the regex pattern for each field, truncate and convert them to lowercase
    acctCopy.accountId = TruncateAndLower(acctCopy.accountId, std::regex("\\d{12}"));
    acctCopy.gameName = TruncateAndLower(acctCopy.gameName, std::regex("[a-zA-Z0-9]{1,12}"));
    acctCopy.companyName = TruncateAndLower(acctCopy.companyName, std::regex("[a-zA-Z0-9]{3,12}"));
    return acctCopy;
}

// Method to compose bootstrap bucket name
std::string GameKit::GetBootstrapBucketName(const GameKit::AccountInfoCopy& accountInfo, const std::string& shortRegionCode)
{
    std::string bootstrapBucketName = "do-not-delete-gamekit-";

    // Bootstrap bucket names have a maximum 63 characters and has the format:
    // do-not-delete-gamekit-<env>-<5_letter_aws_region_code>-<base36_account_id>-<gamename>
    bootstrapBucketName.append(accountInfo.environment.GetEnvironmentString())
        .append("-")
        .append(shortRegionCode)
        .append("-")
        .append(GameKit::Utils::EncodingUtils::DecimalToBase(accountInfo.accountId, GameKit::Utils::BASE_36))
        .append("-")
        .append(accountInfo.gameName);

    return bootstrapBucketName;
}
