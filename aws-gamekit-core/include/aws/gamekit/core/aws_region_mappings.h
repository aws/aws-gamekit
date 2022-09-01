// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Lib
#include <string>
#include <unordered_map>

// GameKit
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/logging.h>

// yaml-cpp
#include <yaml-cpp/yaml.h>

using namespace GameKit::Logger;

namespace GameKit
{
    static const std::string GAMEKIT_AWS_REGION_MAPPINGS_FILE_NAME = "awsGameKitAwsRegionMappings.yml";
    static const std::string GAMEKIT_FIVE_LETTER_REGION_CODES_PREFIX = "five_letter_region_codes";

    class GAMEKIT_API AwsRegionMappings
    {
    private:
        std::string m_baseTemplatesFolder;
        YAML::Node m_regionShortCodes;
        FuncLogCallback m_logCb;

        // Make the constructors and destructor private to prevent duplicate instances
        AwsRegionMappings() {};
        AwsRegionMappings(const std::string& baseTemplatesFolder, FuncLogCallback logCallback);
        ~AwsRegionMappings();
        
        std::string GetRegionMappingsFilePath() const;

    public:
        // Delete constructors and operator overloading that should not be used
        AwsRegionMappings(const AwsRegionMappings&) = delete;
        const AwsRegionMappings& operator=(const AwsRegionMappings&) = delete;

        static AwsRegionMappings& getInstance(const std::string& baseTemplatesFolder, FuncLogCallback logCallback);

        /**
        * @brief Get the 5-letter region code for AWS region.
        *
        * @details Converts an input AWS region code to it's 5-letter short code based on mapping in GAMEKIT_AWS_REGION_MAPPINGS_FILE_NAME in gamekit root directory.
        *
        * @param fullRegionCode Input aws region code.
        * @returns 5-letter short region code based on mapping in GAMEKIT_AWS_REGION_MAPPINGS_FILE_NAME in plugin, empty string if error or doesn't exist in map.
        */
        std::string getFiveLetterRegionCode(const std::string& fullRegionCode) const;
    };
}