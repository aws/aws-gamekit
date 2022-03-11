// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <string>

namespace GameKit
{
    // Template system variables. These variables are replaced when instance templates are saved.
    namespace TemplateVars
    {
        static const std::string BEGIN = "\\{\\{";
        static const std::string BEGIN_NO_ESCAPE = "{{";
        static const std::string END = "\\}\\}";
        static const std::string END_NO_ESCAPE = "}}";
        static const std::string AWS_GAMEKIT_ENVIRONMENT = "AWSGAMEKIT::SYS::ENV";
        static const std::string AWS_GAMEKIT_GAMENAME = "AWSGAMEKIT::SYS::GAMENAME";
        static const std::string AWS_GAMEKIT_USERVAR_PREFIX = "AWSGAMEKIT::VARS::";
        static const std::string AWS_GAMEKIT_CLOUDFORMATION_OUTPUT_PREFIX = "AWSGAMEKIT::CFNOUTPUT::";
        static const std::string AWS_GAMEKIT_BASE36_AWS_ACCOUNTID = "AWSGAMEKIT::SYS::BASE36AWSACCOUNTID";
        static const std::string AWS_GAMEKIT_SHORT_REGION_CODE = "AWSGAMEKIT::SYS::SHORTREGIONCODE";
    }

    namespace ResourceDirectories
    {
        static const std::string CLOUDFORMATION_DIRECTORY = "/cloudformation/";
        static const std::string LAYERS_DIRECTORY = "/layers/";
        static const std::string FUNCTIONS_DIRECTORY = "/functions/";
        static const std::string CONFIG_OUTPUTS_DIRECTORY = "/configOutputs/";
    }

    namespace TemplateFileNames
    {
        static const std::string FEATURE_DASHBOARD_FILE = "dashboard.yml";
        static const std::string CLOUDFORMATION_FILE = "cloudFormation.yml";
        static const std::string PARAMETERS_FILE = "parameters.yml";
        static const std::string FEATURE_CLIENT_CONFIGURATION_FILE = "clientConfig.yml";
        static const std::string GAMEKIT_CLIENT_CONFIGURATION_FILE = "awsGameKitClientConfig.yml";
    }
}
