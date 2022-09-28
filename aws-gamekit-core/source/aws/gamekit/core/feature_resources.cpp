// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// GameKit
#include <aws/gamekit/core/feature_resources.h>
#include <aws/gamekit/core/gamekit_settings.h>
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>
#include <aws/gamekit/core/utils/file_utils.h>
#include <aws/gamekit/core/gamekit_account.h>

// Boost
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace GameKit;
using namespace GameKit::Logger;
using namespace Aws::CloudFormation;

namespace CfnModel = Aws::CloudFormation::Model;
namespace LambdaModel = Aws::Lambda::Model;
namespace S3Model = Aws::S3::Model;
namespace SSMModel = Aws::SSM::Model;
namespace fs = boost::filesystem;

#pragma region Constructors/Destructor
GameKitFeatureResources::GameKitFeatureResources(const AccountInfo accountInfo, const AccountCredentials credentials, FeatureType featureType, FuncLogCallback logCb) :
    GameKitFeatureResources(CreateAccountInfoCopy(accountInfo), CreateAccountCredentialsCopy(credentials), featureType, logCb)
{}

GameKitFeatureResources::GameKitFeatureResources(const AccountInfoCopy& accountInfo, const AccountCredentialsCopy& credentials, FeatureType featureType, FuncLogCallback logCb)
{
    m_accountInfo = accountInfo;
    m_credentials = credentials;
    m_credentials.accountId = accountInfo.accountId;
    m_featureType = featureType;
    m_logCb = logCb;

    m_resouceStatusMap = {};
    m_stackName = GetStackName();

    GameKit::AwsApiInitializer::Initialize(m_logCb, this);
    this->InitializeDefaultAwsClients();

    Logging::Log(m_logCb, Level::Info, "GameKitFeatureResources()", this);
}

GameKitFeatureResources::~GameKitFeatureResources()
{
    Logging::Log(m_logCb, Level::Info, "~GameKitFeatureResources()", this);

    if (!m_isUsingSharedS3Client)
    {
        Logging::Log(m_logCb, Level::Info, "~GameKitFeatureResources() m_s3Client", this);
        delete(m_s3Client);
    }
    if (!m_isUsingSharedSSMClient)
    {
        Logging::Log(m_logCb, Level::Info, "~GameKitFeatureResources() m_ssmClient", this);
        delete(m_ssmClient);
    }
    if (!m_isUsingSharedCfClient)
    {
        Logging::Log(m_logCb, Level::Info, "~GameKitFeatureResources() m_cfClient", this);
        delete(m_cfClient);
    }
    if (!m_isUsingSharedLambdaClient)
    {
        Logging::Log(m_logCb, Level::Info, "~GameKitFeatureResources() m_lambdaClient", this);
        delete(m_lambdaClient);
    }

    // Safe to shutdown here. Other objects that rely on it will shut it down when they go
    // out of scope or are deleted.
    GameKit::AwsApiInitializer::Shutdown(m_logCb, this);

    if (m_isUsingSharedS3Client || m_isUsingSharedSSMClient || m_isUsingSharedCfClient || m_isUsingSharedLambdaClient)
    {
        return;
    }
}
#pragma endregion

#pragma region Public Methods
void GameKitFeatureResources::InitializeDefaultAwsClients()
{
    this->SetS3Client(DefaultClients::GetDefaultS3Client(this->GetAccountCredentials()), false);
    this->SetCloudFormationClient(DefaultClients::GetDefaultCloudFormationClient(this->GetAccountCredentials()), false);
    this->SetSSMClient(DefaultClients::GetDefaultSSMClient(this->GetAccountCredentials()), false);
    this->SetLambdaClient(DefaultClients::GetDefaultLambdaClient(this->GetAccountCredentials()), false);
}

bool GameKit::GameKitFeatureResources::IsCloudFormationInstanceTemplatePresent() const
{
    return boost::filesystem::exists(m_instanceCloudformationPath);
}

bool GameKit::GameKitFeatureResources::AreLayerInstancesPresent() const
{
    return boost::filesystem::exists(m_instanceLayersPath);
}

bool GameKit::GameKitFeatureResources::AreFunctionInstancesPresent() const
{
    return boost::filesystem::exists(m_instanceFunctionsPath);
}

unsigned int GameKitFeatureResources::SaveDeployedCloudFormationTemplate() const
{
    std::string templateBody;
    const auto getTemplateResult = getDeployedTemplateBody(m_stackName, templateBody);
    if (getTemplateResult != GAMEKIT_SUCCESS)
    {
        return getTemplateResult;
    }

    const auto writeResult = writeCloudFormationTemplateInstance(templateBody);
    if (writeResult != GAMEKIT_SUCCESS)
    {
        return writeResult;
    }

    const auto describeStackResourcesRequest = CfnModel::DescribeStackResourcesRequest().WithStackName(ToAwsString(m_stackName));

    CfnModel::DescribeStackResourcesOutcome describeResourcesOutcome = m_cfClient->DescribeStackResources(describeStackResourcesRequest);
    Aws::Vector<CfnModel::StackResource> resources = describeResourcesOutcome.GetResult().GetStackResources();
    for (auto& resource : resources)
    {
        const std::string physicalId = ToStdString(resource.GetPhysicalResourceId());
        if (resource.GetResourceType() == "AWS::CloudFormation::Stack")
        {
            // A Stack's physical ID is the ARN. We need to strip out the ARN strings so we only get the stack name. But we're only interested in CloudWatchDashboardStack
            std::smatch match;
            const std::regex re("arn:aws:cloudformation:[a-z0-9-]+:[0-9]{12}:stack/([a-zA-Z0-9-]+-CloudWatchDashboardStack-[a-zA-Z0-9-]+)/[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}");
            if (!std::regex_search(physicalId, match, re))
            {
                continue;
            }

            // We know this is a valid stack since it was returned as a CloudFormation resource. We can ignore the results of these calls.
            const std::string nestedStackName = match[1];
            getDeployedTemplateBody(nestedStackName, templateBody);
            writeCloudFormationDashboardInstance(templateBody);
            break;
        }
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitFeatureResources::GetDeployedCloudFormationParameters(DeployedParametersCallback callback) const
{
    // If we're given no callback function, this serves no purpose
    if (callback == nullptr)
        return GAMEKIT_ERROR_GENERAL;

    const auto describeStackReq = CfnModel::DescribeStacksRequest()
        .WithStackName(ToAwsString(m_stackName));

    auto outcome = m_cfClient->DescribeStacks(describeStackReq);
    if (!outcome.IsSuccess())
    {
        return GAMEKIT_ERROR_CLOUDFORMATION_DESCRIBE_STACKS_FAILED;
    }

    Aws::Vector<Aws::CloudFormation::Model::Stack> stacks = outcome.GetResult().GetStacks();
    if (stacks.size() > 0)
    {
        // Build a parameter map for easier lookups later
        Aws::Vector<Aws::CloudFormation::Model::Parameter> params = stacks.at(0).GetParameters();
        std::map<std::string, std::string> paramMap;
        for (auto param : params)
        {
            paramMap.insert({ ToStdString(param.GetParameterKey()), ToStdString(param.GetParameterValue()) });
        }

        YAML::Node cfnParams;
        Utils::FileUtils::ReadFileAsYAML(m_baseCloudformationPath + TemplateFileNames::PARAMETERS_FILE, cfnParams);

        for (YAML::const_iterator it = cfnParams.begin(); it != cfnParams.end(); ++it) 
        {
            std::string key = it->first.as<std::string>();
            YAML::Node nestedNode = it->second.as<YAML::Node>();

            // There should only be `value:` as a nested key
            std::string internalVariableName = nestedNode.begin()->second.as<std::string>();

            // check to see if this is a templated param we should save from CF
            if (internalVariableName.find(TemplateVars::AWS_GAMEKIT_USERVAR_PREFIX) != std::string::npos)
            {
                boost::replace_all(internalVariableName, TemplateVars::AWS_GAMEKIT_USERVAR_PREFIX, "");
                boost::replace_all(internalVariableName, TemplateVars::BEGIN_NO_ESCAPE, "");
                boost::replace_all(internalVariableName, TemplateVars::END_NO_ESCAPE, "");

                std::string existingValue = paramMap.at(key);
                if (existingValue.length() > 0)
                {
                    callback(internalVariableName.c_str(), existingValue.c_str());
                }
            }
        }
        return GAMEKIT_SUCCESS;
    }

    return GAMEKIT_ERROR_CLOUDFORMATION_DESCRIBE_STACKS_FAILED;
}

unsigned int GameKitFeatureResources::SaveCloudFormationInstance()
{
    return SaveCloudFormationInstance("UNKNOWN", "UNKNOWN");
}

unsigned int GameKitFeatureResources::SaveCloudFormationInstance(std::string sourceEngine, std::string pluginVersion)
{
    // If region name cannot be converted to short region code, return an error (all s3 buckets use 5-letter short region codes)
    const std::string shortRegionCode = getShortRegionCode();
    if (shortRegionCode.empty())
    {
        return GameKit::GAMEKIT_ERROR_REGION_CODE_CONVERSION_FAILED;
    }

    auto cfTemplate = this->getCloudFormationTemplate(TemplateType::Base);
    auto cfDashboard = this->getFeatureDashboardTemplate(TemplateType::Base);
    auto cfParams = this->getRawStackParameters(TemplateType::Base);

    // regex swap description to describe the engine and version
    std::regex targetLine("Description: \\(GAMEKIT(.*)\\).*");

    std::string replacementDescription = "Description: (GAMEKIT-$1-" + sourceEngine + ") The AWS CloudFormation template for AWS GameKit" + GetFeatureTypeString(m_featureType) + ". v" + pluginVersion;
    cfTemplate = std::regex_replace(cfTemplate, targetLine, replacementDescription);
    cfDashboard = std::regex_replace(cfDashboard, targetLine, replacementDescription);

    // replace occurrances of GAMEKIT System Variables AWSGAMEKIT::SYS::*
    std::regex envVar(TemplateVars::BEGIN + TemplateVars::AWS_GAMEKIT_ENVIRONMENT + TemplateVars::END);
    cfTemplate = std::regex_replace(cfTemplate, envVar, m_accountInfo.environment.GetEnvironmentString());
    cfDashboard = std::regex_replace(cfDashboard, envVar, m_accountInfo.environment.GetEnvironmentString());
    cfParams = std::regex_replace(cfParams, envVar, m_accountInfo.environment.GetEnvironmentString());

    std::regex gameName(TemplateVars::BEGIN + TemplateVars::AWS_GAMEKIT_GAMENAME + TemplateVars::END);
    cfTemplate = std::regex_replace(cfTemplate, gameName, m_accountInfo.gameName);
    cfDashboard = std::regex_replace(cfDashboard, gameName, m_accountInfo.gameName);
    cfParams = std::regex_replace(cfParams, gameName, m_accountInfo.gameName);

    const std::string base36AwsAccountIdStr = GameKit::Utils::EncodingUtils::DecimalToBase(m_accountInfo.accountId, GameKit::Utils::BASE_36);
    std::regex base36AwsAccountId(TemplateVars::BEGIN + TemplateVars::AWS_GAMEKIT_BASE36_AWS_ACCOUNTID + TemplateVars::END);
    cfTemplate = std::regex_replace(cfTemplate, base36AwsAccountId, base36AwsAccountIdStr);
    cfDashboard = std::regex_replace(cfDashboard, base36AwsAccountId, base36AwsAccountIdStr);
    cfParams = std::regex_replace(cfParams, base36AwsAccountId, base36AwsAccountIdStr);

    const std::regex shortRegionCodeRegex(TemplateVars::BEGIN + TemplateVars::AWS_GAMEKIT_SHORT_REGION_CODE + TemplateVars::END);
    cfTemplate = std::regex_replace(cfTemplate, shortRegionCodeRegex, shortRegionCode);
    cfDashboard = std::regex_replace(cfDashboard, shortRegionCodeRegex, shortRegionCode);
    cfParams = std::regex_replace(cfParams, shortRegionCodeRegex, shortRegionCode);

    //  save to GAMEKIT_ROOT
    auto writeResult = writeCloudFormationParameterInstance(cfParams);
    if (writeResult != GAMEKIT_SUCCESS)
    {
        return writeResult;
    }

    writeResult = writeCloudFormationTemplateInstance(cfTemplate);
    if (writeResult != GAMEKIT_SUCCESS)
    {
        return writeResult;
    }

    writeResult = writeCloudFormationDashboardInstance(cfDashboard);
    if (writeResult != GAMEKIT_SUCCESS)
    {
        return writeResult;
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitFeatureResources::UpdateCloudFormationParameters()
{
    // If region name cannot be converted to short region code, return an error (all s3 buckets use 5-letter short region codes)
    const std::string shortRegionCode = getShortRegionCode();
    if (shortRegionCode.empty())
    {
        return GameKit::GAMEKIT_ERROR_REGION_CODE_CONVERSION_FAILED;
    }

    auto cfParams = this->getRawStackParameters(TemplateType::Base);

    // replace occurrances of GAMEKIT System Variables AWSGAMEKIT::SYS::*
    const std::regex envVar(TemplateVars::BEGIN + TemplateVars::AWS_GAMEKIT_ENVIRONMENT + TemplateVars::END);
    cfParams = std::regex_replace(cfParams, envVar, m_accountInfo.environment.GetEnvironmentString());

    const std::regex gameName(TemplateVars::BEGIN + TemplateVars::AWS_GAMEKIT_GAMENAME + TemplateVars::END);
    cfParams = std::regex_replace(cfParams, gameName, m_accountInfo.gameName);

    const std::regex base36AwsAccountId(TemplateVars::BEGIN + TemplateVars::AWS_GAMEKIT_BASE36_AWS_ACCOUNTID + TemplateVars::END);
    cfParams = std::regex_replace(cfParams, base36AwsAccountId, GameKit::Utils::EncodingUtils::DecimalToBase(m_accountInfo.accountId, GameKit::Utils::BASE_36));

    const std::regex shortRegionCodeRegex(TemplateVars::BEGIN + TemplateVars::AWS_GAMEKIT_SHORT_REGION_CODE + TemplateVars::END);
    cfParams = std::regex_replace(cfParams, shortRegionCodeRegex, shortRegionCode);

    // Do not replace AWSGAMEKIT::VARS::* values; these will be replaced at time of deployment

    //  save to GAMEKIT_ROOT
    return writeCloudFormationParameterInstance(cfParams);
}

unsigned int GameKitFeatureResources::SaveLayerInstances() const
{
    // if there's nothing to copy, just return success
    if (!boost::filesystem::exists(m_baseLayersPath))
    {
        return GAMEKIT_SUCCESS;
    }

    // create destination instance directory and copy the base layers directory to the instance layers directory
    std::string logMsg;
    boost::filesystem::create_directories(m_instanceLayersPath);
    boost::system::error_code err;
    boost::filesystem::copy(m_baseLayersPath, m_instanceLayersPath, boost::filesystem::copy_options::recursive | boost::filesystem::copy_options::overwrite_existing, err);
    if (err.failed())
    {
        logMsg = std::string("Failed to copy Lambda Layers to ").append(m_instanceLayersPath).append("; ").append(err.message());
        Logging::Log(m_logCb, Level::Error, logMsg.c_str(), this);
        return GAMEKIT_ERROR_FUNCTIONS_COPY_FAILED;
    }

    logMsg = std::string("Lambda Layers copied to ").append(m_instanceLayersPath);
    Logging::Log(m_logCb, Level::Info, logMsg.c_str(), this);
    return GAMEKIT_SUCCESS;
}

unsigned int GameKitFeatureResources::SaveFunctionInstances() const
{
    // if there's nothing to copy, just return success
    if (!boost::filesystem::exists(m_baseFunctionsPath))
    {
        return GAMEKIT_SUCCESS;
    }

    // create destination instance directory and copy the base function directory to the instance function directory
    std::string logMsg;
    boost::filesystem::create_directories(m_instanceFunctionsPath);
    boost::system::error_code err;
    boost::filesystem::copy(m_baseFunctionsPath, m_instanceFunctionsPath, boost::filesystem::copy_options::recursive | boost::filesystem::copy_options::overwrite_existing, err);
    if (err.failed())
    {
        logMsg = std::string("Failed to copy Lambda Functions to ").append(m_instanceFunctionsPath).append("; ").append(err.message());
        Logging::Log(m_logCb, Level::Error, logMsg.c_str(), this);
        return GAMEKIT_ERROR_FUNCTIONS_COPY_FAILED;
    }

    logMsg = std::string("Lambda Functions copied to ").append(m_instanceFunctionsPath);
    Logging::Log(m_logCb, Level::Info, logMsg.c_str(), this);
    return GAMEKIT_SUCCESS;
}

void GameKitFeatureResources::SetLayersReplacementId(const std::string& replacementId)
{
    m_layersReplacementId = replacementId;
}

void GameKitFeatureResources::SetFunctionsReplacementId(const std::string& replacementId)
{
    m_functionsReplacementId = replacementId;
}

unsigned int GameKitFeatureResources::CreateAndSetLayersReplacementId()
{
    // get current time in milliseconds and use it as the function replacement id
    const std::chrono::milliseconds ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
    const std::string replacementId = std::to_string(ts.count());

    const std::string paramName = GetLambdaLayerReplacementIDParamName();

    // add GAMEKIT_LAMBDA_LAYERS_REPLACEMENT_ID to Parameter Store with the replacement id
    SSMModel::PutParameterRequest putParamRequest;
    putParamRequest.SetType(SSMModel::ParameterType::String);
    putParamRequest.SetName(ToAwsString(paramName));
    putParamRequest.SetValue(replacementId.c_str());
    putParamRequest.SetOverwrite(true);

    auto putParamOutcome = m_ssmClient->PutParameter(putParamRequest);
    if (!putParamOutcome.IsSuccess())
    {
        Logging::Log(m_logCb, Level::Error, putParamOutcome.GetError().GetMessage().c_str(), this);
        return GAMEKIT_ERROR_PARAMSTORE_WRITE_FAILED;
    }

    m_layersReplacementId = replacementId;

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitFeatureResources::CreateAndSetFunctionsReplacementId()
{
    // get current time and use it as the function replacement id
    const std::chrono::milliseconds ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
    const std::string replacementId = std::to_string(ts.count());

    const std::string paramName = GetLambdaFunctionReplacementIDParamName();

    // add GAMEKIT_LAMBDA_FUNCTIONS_REPLACEMENT_ID to Parameter Store with the replacement id
    SSMModel::PutParameterRequest putParamRequest;
    putParamRequest.SetType(SSMModel::ParameterType::String);
    putParamRequest.SetName(ToAwsString(paramName));
    putParamRequest.SetValue(ToAwsString(replacementId));
    putParamRequest.SetOverwrite(true);

    auto putParamOutcome = m_ssmClient->PutParameter(putParamRequest);
    if (!putParamOutcome.IsSuccess())
    {
        Logging::Log(m_logCb, Level::Error, putParamOutcome.GetError().GetMessage().c_str(), this);
        return GAMEKIT_ERROR_PARAMSTORE_WRITE_FAILED;
    }

    m_functionsReplacementId = replacementId;

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitFeatureResources::UploadDashboard(const std::string& path)
{
    Logging::Log(m_logCb, Level::Verbose, "Start UploadDashboard()", this);

    const fs::path cp = (path + "/" + TemplateFileNames::FEATURE_DASHBOARD_FILE);

    fs::directory_iterator endIterator;
    // Verify that the path exists and is a directory
    if (fs::exists(cp) && fs::is_regular_file(cp))
    {
        assert(GameKit::AwsApiInitializer::IsInitialized());

        // If region name cannot be converted to short region code, return an error (all s3 buckets use 5-letter short region codes)
        const std::string shortRegionCode = getShortRegionCode();
        if (shortRegionCode.empty())
        {
            return GameKit::GAMEKIT_ERROR_REGION_CODE_CONVERSION_FAILED;
        }

        // set put request params
        const std::string objectName = std::string("cloudformation/")
            .append(GameKit::GetFeatureTypeString(m_featureType))
            .append("/")
            .append(TemplateFileNames::FEATURE_DASHBOARD_FILE);

        const std::shared_ptr<Aws::IOStream> inputData = Aws::MakeShared<Aws::FStream>(
            cp.string().c_str(),
            cp.native(),
            std::ios_base::in | std::ios_base::binary);

        S3Model::PutObjectRequest putObjRequest;

        putObjRequest.SetBucket(ToAwsString(GetBootstrapBucketName(m_accountInfo, shortRegionCode)));
        putObjRequest.SetKey(ToAwsString(objectName));
        putObjRequest.SetBody(inputData);
        putObjRequest.SetExpectedBucketOwner(ToAwsString(m_accountInfo.accountId));

        // upload yaml file
        Logging::Log(m_logCb, Level::Verbose, "GameKitFeatureResources::UploadDashboard() Start put object", m_s3Client);
        auto putObjOutcome = m_s3Client->PutObject(putObjRequest);
        Logging::Log(m_logCb, Level::Verbose, "GameKitFeatureResources::UploadDashboard() End put object", m_s3Client);

        if (!putObjOutcome.IsSuccess())
        {
            Logging::Log(m_logCb, Level::Error, putObjOutcome.GetError().GetMessage().c_str(), this);
            return GAMEKIT_ERROR_BOOTSTRAP_BUCKET_UPLOAD_FAILED;
        }

    }

    Logging::Log(m_logCb, Level::Verbose, "End UploadDashboard()", this);

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitFeatureResources::CompressFeatureLayers()
{
    // loop through the feature's layers directory
    std::string layerName;
    Aws::UniquePtr<Zipper> zipper;
    fs::path p(m_instanceLayersPath);
    fs::directory_iterator endIterator;

    // Verify that path exists and is a directory
    if (fs::exists(p) && fs::is_directory(p))
    {
        for (fs::directory_iterator dirIterator(p); dirIterator != endIterator; ++dirIterator)
        {
            // create zip file for every layers directory
            const fs::path cp = (*dirIterator);

            // Verify that path is a directory
            if (fs::is_directory(cp))
            {
                layerName = cp.stem().string();

                std::string layerHash;
                const unsigned int result = GameKit::Utils::FileUtils::CalculateDirectoryHash(cp.string(), layerHash);

                if (result == GAMEKIT_SUCCESS && !lambdaLayerHashChanged(layerName, layerHash))
                {
                    // update hash param
                    if (createAndSetLambdaLayerHash(layerName, layerHash) != GAMEKIT_SUCCESS)
                    {
                        std::string msg = std::string("Unable to save layer hash for ").append(layerName);
                        Logging::Log(m_logCb, Level::Error, msg.c_str());
                    }


                    // create output directory in temp path
                    std::string tempLayersPath = getTempLayersPath();
                    fs::create_directories(tempLayersPath);

                    // create zip file
                    std::string zipFileName = tempLayersPath + "/" + layerName + ".zip";
                    zipper = Aws::MakeUnique<Zipper>(zipFileName.c_str(), cp.string(), zipFileName);
                    if (!zipper->AddDirectoryToZipFile(cp.string()))
                    {
                        std::string msg = std::string("Unable to initialize ").append(zipFileName);
                        Logging::Log(m_logCb, Level::Error, msg.c_str());
                        return GAMEKIT_ERROR_LAYER_ZIP_INIT_FAILED;
                    }

                    // write zip file to disk
                    if (!zipper->CloseZipFile())
                    {
                        std::string msg = std::string("Unable to write ").append(zipFileName).append(" to disk");
                        Logging::Log(m_logCb, Level::Error, msg.c_str(), this);
                        return GAMEKIT_ERROR_LAYER_ZIP_WRITE_FAILED;
                    }

                    // zip file creation successful
                    std::string msg = std::string("Zip file ")
                        .append(zipFileName)
                        .append(" created");
                    Logging::Log(m_logCb, Level::Info, msg.c_str(), this);
                }
            }
        }
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitFeatureResources::UploadFeatureLayers()
{
    Logging::Log(m_logCb, Level::Verbose, "Start UploadFeatureLayers()", this);

    // loop through feature layers temp directory
    const std::string tempLayersPath = getTempLayersPath();
    const fs::path p(tempLayersPath);
    fs::directory_iterator endIterator;

    // If region name cannot be converted to short region code, return an error (all s3 buckets use 5-letter short region codes)
    const std::string shortRegionCode = getShortRegionCode();
    if (shortRegionCode.empty())
    {
        return GameKit::GAMEKIT_ERROR_REGION_CODE_CONVERSION_FAILED;
    }

    // Verify that the path exists and is a directory
    if (fs::exists(p) && fs::is_directory(p))
    {
        for (fs::directory_iterator dirIterator(p); dirIterator != endIterator; ++dirIterator)
        {
            assert(GameKit::AwsApiInitializer::IsInitialized());
            const fs::path cp = (*dirIterator);

            // Verify that the path is a file
            if (fs::is_regular_file(cp))
            {
                std::string layerDirName = cp.stem().string();

                // set put request params
                std::string objectName = std::string("layers/")
                    .append(GameKit::GetFeatureTypeString(m_featureType))
                    .append("/")
                    .append(layerDirName)
                    .append(".")
                    .append(m_layersReplacementId)
                    .append(cp.extension().string());

                std::shared_ptr<Aws::IOStream> inputData = Aws::MakeShared<Aws::FStream>(
                    cp.string().c_str(),
                    cp.native(),
                    std::ios_base::in | std::ios_base::binary);

                S3Model::PutObjectRequest putObjRequest;
                putObjRequest.SetBucket(ToAwsString(GetBootstrapBucketName(m_accountInfo, shortRegionCode)));
                putObjRequest.SetKey(ToAwsString(objectName));
                putObjRequest.SetBody(inputData);
                putObjRequest.SetExpectedBucketOwner(ToAwsString(m_accountInfo.accountId));

                // upload zip file
                Logging::Log(m_logCb, Level::Verbose, "GameKitFeatureResources::UploadFeatureLayers() Start put object", m_s3Client);
                auto putObjOutcome = m_s3Client->PutObject(putObjRequest);
                Logging::Log(m_logCb, Level::Verbose, "GameKitFeatureResources::UploadFeatureLayers() End put object", m_s3Client);

                if (!putObjOutcome.IsSuccess())
                {
                    Logging::Log(m_logCb, Level::Error, putObjOutcome.GetError().GetMessage().c_str(), this);
                    return GAMEKIT_ERROR_BOOTSTRAP_BUCKET_UPLOAD_FAILED;
                }

                std::string msg = std::string("Object: ")
                    .append(objectName)
                    .append(" uploaded to: ")
                    .append(GetBootstrapBucketName(m_accountInfo, shortRegionCode))
                    .append("; ETag: ")
                    .append(ToStdString(putObjOutcome.GetResult().GetETag()));

                Logging::Log(m_logCb, Level::Info, msg.c_str(), this);

                // create Lambda layer
                msg = std::string("GameKitFeatureResources::UploadFeatureLayers() Creating Lambda Layer for ").append(layerDirName);
                auto layerCreationOutcome = createFeatureLayer(layerDirName, objectName);
                if (!layerCreationOutcome.IsSuccess())
                {
                    return GAMEKIT_ERROR_LAYER_CREATION_FAILED;
                }

                // get latest version ARN and set it in parameter store
                std::string latestArn = ToStdString(layerCreationOutcome.GetResult().GetLayerVersionArn());
                unsigned int paramWriteResult = createAndSetLambdaLayerArn(layerDirName, latestArn);
                if (paramWriteResult != GAMEKIT_SUCCESS)
                {
                    return paramWriteResult;
                }
            }
        }
    }

    Logging::Log(m_logCb, Level::Verbose, "End UploadFeatureLayers()", this);

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitFeatureResources::DeployFeatureLayers()
{
    unsigned int result = CreateAndSetLayersReplacementId();
    if (result != GameKit::GAMEKIT_SUCCESS)
    {
        return result;
    }

    result = CompressFeatureLayers();
    if (result != GameKit::GAMEKIT_SUCCESS)
    {
        CleanupTempFiles();
        return result;
    }

    result = UploadFeatureLayers();
    if (result != GameKit::GAMEKIT_SUCCESS)
    {
        CleanupTempFiles();
        return result;
    }

    CleanupTempFiles();

    return result;
}

unsigned int GameKitFeatureResources::CompressFeatureFunctions()
{
    // loop through the feature's functions directory
    std::string functionName;
    Aws::UniquePtr<Zipper> zipper;
    const fs::path p(m_instanceFunctionsPath);
    fs::directory_iterator endIterator;

    // Verify that path exists and is a directory
    if (fs::exists(p) && fs::is_directory(p))
    {
        for (fs::directory_iterator dirIterator(p); dirIterator != endIterator; ++dirIterator)
        {
            // create zip file for every function directory
            const fs::path cp = (*dirIterator);

            // Verify that path is a directory
            if (fs::is_directory(cp))
            {
                // create output directory in temp path
                std::string tempFunctionsPath = getTempFunctionsPath();
                fs::create_directories(tempFunctionsPath);

                // create zip file
                functionName = cp.stem().string();
                std::string zipFileName = tempFunctionsPath + "/" + functionName + ".zip";
                zipper = Aws::MakeUnique<Zipper>(zipFileName.c_str(), cp.string(), zipFileName);
                bool result = zipper->AddDirectoryToZipFile(cp.string());
                if (!result)
                {
                    std::string msg = std::string("Unable to initialize ").append(zipFileName);
                    Logging::Log(m_logCb, Level::Error, msg.c_str());
                    return GAMEKIT_ERROR_FUNCTION_ZIP_INIT_FAILED;
                }

                // write zip file to disk
                result = zipper->CloseZipFile();
                if (!result)
                {
                    std::string msg = std::string("Unable to write ").append(zipFileName).append(" to disk");
                    Logging::Log(m_logCb, Level::Error, msg.c_str(), this);
                    return GAMEKIT_ERROR_FUNCTION_ZIP_WRITE_FAILED;
                }

                // zip file creation successful
                std::string msg = std::string("Zip file ")
                    .append(zipFileName)
                    .append(" created");
                Logging::Log(m_logCb, Level::Info, msg.c_str(), this);
            }
        }
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitFeatureResources::UploadFeatureFunctions()
{
    Logging::Log(m_logCb, Level::Verbose, "Start UploadFeatureFunctions()", this);

    // loop through feature functions temp directory
    const std::string tempFunctionsPath = getTempFunctionsPath();
    const fs::path p(tempFunctionsPath);
    fs::directory_iterator endIterator;

    // If region name cannot be converted to short region code, return an error (all s3 buckets use 5-letter short region codes)
    const std::string shortRegionCode = getShortRegionCode();
    if (shortRegionCode.empty())
    {
        return GameKit::GAMEKIT_ERROR_REGION_CODE_CONVERSION_FAILED;
    }
    const std::string bootstrapBucketName = GetBootstrapBucketName(m_accountInfo, shortRegionCode);

    // Verify that path exists and is a directory
    if (fs::exists(p) && fs::is_directory(p))
    {
        for (fs::directory_iterator dirIterator(p); dirIterator != endIterator; ++dirIterator)
        {
            assert(GameKit::AwsApiInitializer::IsInitialized());
            const fs::path cp = (*dirIterator);

            // Verify that path is a file
            if (fs::is_regular_file(cp))
            {
                // set put request params
                std::string objectName = std::string("functions/")
                    .append(GameKit::GetFeatureTypeString(m_featureType))
                    .append("/")
                    .append(cp.stem().string())
                    .append(".")
                    .append(m_functionsReplacementId)
                    .append(cp.extension().string());

                std::shared_ptr<Aws::IOStream> inputData = Aws::MakeShared<Aws::FStream>(
                    cp.string().c_str(),
                    cp.native(),
                    std::ios_base::in | std::ios_base::binary);

                S3Model::PutObjectRequest putObjRequest;
                putObjRequest.SetExpectedBucketOwner(ToAwsString(m_accountInfo.accountId));
                putObjRequest.SetBucket(ToAwsString(bootstrapBucketName));
                putObjRequest.SetKey(ToAwsString(objectName));
                putObjRequest.SetBody(inputData);

                // upload zip file
                Logging::Log(m_logCb, Level::Verbose, "GameKitFeatureResources::UploadFeatureFunctions() Start put object", m_s3Client);
                auto putObjOutcome = m_s3Client->PutObject(putObjRequest);
                Logging::Log(m_logCb, Level::Verbose, "GameKitFeatureResources::UploadFeatureFunctions() End put object", m_s3Client);

                if (putObjOutcome.IsSuccess())
                {
                    std::string msg = std::string("Object: ")
                        .append(objectName)
                        .append(" uploaded to: ")
                        .append(bootstrapBucketName.c_str())
                        .append("; ETag: ")
                        .append(ToStdString(putObjOutcome.GetResult().GetETag()));

                    Logging::Log(m_logCb, Level::Info, msg.c_str(), this);
                }
                else
                {
                    Logging::Log(m_logCb, Level::Error, putObjOutcome.GetError().GetMessage().c_str(), this);
                    return GAMEKIT_ERROR_BOOTSTRAP_BUCKET_UPLOAD_FAILED;
                }
            }
        }
    }

    Logging::Log(m_logCb, Level::Verbose, "End UploadFeatureFunctions()", this);

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitFeatureResources::DeployFeatureFunctions()
{
    unsigned int result = CreateAndSetFunctionsReplacementId();
    if (result != GameKit::GAMEKIT_SUCCESS)
    {
        return result;
    }

    result = CompressFeatureFunctions();
    if (result != GameKit::GAMEKIT_SUCCESS)
    {
        CleanupTempFiles();
        return result;
    }

    result = UploadFeatureFunctions();
    if (result != GameKit::GAMEKIT_SUCCESS)
    {
        CleanupTempFiles();
        return result;
    }

    CleanupTempFiles();

    return result;
}

void GameKitFeatureResources::CleanupTempFiles()
{
    if (!m_functionsReplacementId.empty())
    {
        const std::string functionsPath = getTempFunctionsPath();
        std::string message = "Deleting temp files from " + functionsPath;
        Logging::Log(m_logCb, Level::Info, message.c_str());
        fs::remove_all(functionsPath);
    }

    if (!m_layersReplacementId.empty())
    {
        const std::string layersPath = getTempLayersPath();
        std::string message = "Deleting temp files from " + layersPath;
        Logging::Log(m_logCb, Level::Info, message.c_str());
        fs::remove_all(layersPath);
    }
}

std::string GameKitFeatureResources::GetCurrentStackStatus() const
{
    const auto describeStackReq = CfnModel::DescribeStacksRequest()
        .WithStackName(m_stackName.c_str());

    auto outcome = m_cfClient->DescribeStacks(describeStackReq);
    auto stackStatus = CfnModel::StackStatus::NOT_SET;
    auto stacks = outcome.GetResult().GetStacks();

    if (stacks.size() > 0)
    {
        stackStatus = stacks.at(0).GetStackStatus();
    }

    if (stackStatus == CfnModel::StackStatus::CREATE_COMPLETE || stackStatus == CfnModel::StackStatus::UPDATE_COMPLETE)
    {
        const auto outputs = stacks.at(0).GetOutputs();
        const auto writeResult = this->writeClientConfigurationWithOutputs(outputs);
        if (writeResult != GAMEKIT_SUCCESS)
        {
            std::string msg = std::string("Failed to write client configuration parameters for ").append(m_stackName);
            Logging::Log(m_logCb, Level::Warning, msg.c_str(), this);
        }
    }

    const std::string status = ToStdString(CfnModel::StackStatusMapper::GetNameForStackStatus(stackStatus));

    // NOT_SET status maps to an empty string, give an actual status.
    return status.empty() ? GameKit::ERR_STACK_CURRENT_STATUS_UNDEPLOYED : status;
}

void GameKitFeatureResources::UpdateDashboardDeployStatus(std::unordered_set<FeatureType> features) const
{
    Aws::String nextToken = "";

    auto stackFilter = Aws::Vector<CfnModel::StackStatus>();
    stackFilter.push_back(CfnModel::StackStatus::CREATE_COMPLETE);
    stackFilter.push_back(CfnModel::StackStatus::UPDATE_COMPLETE);


    GameKitSettings settings = GameKitSettings(m_gamekitRoot, "", m_accountInfo.gameName, m_accountInfo.environment.GetEnvironmentString(), m_logCb);
    std::map<std::string, std::string> enabledMap = { {"cloudwatch_dashboard_enabled",  "true"} };
    std::map<std::string, std::string> disabledMap = { {"cloudwatch_dashboard_enabled", "false"} };
    std::unordered_set<FeatureType> enabledFeatureDashboards;

    // Loop for pagination
    do
    {
        // List functioning cloudformation stacks
        auto listRequest = CfnModel::ListStacksRequest().WithStackStatusFilter(stackFilter);
        if (!nextToken.empty())
        {
            listRequest.SetNextToken(nextToken);
        }
        CfnModel::ListStacksOutcome outcome = m_cfClient->ListStacks(listRequest);

        if (!outcome.IsSuccess())
        {
            return;
        }

        nextToken = outcome.GetResult().GetNextToken();
        Aws::Vector<CfnModel::StackSummary> stacks = outcome.GetResult().GetStackSummaries();

        for (auto s : stacks)
        {
            // Check if this stack matches the name of the dashboard for the feature given
            Aws::String awsStackName = s.GetStackName();
            std::string stackName = ToStdString(awsStackName);

            for (FeatureType feature : features)
            {
                if (stackName.substr(0, getStackName(feature).size()) == getStackName(feature) && (stackName.find("CloudWatchDashboardStack") != Aws::String::npos))
                {
                    settings.SetFeatureVariables(feature, enabledMap);
                    enabledFeatureDashboards.insert(feature);
                }
            }
        }
    } while(!nextToken.empty());

    for (FeatureType f : enabledFeatureDashboards)
    {
        features.erase(f);
    }
    for (FeatureType f : features)
    {
        // These features don't have a dashboard deployed, make sure they're set to disabled
        settings.SetFeatureVariables(f, disabledMap);
    }

    settings.SaveSettings();
}

unsigned int GameKitFeatureResources::internalDescribeFeatureResources(FuncResourceInfoCallback resourceInfoCb, DISPATCH_RECEIVER_HANDLE receiver, DispatchedResourceInfoCallback dispatchedCb) const
{
    auto describeStackResourcesRequest = CfnModel::DescribeStackResourcesRequest().WithStackName(ToAwsString(m_stackName));

    auto outcome = m_cfClient->DescribeStackResources(describeStackResourcesRequest);

    if (outcome.IsSuccess())
    {
        auto& resources = outcome.GetResult().GetStackResources();
        for (auto& resource : resources)
        {
            const char* logicalResourceId = resource.GetLogicalResourceId().c_str();
            const char* resourceType = resource.GetResourceType().c_str();
            CfnModel::ResourceStatus status = resource.GetResourceStatus();
            std::string statusStr = ToStdString(CfnModel::ResourceStatusMapper::GetNameForResourceStatus(status));

            if (receiver != nullptr && dispatchedCb != nullptr)
            {
                dispatchedCb(receiver, logicalResourceId, resourceType, statusStr.c_str());
            }
            else if (resourceInfoCb != nullptr)
            {
                resourceInfoCb(logicalResourceId, resourceType, statusStr.c_str());
            }
        }

        return GameKit::GAMEKIT_SUCCESS;
    }

    Logging::Log(m_logCb, Level::Error, outcome.GetError().GetMessage().c_str(), this);
    return GameKit::GAMEKIT_ERROR_CLOUDFORMATION_DESCRIBE_RESOURCE_FAILED;
}

unsigned int GameKitFeatureResources::DescribeStackResources(FuncResourceInfoCallback resourceInfoCb) const
{
    return internalDescribeFeatureResources(resourceInfoCb=resourceInfoCb);
}

unsigned int GameKitFeatureResources::DescribeStackResources(const DISPATCH_RECEIVER_HANDLE dispatchReceiver, DispatchedResourceInfoCallback resourceInfoCb) const
{
    return internalDescribeFeatureResources(nullptr, dispatchReceiver, resourceInfoCb);
}

std::string GameKitFeatureResources::getFeatureLayerNameFromDirName(const std::string& layerDirName) const
{
    return std::string("gamekit_")
                .append(m_accountInfo.environment.GetEnvironmentString()
                            .append("_"))
                .append(m_accountInfo.gameName)
                .append("_")
                .append(layerDirName);
}

LambdaModel::PublishLayerVersionOutcome GameKitFeatureResources::createFeatureLayer(const std::string& layerDirName, const std::string& s3ObjectName)
{
    const auto layerContent = LambdaModel::LayerVersionContentInput()
        .WithS3Bucket(ToAwsString(GetBootstrapBucketName(m_accountInfo, getShortRegionCode())))
        .WithS3Key(ToAwsString(s3ObjectName));

    const std::string layerName = getFeatureLayerNameFromDirName(layerDirName);
    const auto publishRequest = LambdaModel::PublishLayerVersionRequest()
                          .AddCompatibleRuntimes(LambdaModel::Runtime::python3_7)
                          .WithContent(layerContent)
                          .WithDescription(ToAwsString(GameKit::GetFeatureTypeString(m_featureType).append(" ").append("Lambda Layer ").append(layerDirName)))
                          .WithLayerName(ToAwsString(layerName));

    return m_lambdaClient->PublishLayerVersion(publishRequest);
}

bool GameKitFeatureResources::lambdaLayerHashChanged(const std::string& layerName, const std::string& layerHash) const
{
    const std::string paramName = GetLambdaLayerHashParamName(layerName);

    SSMModel::GetParameterRequest getParamRequest;
    getParamRequest.SetName(ToAwsString(paramName));

    const auto getParamOutcome = m_ssmClient->GetParameter(getParamRequest);
    if (!getParamOutcome.IsSuccess())
    {
        // SSM returns 400 for all errors except internal server error (500)
        // Warn if not internal server error, otherwise log as Error by default
        Level level = Level::Error;
        std::string defaultErrorMessage = std::string("Lambda Layer hash parameter not found for layer ").append(layerName);
        if (getParamOutcome.GetError().GetResponseCode() != Aws::Http::HttpResponseCode::INTERNAL_SERVER_ERROR)
        {
            level = Level::Warning;
            defaultErrorMessage.append(". This is expected when you deploy your first GameKit feature.");
        }

        // Returned error message may be empty. Use default error message instead.
        std::string errorMessage = (getParamOutcome.GetError().GetMessage().empty()) ? defaultErrorMessage : ToStdString(getParamOutcome.GetError().GetMessage());
        Logging::Log(m_logCb, level, errorMessage.c_str(), this);
        return false;
    }

    const auto lastRecordedHash = getParamOutcome.GetResult().GetParameter();
    if (std::string(lastRecordedHash.GetValue()) != layerHash)
    {
        return false;
    }

    return true;
}

unsigned int GameKitFeatureResources::createAndSetLambdaLayerHash(const std::string& layerName, const std::string& layerHash) const
{
    const std::string paramName = GetLambdaLayerHashParamName(layerName);

    SSMModel::PutParameterRequest putParamRequest;
    putParamRequest.SetType(SSMModel::ParameterType::String);
    putParamRequest.SetName(ToAwsString(paramName));
    putParamRequest.SetValue(ToAwsString(layerHash));
    putParamRequest.SetOverwrite(true);

    const auto putParamOutcome = m_ssmClient->PutParameter(putParamRequest);
    if (!putParamOutcome.IsSuccess())
    {
        Logging::Log(m_logCb, Level::Error, putParamOutcome.GetError().GetMessage().c_str(), this);
        return GAMEKIT_ERROR_PARAMSTORE_WRITE_FAILED;
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitFeatureResources::createAndSetLambdaLayerArn(const std::string& layerName, const std::string& layerArn) const
{
    // add GAMEKIT_LAMBDA_LAYER_ARN to Parameter Store with the replacement id
    const auto paramName = GetLambdaLayerARNParamName(layerName);

    SSMModel::PutParameterRequest putParamRequest;
    putParamRequest.SetType(SSMModel::ParameterType::String);
    putParamRequest.SetName(ToAwsString(paramName));
    putParamRequest.SetValue(ToAwsString(layerArn));
    putParamRequest.SetOverwrite(true);

    const auto putParamOutcome = m_ssmClient->PutParameter(putParamRequest);
    if (!putParamOutcome.IsSuccess())
    {
        Logging::Log(m_logCb, Level::Error, putParamOutcome.GetError().GetMessage().c_str(), this);
        return GAMEKIT_ERROR_PARAMSTORE_WRITE_FAILED;
    }

    return GAMEKIT_SUCCESS;
}

std::string GameKitFeatureResources::getShortRegionCode()
{
    if (GetPluginRoot().empty())
    {
        return "";
    }
    AwsRegionMappings& regionMappings = AwsRegionMappings::getInstance(GetPluginRoot(), m_logCb);
    return regionMappings.getFiveLetterRegionCode(m_credentials.region);
}

unsigned int GameKitFeatureResources::CreateOrUpdateFeatureStack()
{
    const auto describeStackReq = CfnModel::DescribeStacksRequest().WithStackName(ToAwsString(m_stackName));

    const auto outcome = m_cfClient->DescribeStacks(describeStackReq);
    unsigned int createUpdateResult;
    if (!outcome.IsSuccess())
    {
        createUpdateResult = createStack();
    }
    else
    {
        createUpdateResult = updateStack();
    }

    if (createUpdateResult != GAMEKIT_SUCCESS)
    {
        return createUpdateResult;
    }

    char buffer[256];

    snprintf(buffer, 256, "Creating stack resources for stack: %s", m_stackName.c_str());
    Logging::Log(m_logCb, Level::Info, buffer, this);
    const auto stackStatus = periodicallyDescribeStackEvents();

    // if last stack status is a failed state or deletion completion or deletion in progress, return a failed creation error code
    if (isFailedState(stackStatus) || stackStatus == CfnModel::StackStatus::DELETE_IN_PROGRESS || stackStatus == CfnModel::StackStatus::DELETE_COMPLETE)
    {
        Logging::Log(m_logCb, Level::Error, "CloudFormation creation failed.", this);
        return GameKit::GAMEKIT_ERROR_CLOUDFORMATION_RESOURCE_CREATION_FAILED;
    }

    // update clientConfig.yml
    const auto writeStatus = this->WriteClientConfiguration();
    if (writeStatus != GAMEKIT_SUCCESS)
    {
        snprintf(buffer, 256, "Failed to update clientConfig.yml for feature %s", GetFeatureTypeString(m_featureType).c_str());
        Logging::Log(m_logCb, Level::Error, buffer, this);
    }

    return GameKit::GAMEKIT_SUCCESS;
}

std::string GameKitFeatureResources::getClientConfigFilePath() const
{
    const std::string configDirectory = m_gamekitRoot + "/" + m_accountInfo.gameName + "/" + m_accountInfo.environment.GetEnvironmentString();
    const std::string configFilePath = configDirectory + "/" + TemplateFileNames::GAMEKIT_CLIENT_CONFIGURATION_FILE;
    return configFilePath;
}

unsigned int GameKitFeatureResources::writeClientConfigYamlToDisk(const YAML::Node& paramsYml) const
{
    return Utils::FileUtils::WriteYAMLToFile(paramsYml, getClientConfigFilePath(), Configuration::DO_NOT_EDIT, m_logCb);
}

unsigned int GameKitFeatureResources::removeOutputsFromClientConfiguration() const
{
    YAML::Node paramsYml = this->getClientConfigYaml();
    auto configParams = this->getConfigOutputParameters();
    if (configParams.size() == 0)
    {
        return GameKit::GAMEKIT_SUCCESS;
    }

    for (auto& param : configParams)
    {
        std::string paramKey = std::get<0>(param);
        paramsYml.remove(paramKey);
    }
    // Write updated config file
    return this->writeClientConfigYamlToDisk(paramsYml);
}

unsigned int GameKitFeatureResources::writeClientConfigurationWithOutputs(Aws::Vector<CfnModel::Output> outputs) const
{
    // Defensively check to make sure we actually are being passed new data and we are not working with Main stack,
    // otherwise just return success
    if (outputs.size() == 0 || m_featureType == FeatureType::Main)
    {
        return GameKit::GAMEKIT_SUCCESS;
    }

    bool newCloudFormationOutputValues = false;

    // Read feature-specific config settings
    YAML::Node paramsYml;
    // For the client config file, "not found" is expected, because it never exists before the first feature's successful deployment.
    if (!fs::exists(getClientConfigFilePath()))
    {
        // Log that new file will be created.
        std::string msg = std::string("Client Config file not found at ")
            .append(getClientConfigFilePath())
            .append(" . This is expected when you deploy your first GameKit feature. Creating a new one.");
        Logging::Log(m_logCb, Level::Info, msg.c_str());

        WriteEmptyClientConfiguration();

        // Set flag so empty file is created even if no config values to append.
        newCloudFormationOutputValues = true;
    }
    else
    {
        paramsYml = this->getClientConfigYaml();
    }

    auto configParams = this->getConfigOutputParameters();
    for (auto& param : configParams)
    {
        std::string paramKey = std::get<0>(param);
        std::string paramVal = std::get<1>(param);
        for (auto& output : outputs)
        {
            // replace occurrences of CloudFormation output vars AWSGAMEKIT::CFNOUTPUT::*
            std::regex key(TemplateVars::BEGIN + TemplateVars::AWS_GAMEKIT_CLOUDFORMATION_OUTPUT_PREFIX + ToStdString(output.GetOutputKey()) + TemplateVars::END);
            paramVal = std::regex_replace(paramVal, key, output.GetOutputValue());
        }

        std::string existingVal = paramsYml[paramKey].Scalar();
        if (existingVal != paramVal)
        {
            // replacement values in actual config
            paramsYml[paramKey] = paramVal;
            newCloudFormationOutputValues = true;
        }
    }

    // prevent unnecessary disk writes by making sure we actually have changes first
    if (!newCloudFormationOutputValues)
    {
        return GAMEKIT_SUCCESS;
    }

    return this->writeClientConfigYamlToDisk(paramsYml);
}

unsigned int GameKitFeatureResources::WriteEmptyClientConfiguration() const
{
    // Empty params since this should only be called when submitting an environment for the first time
    const YAML::Node paramsYml;
    return this->writeClientConfigYamlToDisk(paramsYml);
}

unsigned int GameKitFeatureResources::WriteClientConfiguration() const
{
    // Get stack Outputs
    const auto describeStackReq = CfnModel::DescribeStacksRequest().WithStackName(ToAwsString(m_stackName));
    auto outcome = m_cfClient->DescribeStacks(describeStackReq);
    if (!outcome.IsSuccess())
    {
        Logging::Log(m_logCb, Level::Error, outcome.GetError().GetMessage().c_str(), this);
        return GAMEKIT_ERROR_CLOUDFORMATION_DESCRIBE_STACKS_FAILED;
    }

    Aws::Vector<CfnModel::Output> outputs;
    if (outcome.GetResult().GetStacks().size() > 0)
    {
        outputs = outcome.GetResult().GetStacks().at(0).GetOutputs();
    }

    if (outputs.size() == 0)
    {
        // nothing to use for replacement; just return success
        return GameKit::GAMEKIT_SUCCESS;
    }

    return this->writeClientConfigurationWithOutputs(outputs);
}

unsigned int GameKitFeatureResources::DeleteFeatureStack()
{
    const auto describeStackReq = CfnModel::DescribeStacksRequest().WithStackName(ToAwsString(m_stackName));

    const auto outcome = m_cfClient->DescribeStacks(describeStackReq);
    unsigned int deleteResult = GAMEKIT_ERROR_CLOUDFORMATION_STACK_DELETE_FAILED;
    if (outcome.IsSuccess())
    {
        deleteResult = deleteStack();
    }

    if (deleteResult != GAMEKIT_SUCCESS)
    {
        return deleteResult;
    }

    char buffer[256];

    snprintf(buffer, 256, "Deleting stack resources for stack: %s", m_stackName.c_str());
    Logging::Log(m_logCb, Level::Info, buffer, this);
    const auto stackStatus = periodicallyDescribeStackEvents();

    // Deleted stacks do not show up in the DescribeStacks API (by stack name) if the deletion has been completed successfully,
    // so the last status could be DELETE_IN_PROGRESS for successfully deleted stacks.
    if (stackStatus != CfnModel::StackStatus::DELETE_COMPLETE && stackStatus != CfnModel::StackStatus::DELETE_IN_PROGRESS)
    {
        std::string msg = std::string("CloudFormation stack ").append(m_stackName).append(" deletion failed.");
        Logging::Log(m_logCb, Level::Error, msg.c_str(), this);
        return GameKit::GAMEKIT_ERROR_CLOUDFORMATION_STACK_DELETE_FAILED;
    }

    const auto writeResult = this->removeOutputsFromClientConfiguration();
    if (writeResult != GAMEKIT_SUCCESS)
    {
        std::string msg = std::string("Failed to delete output parameters from client configuration file for ").append(m_stackName);
        Logging::Log(m_logCb, Level::Warning, msg.c_str(), this);
    }

    return GameKit::GAMEKIT_SUCCESS;
}

std::string GameKitFeatureResources::GetStackName() const
{
    return getStackName(m_featureType);
}

std::string GameKitFeatureResources::getStackName(FeatureType featureType) const
{
    const std::string gameName = m_accountInfo.gameName;
    std::string stackName = "gamekit-";
    stackName.append(m_accountInfo.environment.GetEnvironmentString())
        .append("-")
        .append(gameName)
        .append("-")
        .append(GetFeatureTypeString(featureType));

    return stackName;
}

#pragma endregion

#pragma region Private/Helper Methods
Aws::Vector<CfnModel::Parameter> GameKitFeatureResources::getStackParameters(TemplateType templateType) const
{
    auto cfPath = m_baseCloudformationPath;
    if (templateType == TemplateType::Instance)
    {
        cfPath = m_instanceCloudformationPath;
    }    

    // extract user parameters for the current feature from settings file
    const GameKitSettings settings = GameKitSettings(m_gamekitRoot, "", m_accountInfo.gameName, m_accountInfo.environment.GetEnvironmentString(), m_logCb);
    const std::map<std::string, std::string> userParams = settings.GetFeatureVariables(m_featureType);

    // Replace all instances of AWSGAMEKIT::VARS::* values with user parameter values
    std::string rawParams = this->getRawStackParameters(templateType);
    for (const std::pair<std::string, std::string>& entry : userParams)
    {
        std::regex key(TemplateVars::BEGIN + TemplateVars::AWS_GAMEKIT_USERVAR_PREFIX + entry.first + TemplateVars::END);
        rawParams = std::regex_replace(rawParams, key, entry.second);
    }

    YAML::Node paramsYml = YAML::Load(rawParams);

    // read parameters and put them in a vector
    Aws::Vector<CfnModel::Parameter> params;
    for (YAML::const_iterator it = paramsYml.begin(); it != paramsYml.end(); ++it)
    {
        CfnModel::Parameter p;
        p.SetParameterKey(it->first.as<std::string>().c_str());
        p.SetParameterValue(it->second["value"].as<std::string>().c_str());
        params.push_back(p);
    }

    return params;
}

std::string GameKitFeatureResources::getRawStackParameters(TemplateType templateType) const
{
    auto cfPath = m_baseCloudformationPath;
    if (templateType == TemplateType::Instance)
    {
        cfPath = m_instanceCloudformationPath;
    }

    std::string loadedString;
    GameKit::Utils::FileUtils::ReadFileIntoString(cfPath + TemplateFileNames::PARAMETERS_FILE, loadedString);

    return loadedString;
}

std::string GameKitFeatureResources::getFeatureDashboardTemplate(TemplateType templateType) const
{
    auto cfPath = m_baseCloudformationPath;
    if (templateType == TemplateType::Instance)
    {
        cfPath = m_instanceCloudformationPath;
    }

    std::string loadedString;
    GameKit::Utils::FileUtils::ReadFileIntoString(cfPath + TemplateFileNames::FEATURE_DASHBOARD_FILE, loadedString);

    return loadedString;
}

std::string GameKitFeatureResources::getCloudFormationTemplate(TemplateType templateType) const
{
    auto cfPath = m_baseCloudformationPath;
    if (templateType == TemplateType::Instance)
    {
        cfPath = m_instanceCloudformationPath;
    }

    std::string loadedString;
    GameKit::Utils::FileUtils::ReadFileIntoString(cfPath + TemplateFileNames::CLOUDFORMATION_FILE, loadedString);

    return loadedString;
}

unsigned int GameKitFeatureResources::createStack() const
{
    char buffer[256];
    snprintf(buffer, 256, "Creating stack: %s", m_stackName.c_str());
    Logging::Log(m_logCb, Level::Info, buffer);

    auto createStackReq = CfnModel::CreateStackRequest();
    createStackReq.SetStackName(ToAwsString(m_stackName));
    createStackReq.SetTemplateBody(ToAwsString(this->getCloudFormationTemplate(TemplateType::Instance)));
    createStackReq.SetParameters(this->getStackParameters(TemplateType::Instance));
    createStackReq.AddCapabilities(CfnModel::Capability::CAPABILITY_IAM);
    createStackReq.AddCapabilities(CfnModel::Capability::CAPABILITY_NAMED_IAM);
    createStackReq.SetOnFailure(CfnModel::OnFailure::DELETE_);

    auto futureOutcome = m_cfClient->CreateStackCallable(createStackReq);
    futureOutcome.wait();

    unsigned int createStackResult = GAMEKIT_SUCCESS;
    auto outcome = futureOutcome.get();
    Level level = Level::Info;
    if (outcome.IsSuccess())
    {
        snprintf(buffer, 256, "CreateStack Successful; StackId: %s", outcome.GetResult().GetStackId().c_str());
    }
    else
    {
        snprintf(buffer, 256, "CreateStack Failed: %s", outcome.GetError().GetMessage().c_str());
        level = Level::Error;
        createStackResult = GAMEKIT_ERROR_CLOUDFORMATION_RESOURCE_CREATION_FAILED;
    }

    Logging::Log(m_logCb, level, buffer, this);
    return createStackResult;
}

unsigned int GameKitFeatureResources::updateStack() const
{
    char buffer[256];
    snprintf(buffer, 256, "Updating stack: %s", m_stackName.c_str());
    Logging::Log(m_logCb, Level::Info, buffer);

    auto updateStackReq = CfnModel::UpdateStackRequest();
    updateStackReq.SetStackName(ToAwsString(m_stackName));
    updateStackReq.SetTemplateBody(ToAwsString(this->getCloudFormationTemplate(TemplateType::Instance)));
    updateStackReq.SetParameters(this->getStackParameters(TemplateType::Instance));
    updateStackReq.AddCapabilities(CfnModel::Capability::CAPABILITY_IAM);
    updateStackReq.AddCapabilities(CfnModel::Capability::CAPABILITY_NAMED_IAM);

    auto futureOutcome = m_cfClient->UpdateStackCallable(updateStackReq);
    futureOutcome.wait();

    auto updateStackResult = GAMEKIT_SUCCESS;
    auto outcome = futureOutcome.get();
    enum Level level = Level::Info;
    if (outcome.IsSuccess())
    {
        snprintf(buffer, 256, "UpdateStack Successful; StackId: %s", outcome.GetResult().GetStackId().c_str());
    }
    else
    {
        snprintf(buffer, 256, "UpdateStack Failed: %s", outcome.GetError().GetMessage().c_str());
        level = Level::Error;

        // If the update failed because there are not CloudFormation changes, return GAMEKIT_SUCCESS
        auto updateError = outcome.GetError();
        if (updateError.GetErrorType() == Aws::CloudFormation::CloudFormationErrors::VALIDATION && updateError.GetMessage() == "No updates are to be performed.")
        {
            updateStackResult = GAMEKIT_SUCCESS;
        }
        else
        {
            Logging::Log(m_logCb, Level::Error, updateError.GetMessage().c_str());
            updateStackResult = GAMEKIT_ERROR_CLOUDFORMATION_STACK_UPDATE_FAILED;
        }
    }

    Logging::Log(m_logCb, level, buffer, this);
    return updateStackResult;
}

unsigned int GameKitFeatureResources::deleteStack() const
{
    char buffer[256];
    snprintf(buffer, 256, "Deleting stack: %s", m_stackName.c_str());
    Logging::Log(m_logCb, Level::Info, buffer);

    auto deleteStackReq = CfnModel::DeleteStackRequest();
    deleteStackReq.SetStackName(ToAwsString(m_stackName));

    auto futureOutcome = m_cfClient->DeleteStackCallable(deleteStackReq);
    futureOutcome.wait();

    auto deleteStackResult = GAMEKIT_SUCCESS;
    auto outcome = futureOutcome.get();
    enum Level level = Level::Info;
    if (outcome.IsSuccess())
    {
        snprintf(buffer, 256, "DeleteStack Started; StackName: %s", m_stackName.c_str());
    }
    else
    {
        snprintf(buffer, 256, "DeleteStack Failed: %s", outcome.GetError().GetMessage().c_str());
        level = Level::Error;
        deleteStackResult = GAMEKIT_ERROR_CLOUDFORMATION_STACK_DELETE_FAILED;
    }

    Logging::Log(m_logCb, level, buffer, this);
    return deleteStackResult;
}

CfnModel::StackStatus GameKitFeatureResources::periodicallyDescribeStackEvents()
{
    const auto describeStackReq = CfnModel::DescribeStacksRequest().WithStackName(ToAwsString(m_stackName));

    auto outcome = m_cfClient->DescribeStacks(describeStackReq);
    auto stackStatus = CfnModel::StackStatus::NOT_SET;
    if (outcome.GetResult().GetStacks().size() > 0)
    {
        stackStatus = outcome.GetResult().GetStacks().at(0).GetStackStatus();
    }
    // get first description of stack events (we may not even go into the loop below if process is already complete)
    describeStackEvents();

    while (outcome.IsSuccess() && !isTerminalState(stackStatus))
    {
        outcome = m_cfClient->DescribeStacks(describeStackReq);
        if (outcome.GetResult().GetStacks().size() > 0)
        {
            stackStatus = outcome.GetResult().GetStacks().at(0).GetStackStatus();
        }

        // get stack events
        describeStackEvents();

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return stackStatus;
}

void GameKitFeatureResources::describeStackEvents()
{
    auto describeStackEventsReq = CfnModel::DescribeStackEventsRequest();
    describeStackEventsReq.SetStackName(ToAwsString(m_stackName));

    auto futureOutcome = m_cfClient->DescribeStackEventsCallable(describeStackEventsReq);
    futureOutcome.wait();

    auto describeStackOutcome = futureOutcome.get();
    auto resourceStatus = CfnModel::ResourceStatusMapper::GetNameForResourceStatus(CfnModel::ResourceStatus::NOT_SET);
    auto events = describeStackOutcome.GetResult().GetStackEvents();
    if (events.size() > 0)
    {
        auto event = events.at(0);
        resourceStatus = CfnModel::ResourceStatusMapper::GetNameForResourceStatus(event.GetResourceStatus());
        Aws::String resourceId = event.GetLogicalResourceId();

        if (!m_resouceStatusMap[ToStdString(resourceId)])
        {
            m_resouceStatusMap[ToStdString(resourceId)] = true;
            char buffer[1024] = "";
            snprintf(buffer, 1024, "%s: %s | %s: %s", m_stackName.c_str(), resourceId.c_str(), resourceStatus.c_str(), event.GetResourceStatusReason().c_str());
            Logging::Log(m_logCb, Level::Info, buffer, this);
        }
    }
}

unsigned int GameKitFeatureResources::getDeployedTemplateBody(const std::string& stackName, std::string& templateBody) const
{
    const auto getTemplateRequest = CfnModel::GetTemplateRequest().WithStackName(ToAwsString(stackName));

    auto getTemplateOutcome = m_cfClient->GetTemplate(getTemplateRequest);
    if (!getTemplateOutcome.IsSuccess())
    {
        return GAMEKIT_ERROR_CLOUDFORMATION_GET_TEMPLATE_FAILED;
    }

    templateBody = getTemplateOutcome.GetResult().GetTemplateBody();
    return GAMEKIT_SUCCESS;
}


bool GameKitFeatureResources::isTerminalState(CfnModel::StackStatus status)
{
    return status == CfnModel::StackStatus::CREATE_FAILED ||
        status == CfnModel::StackStatus::CREATE_COMPLETE ||
        status == CfnModel::StackStatus::ROLLBACK_FAILED ||
        status == CfnModel::StackStatus::ROLLBACK_COMPLETE ||
        status == CfnModel::StackStatus::DELETE_FAILED ||
        status == CfnModel::StackStatus::DELETE_COMPLETE ||
        status == CfnModel::StackStatus::UPDATE_COMPLETE ||
        status == CfnModel::StackStatus::UPDATE_ROLLBACK_FAILED ||
        status == CfnModel::StackStatus::UPDATE_ROLLBACK_COMPLETE ||
        status == CfnModel::StackStatus::IMPORT_COMPLETE ||
        status == CfnModel::StackStatus::IMPORT_ROLLBACK_FAILED ||
        status == CfnModel::StackStatus::IMPORT_ROLLBACK_COMPLETE;
}

bool GameKitFeatureResources::isFailedState(CfnModel::StackStatus status)
{
    return status == CfnModel::StackStatus::CREATE_FAILED ||
        status == CfnModel::StackStatus::ROLLBACK_FAILED ||
        status == CfnModel::StackStatus::DELETE_FAILED ||
        status == CfnModel::StackStatus::UPDATE_ROLLBACK_FAILED ||
        status == CfnModel::StackStatus::IMPORT_ROLLBACK_FAILED;
}

std::string GameKitFeatureResources::getTempLayersPath() const
{
    return fs::temp_directory_path()
        .append("gamekit_layers")
        .append(m_layersReplacementId)
        .append(GameKit::GetFeatureTypeString(m_featureType)).string();
}

std::string GameKitFeatureResources::getTempFunctionsPath() const
{
    return fs::temp_directory_path()
        .append("gamekit_functions")
        .append(m_functionsReplacementId)
        .append(GameKit::GetFeatureTypeString(m_featureType)).string();
}

YAML::Node GameKitFeatureResources::getClientConfigYaml() const
{
    YAML::Node node;
    Utils::FileUtils::ReadFileAsYAML(getClientConfigFilePath(), node, m_logCb);
    return node;
}

std::vector<std::tuple<std::string, std::string>> GameKitFeatureResources::getConfigOutputParameters() const
{
    std::vector<std::tuple<std::string, std::string>> params;
    const auto configPath = m_baseConfigOutputsPath + TemplateFileNames::FEATURE_CLIENT_CONFIGURATION_FILE;
    YAML::Node paramsYml;
    Utils::FileUtils::ReadFileAsYAML(configPath, paramsYml, m_logCb);
    for (YAML::const_iterator it = paramsYml.begin(); it != paramsYml.end(); ++it)
    {
        std::tuple<std::string, std::string> t = std::make_tuple(it->first.as<std::string>(), it->second.as<std::string>());
        params.push_back(t);
    }
    return params;
}

unsigned int GameKitFeatureResources::writeCloudFormationParameterInstance(const std::string& cfParams) const
{
    std::string logMsg;
    boost::filesystem::create_directories(m_instanceCloudformationPath);
    const auto writeResult = GameKit::Utils::FileUtils::WriteStringToFile(cfParams, m_instanceCloudformationPath + TemplateFileNames::PARAMETERS_FILE, m_logCb);
    if (writeResult != GAMEKIT_SUCCESS)
    {
        logMsg = std::string("Failed to saved parameters file to ").append(m_instanceCloudformationPath);
        Logging::Log(m_logCb, Level::Error, logMsg.c_str(), this);
        return GAMEKIT_ERROR_PARAMETERS_FILE_SAVE_FAILED;
    }

    logMsg = std::string("Parameters file saved to ").append(m_instanceCloudformationPath);
    Logging::Log(m_logCb, Level::Info, logMsg.c_str(), this);

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitFeatureResources::writeCloudFormationTemplateInstance(const std::string& cfTemplate) const
{
    std::string logMsg;
    boost::filesystem::create_directories(m_instanceCloudformationPath);
    const auto writeResult = GameKit::Utils::FileUtils::WriteStringToFile(cfTemplate, m_instanceCloudformationPath + TemplateFileNames::CLOUDFORMATION_FILE, m_logCb);
    if (writeResult != GAMEKIT_SUCCESS)
    {
        logMsg = std::string("Failed to saved CloudFormation file to ").append(m_instanceCloudformationPath);
        Logging::Log(m_logCb, Level::Error, logMsg.c_str(), this);
        return GAMEKIT_ERROR_CLOUDFORMATION_FILE_SAVE_FAILED;
    }

    logMsg = std::string("CloudFormation file saved to ").append(m_instanceCloudformationPath);
    Logging::Log(m_logCb, Level::Info, logMsg.c_str(), this);

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitFeatureResources::writeCloudFormationDashboardInstance(const std::string& cfDashboard) const
{
    std::string logMsg;
    boost::filesystem::create_directories(m_instanceCloudformationPath);
    auto writeResult = GameKit::Utils::FileUtils::WriteStringToFile(cfDashboard, m_instanceCloudformationPath + TemplateFileNames::FEATURE_DASHBOARD_FILE, m_logCb);
    if (writeResult != GAMEKIT_SUCCESS)
    {
        logMsg = std::string("Failed to saved CloudFormation Dashboard file to ").append(m_instanceCloudformationPath);
        Logging::Log(m_logCb, Level::Error, logMsg.c_str(), this);
        return GAMEKIT_ERROR_CLOUDFORMATION_FILE_SAVE_FAILED;
    }

    logMsg = std::string("CloudFormation Dashboard file saved to ").append(m_instanceCloudformationPath);
    Logging::Log(m_logCb, Level::Info, logMsg.c_str(), this);

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitFeatureResources::ConditionallyCreateOrUpdateFeatureResources(FeatureType targetFeature, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback) 
{
    unsigned int result = GAMEKIT_SUCCESS;
    FeatureStatus stackStatus = GetFeatureStatusFromCloudFormationStackStatus(GetCurrentStackStatus());

    if (stackStatus == FeatureStatus::Running)
    {
        if (dispatchReceiver != nullptr && responseCallback != nullptr)
        {
            const std::string output = "The AWS resources for this game feature are currently being updated by another user.";
            responseCallback(dispatchReceiver, output.c_str());
        }
        return GAMEKIT_SUCCESS;
    }

    if (stackStatus == FeatureStatus::Undeployed)
    {
        if (boost::filesystem::exists(m_instanceLayersPath))
        {
            std::string logMsg = std::string("Using existing Lambda layer instance files.");
            Logging::Log(m_logCb, Level::Info, logMsg.c_str(), this);
        }
        else
        {
            result = SaveLayerInstances();
            if (result != GAMEKIT_SUCCESS)
            {
                if (dispatchReceiver != nullptr && responseCallback != nullptr)
                {
                    const std::string output = "Unable to retrieve deployed Lambda Layers for feature";
                    responseCallback(dispatchReceiver, output.c_str());
                }
                return result;
            }
        }

        if (boost::filesystem::exists(m_instanceFunctionsPath))
        {
            std::string logMsg = std::string("Using existing Lambda Function instance files.");
            Logging::Log(m_logCb, Level::Info, logMsg.c_str(), this);
        }
        else
        {
            result = SaveFunctionInstances();
            if (result != GAMEKIT_SUCCESS)
            {
                if (dispatchReceiver != nullptr && responseCallback != nullptr)
                {
                    const std::string output = "Unable to retrieve deployed Lambda Function for feature";
                    responseCallback(dispatchReceiver, output.c_str());
                }
                return result;
            }
        }
    }

    if (!IsCloudFormationInstanceTemplatePresent()) {
        result = SaveDeployedCloudFormationTemplate();
        if (result != GAMEKIT_SUCCESS)
        {
            if (dispatchReceiver != nullptr && responseCallback != nullptr)
            {
                const std::string output = "Unable to retrieve deployed CloudFormation template for feature";
                responseCallback(dispatchReceiver, output.c_str());
            }
            return result;
        }
    }

    GameKit::GameKitAccount gamekitAccount(m_accountInfo, m_credentials, m_logCb);
    gamekitAccount.SetPluginRoot(m_pluginRoot);
    gamekitAccount.SetGameKitRoot(m_gamekitRoot);
    gamekitAccount.InitializeDefaultAwsClients();
 
    result = gamekitAccount.UploadDashboards();
    if (result != GAMEKIT_SUCCESS)
    {
        if (dispatchReceiver != nullptr && responseCallback != nullptr)
        {
            const std::string output = "Failed to upload Dashboard";
            responseCallback(dispatchReceiver, output.c_str());
        }
        return result;
    }

    result = UploadFeatureLayers();
    if (result != GAMEKIT_SUCCESS)
    {
        if (dispatchReceiver != nullptr && responseCallback != nullptr)
        {
            const std::string output = "Failed to upload feature layers";
            responseCallback(dispatchReceiver, output.c_str());
        }
        return result;
    }

    result = UploadFeatureFunctions();
    if (result != GAMEKIT_SUCCESS)
    {
        if (dispatchReceiver != nullptr && responseCallback != nullptr)
        {
            const std::string output = "Failed to upload feature functions";
            responseCallback(dispatchReceiver, output.c_str());
        }
        return result;
    }

    result = CreateOrUpdateFeatureStack();
    if (result != GAMEKIT_SUCCESS)
    {
        if (dispatchReceiver != nullptr && responseCallback != nullptr)
        {
            const std::string output = "Failed to create feature stack";
            responseCallback(dispatchReceiver, output.c_str());
        }
        return result;
    }

    result = gamekitAccount.DeployApiGatewayStage();
    if (result != GAMEKIT_SUCCESS)
    {
        if (dispatchReceiver != nullptr && responseCallback != nullptr)
        {
            const std::string output = "Failed to deploy API Gateway";
            responseCallback(dispatchReceiver, output.c_str());
        }
        return result;
    }

    return result;
}

#pragma endregion
