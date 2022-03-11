// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/core/aws_region_mappings.h>
#include <aws/gamekit/core/utils/file_utils.h>

using namespace GameKit;

AwsRegionMappings::AwsRegionMappings(const std::string& pluginRootPath, FuncLogCallback logCallback) : m_pluginRootPath(pluginRootPath), m_logCb(logCallback)
{
    Logging::Log(m_logCb, Level::Info, "AwsRegionMappings instantiated");
    
    Utils::FileUtils::ReadFileAsYAML(GetRegionMappingsFilePath(), m_regionShortCodes, m_logCb, "AwsRegionMappings: ");
}

AwsRegionMappings::~AwsRegionMappings()
{
    Logging::Log(m_logCb, Level::Info, "AwsRegionMappings instance deleted");
}

std::string GameKit::AwsRegionMappings::GetRegionMappingsFilePath() const
{
    return m_pluginRootPath + "/misc/" + GAMEKIT_AWS_REGION_MAPPINGS_FILE_NAME;
}

#pragma region Public Methods
AwsRegionMappings& AwsRegionMappings::getInstance(const std::string& pluginRootPath, FuncLogCallback logCallback)
{
    static AwsRegionMappings instance(pluginRootPath, logCallback);
    return instance;
}

std::string GameKit::AwsRegionMappings::getFiveLetterRegionCode(const std::string& fullRegionCode) const
{
    try
    {
        std::string fiveLetterCode = m_regionShortCodes[GAMEKIT_FIVE_LETTER_REGION_CODES_PREFIX][fullRegionCode].Scalar();
        return fiveLetterCode;
    }
    catch (const YAML::InvalidNode&)
    {
        std::string message = "AwsRegionMappings::getFiveLetterRegionCode() Could not find a 5 letter region code for: "
            + fullRegionCode
            + " in the "
            + GAMEKIT_AWS_REGION_MAPPINGS_FILE_NAME +
            " file. This most likely means you are trying to use a newly launched AWS Region and the AWS GameKit plugin hasn't been updated yet. Please add the new region to your "
            + GAMEKIT_AWS_REGION_MAPPINGS_FILE_NAME
            + " file.";
        Logging::Log(m_logCb, Level::Error, message.c_str());
        return "";
    }
}