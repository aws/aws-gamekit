// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/core/gamekit_account.h>
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>
#include <aws/gamekit/core/utils/file_utils.h>

using namespace GameKit;
using namespace GameKit::Logger;

namespace CfnModel = Aws::CloudFormation::Model;
namespace S3Model = Aws::S3::Model;
namespace SSMModel = Aws::SSM::Model;
namespace SecretsModel = Aws::SecretsManager::Model;
namespace ApiGwyModel = Aws::APIGateway::Model;
namespace fs = boost::filesystem;

#pragma region Constructor/Desctructor
GameKitAccount::GameKitAccount(const AccountInfo& accountInfo, const AccountCredentials& credentials, FuncLogCallback logCallback)
{
    m_accountInfo = CreateAccountInfoCopy(accountInfo);
    m_credentials = CreateAccountCredentialsCopy(credentials);
    m_credentials.accountId = accountInfo.accountId;
    m_logCb = logCallback;

    GameKit::AwsApiInitializer::Initialize(m_logCb, this);
    Logging::Log(m_logCb, Level::Info, "GameKitAccount instantiated", this);
}

GameKitAccount::GameKitAccount(const AccountInfoCopy& accountInfo, const AccountCredentialsCopy& credentials, FuncLogCallback logCallback)
{
    m_accountInfo = accountInfo;
    m_credentials = credentials;
    m_credentials.accountId = accountInfo.accountId;
    m_logCb = logCallback;

    GameKit::AwsApiInitializer::Initialize(m_logCb, this);
    Logging::Log(m_logCb, Level::Info, "GameKitAccount instantiated", this);
}

GameKitAccount::~GameKitAccount()
{
    if (m_deleteClients)
    {
        this->DeleteClients();
    }
    GameKit::AwsApiInitializer::Shutdown(m_logCb, this);
    Logging::Log(m_logCb, Level::Info, "~GameKitAccount()", this);
}
#pragma endregion

#pragma region Public Methods
void GameKitAccount::InitializeDefaultAwsClients()
{
    this->DeleteClientsOnDestruction();

    SetSSMClient(GameKit::DefaultClients::GetDefaultSSMClient(this->GetAccountCredentials()));
    SetS3Client(GameKit::DefaultClients::GetDefaultS3Client(this->GetAccountCredentials()));
    SetCloudFormationClient(GameKit::DefaultClients::GetDefaultCloudFormationClient(this->GetAccountCredentials()));
    SetSecretsManagerClient(GameKit::DefaultClients::GetDefaultSecretsManagerClient(this->GetAccountCredentials()));
    SetApiGatewayClient(GameKit::DefaultClients::GetDefaultApiGatewayClient(this->GetAccountCredentials()));
    SetLambdaClient(GameKit::DefaultClients::GetDefaultLambdaClient(this->GetAccountCredentials()));
}

void GameKitAccount::DeleteClients()
{
    delete(m_ssmClient);
    delete(m_s3Client);
    delete(m_cfnClient);
    delete(m_secretsClient);
    delete(m_apigwyClient);
}

bool GameKitAccount::HasBootstrapBucket()
{
    return hasBootstrapBucket(GetBootstrapBucketName(m_accountInfo, getShortRegionCode()));
}

unsigned int GameKitAccount::Bootstrap()
{
    // If region name cannot be converted to short region code, return an error (all s3 buckets use 5-letter short region codes)
    const std::string shortRegionCode = getShortRegionCode();
    if (shortRegionCode.empty())
    {
        return GameKit::GAMEKIT_ERROR_BOOTSTRAP_REGION_CODE_CONVERSION_FAILED;
    }
    std::string bootstrapBucketName = GetBootstrapBucketName(m_accountInfo, shortRegionCode);

    // The bootstrap bucket doesn't exist; create it
    if (!hasBootstrapBucket(bootstrapBucketName))
    {
        auto createBucketRequest = S3Model::CreateBucketRequest().WithBucket(ToAwsString(bootstrapBucketName));

        // get requested region
        std::string region(m_credentials.region);
        auto bucketLocationConstraint = S3Model::BucketLocationConstraintMapper::GetBucketLocationConstraintForName(ToAwsString(region));

        // Buckets are created on us-east-1 by default and there's no need to specify this constraint
        // if that's the passed region. Otherwise, we need to set the location constraint
        if (bucketLocationConstraint != S3Model::BucketLocationConstraint::us_east_1)
        {
            S3Model::CreateBucketConfiguration bucketConfig;

            bucketConfig.SetLocationConstraint(bucketLocationConstraint);
            createBucketRequest.SetCreateBucketConfiguration(bucketConfig);
        }

        S3Model::CreateBucketOutcome createOutcome = m_s3Client->CreateBucket(createBucketRequest);
        if (!createOutcome.IsSuccess())
        {
            Logging::Log(m_logCb, Level::Error, createOutcome.GetError().GetMessage().c_str());

            if (createOutcome.GetError().GetExceptionName() == TOO_MANY_BUCKETS_EXCEPTION_NAME)
            {
                // Bucket creation failed because the AWS account has too many buckets. To fix this S3 buckets can be deleted or
                // the bucket limit can be increased. See https://docs.aws.amazon.com/AmazonS3/latest/userguide/BucketRestrictions.html.
                return GAMEKIT_ERROR_BOOTSTRAP_TOO_MANY_BUCKETS;
            }

            // Bucket creation failed for an unknown reason
            return GAMEKIT_ERROR_BOOTSTRAP_BUCKET_CREATION_FAILED;
        }

        // bucket successfully created; log it's location
        Logging::Log(m_logCb, Level::Info, createOutcome.GetResult().GetLocation().c_str());

        // set lifecycle policy on bucket
        auto lifeCycleConfig = S3Model::BucketLifecycleConfiguration()
            .WithRules(Aws::Vector<S3Model::LifecycleRule>{
                S3Model::LifecycleRule()
                .WithFilter(S3Model::LifecycleRuleFilter().WithPrefix("functions/"))
                .WithExpiration(S3Model::LifecycleExpiration().WithDays(1))
                .WithStatus(S3Model::ExpirationStatus::Enabled),
                S3Model::LifecycleRule()
                .WithFilter(S3Model::LifecycleRuleFilter().WithPrefix("layers/"))
                .WithExpiration(S3Model::LifecycleExpiration().WithDays(1))
                .WithStatus(S3Model::ExpirationStatus::Enabled),
                S3Model::LifecycleRule()
                .WithFilter(S3Model::LifecycleRuleFilter().WithPrefix("cloudformation/"))
                .WithExpiration(S3Model::LifecycleExpiration().WithDays(1))
                .WithStatus(S3Model::ExpirationStatus::Enabled),
                S3Model::LifecycleRule()
                .WithFilter(S3Model::LifecycleRuleFilter().WithPrefix("cb_completions/"))
                .WithExpiration(S3Model::LifecycleExpiration().WithDays(1))
                .WithStatus(S3Model::ExpirationStatus::Enabled),
                S3Model::LifecycleRule()
                .WithFilter(S3Model::LifecycleRuleFilter().WithPrefix("cb_tokens/"))
                .WithExpiration(S3Model::LifecycleExpiration().WithDays(1))
                .WithStatus(S3Model::ExpirationStatus::Enabled)
        });

        auto lifeCycleRequest = S3Model::PutBucketLifecycleConfigurationRequest()
            .WithBucket(ToAwsString(bootstrapBucketName))
            .WithLifecycleConfiguration(lifeCycleConfig);

        auto lifeCycleOutcome = m_s3Client->PutBucketLifecycleConfiguration(lifeCycleRequest);
        if (!lifeCycleOutcome.IsSuccess())
        {
            Logging::Log(m_logCb, Level::Error, lifeCycleOutcome.GetError().GetMessage().c_str());
        }
    }

    return GameKit::GAMEKIT_SUCCESS;
}

std::string GameKitAccount::composeSecretId(const std::string& secretName)
{
    // compose secretid
    std::string secretId = "gamekit_";
    secretId.append(m_accountInfo.environment.GetEnvironmentString())
        .append("_")
        .append(m_accountInfo.gameName)
        .append("_")
        .append(secretName);

    return secretId;
}

unsigned int GameKitAccount::CheckSecretExists(const std::string& secretName)
{
    std::string secretId = this->composeSecretId(secretName);

    SecretsModel::DescribeSecretRequest describeRequest;
    describeRequest.SetSecretId(secretId.c_str());
    auto const describeOutcome = m_secretsClient->DescribeSecret(describeRequest);

    if (!describeOutcome.IsSuccess())
    {
        return GAMEKIT_WARNING_SECRETSMANAGER_SECRET_NOT_FOUND;
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitAccount::SaveSecret(const std::string& secretName, const std::string& secretValue)
{
    std::string const secretId = this->composeSecretId(secretName);

    // check if secret exists
    auto const checkSecretOutcome = CheckSecretExists(secretName);

    if (checkSecretOutcome == GAMEKIT_WARNING_SECRETSMANAGER_SECRET_NOT_FOUND)
    {
        return this->createSecret(secretId, secretValue);
    }

    return this->updateSecret(secretId, secretValue);
}

unsigned int GameKitAccount::DeleteSecret(const std::string& secretName)
{
    std::string const secretId = this->composeSecretId(secretName);

    // check if secret exists
    auto const checkSecretOutcome = CheckSecretExists(secretName);

    // Fail-Safe, if we don't have an existing secret for this, return SUCCESS
    if (checkSecretOutcome == GAMEKIT_WARNING_SECRETSMANAGER_SECRET_NOT_FOUND)
    {
        return GAMEKIT_SUCCESS;
    }

    return this->deleteSecret(secretId);
}

unsigned int GameKitAccount::SaveFeatureInstanceTemplates()
{
    if (!isFunctionsPathValid(TemplateType::Base))
    {
        return GAMEKIT_ERROR_FUNCTIONS_PATH_NOT_FOUND;
    }

    fs::path p(m_baseCloudformationPath);
    fs::directory_iterator end_iter;
    for (fs::directory_iterator iter(p); iter != end_iter; ++iter)
    {
        // get feature name from directory
        const fs::path cp = (*iter);
        std::string featureName = cp.stem().string();

        Aws::UniquePtr<GameKitFeatureResources> featureResources = Aws::MakeUnique<GameKitFeatureResources>(
            featureName.c_str(),
            m_accountInfo,
            m_credentials,
            GameKit::GetFeatureTypeFromString(featureName),
            m_logCb);

        featureResources->SetPluginRoot(m_pluginRoot);
        featureResources->SetGameKitRoot(m_gamekitRoot);

        auto result = featureResources->SaveCloudFormationInstance();
        if (result != GAMEKIT_SUCCESS)
        {
            return result;
        }

        result = featureResources->SaveLayerInstances();
        if (result != GAMEKIT_SUCCESS)
        {
            return result;
        }

        result = featureResources->SaveFunctionInstances();
        if (result != GAMEKIT_SUCCESS)
        {
            return result;
        }
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKit::GameKitAccount::UploadDashboards()
{
    if (!isCloudFormationPathValid(TemplateType::Instance))
    {
        return GAMEKIT_ERROR_CLOUDFORMATION_PATH_NOT_FOUND;
    }

    // loop through the feature directories
    fs::path p(m_instanceCloudformationPath);
    fs::directory_iterator end_iter;
    for (fs::directory_iterator iter(p); iter != end_iter; ++iter)
    {
        // get feature name from directory
        const fs::path cp = (*iter);
        std::string featureName = cp.stem().string();

        // instantiate a feature resource
        Aws::UniquePtr<GameKitFeatureResources> featureResources = Aws::MakeUnique<GameKitFeatureResources>(
            featureName.c_str(),
            m_accountInfo,
            m_credentials,
            GameKit::GetFeatureTypeFromString(featureName),
            m_logCb);

        // set base and instance paths
        featureResources->SetPluginRoot(m_pluginRoot);
        featureResources->SetGameKitRoot(m_gamekitRoot);

        // set AWS clients and bucket
        featureResources->SetS3Client(m_s3Client, true);
        featureResources->SetSSMClient(m_ssmClient, true);

        // upload feature dashboard
        auto uploadResult = featureResources->UploadDashboard(cp.string());
        if (uploadResult != GAMEKIT_SUCCESS)
        {
            return uploadResult;
        }
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKit::GameKitAccount::UploadLayers()
{
    if (!isLayersPathValid(TemplateType::Instance))
    {
        return GAMEKIT_ERROR_LAYERS_PATH_NOT_FOUND;
    }

    // loop through the feature directories
    fs::path p(m_instanceLayersPath);
    fs::directory_iterator end_iter;
    for (fs::directory_iterator iter(p); iter != end_iter; ++iter)
    {
        // get feature name from directory
        const fs::path cp = (*iter);
        std::string featureName = cp.stem().string();

        // instantiate a feature resource
        Aws::UniquePtr<GameKitFeatureResources> featureResources = Aws::MakeUnique<GameKitFeatureResources>(
            featureName.c_str(),
            m_accountInfo,
            m_credentials,
            GameKit::GetFeatureTypeFromString(featureName),
            m_logCb);

        // set base and instance paths
        featureResources->SetPluginRoot(m_pluginRoot);
        featureResources->SetGameKitRoot(m_gamekitRoot);

        // set AWS clients and bucket
        featureResources->SetS3Client(m_s3Client, true);
        featureResources->SetSSMClient(m_ssmClient, true);

        // create/set replacement id
        featureResources->CreateAndSetLayersReplacementId();

        // compress feature functions
        auto compressResult = featureResources->CompressFeatureLayers();
        if (compressResult != GAMEKIT_SUCCESS)
        {
            return compressResult;
        }

        // upload feature layers
        auto uploadResult = featureResources->UploadFeatureLayers();
        if (uploadResult != GAMEKIT_SUCCESS)
        {
            return uploadResult;
        }

        // cleanup
        featureResources->CleanupTempFiles();
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitAccount::UploadFunctions()
{
    if (!isFunctionsPathValid(TemplateType::Instance))
    {
        return GAMEKIT_ERROR_FUNCTIONS_PATH_NOT_FOUND;
    }

    // loop through the feature directories
    fs::path p(m_instanceFunctionsPath);
    fs::directory_iterator end_iter;
    for (fs::directory_iterator iter(p); iter != end_iter; ++iter)
    {
        // get feature name from directory
        const fs::path cp = (*iter);
        std::string featureName = cp.stem().string();

        // instantiate a feature resource
        Aws::UniquePtr<GameKitFeatureResources> featureResources = Aws::MakeUnique<GameKitFeatureResources>(
            featureName.c_str(),
            m_accountInfo,
            m_credentials,
            GameKit::GetFeatureTypeFromString(featureName),
            m_logCb);

        // set base and instance paths
        featureResources->SetPluginRoot(m_pluginRoot);
        featureResources->SetGameKitRoot(m_gamekitRoot);

        // set AWS clients and bucket
        featureResources->SetS3Client(m_s3Client, true);
        featureResources->SetSSMClient(m_ssmClient, true);

        // create/set replacement id
        featureResources->CreateAndSetFunctionsReplacementId();

        // compress feature functions
        auto compressResult = featureResources->CompressFeatureFunctions();
        if (compressResult != GAMEKIT_SUCCESS)
        {
            return compressResult;
        }

        // upload feature functions
        auto uploadResult = featureResources->UploadFeatureFunctions();
        if (uploadResult != GAMEKIT_SUCCESS)
        {
            return uploadResult;
        }

        // cleanup
        featureResources->CleanupTempFiles();
    }

    return GAMEKIT_SUCCESS;
}

bool GameKitAccount::HasValidCredentials()
{
    if (m_credentials.accessSecret.empty() || m_credentials.accessKey.empty())
    {
        return false;
    }

    // if credentials are allowed to list S3 buckets, it's valid
    S3Model::ListBucketsOutcome listOutcome = m_s3Client->ListBuckets();
    if (!listOutcome.IsSuccess())
    {
        Logging::Log(m_logCb, Level::Error, listOutcome.GetError().GetMessage().c_str());
    }

    return listOutcome.IsSuccess();
}

unsigned int GameKitAccount::CreateOrUpdateStacks()
{
    std::shared_ptr<GameKitFeatureResources> featureResources;

    //compress and upload functions
    auto result = this->UploadFunctions();
    if (result != GAMEKIT_SUCCESS)
    {
        return result;
    }

    // read main stack directory and execute create/update
    result = this->CreateOrUpdateMainStack();
    if (result != GAMEKIT_SUCCESS)
    {
        return result;
    }

    // read feature stack directories and execute create/update
    result = this->CreateOrUpdateFeatureStacks();
    if (result != GAMEKIT_SUCCESS)
    {
        return result;
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitAccount::DeployApiGatewayStage()
{
    // instantiate Main FeatureResource
    Aws::UniquePtr<GameKitFeatureResources> mainResources = Aws::MakeUnique<GameKitFeatureResources>(
        GameKit::GetFeatureTypeString(FeatureType::Main).c_str(),
        m_accountInfo,
        m_credentials,
        FeatureType::Main,
        nullptr);

    // get RestApi id from main stack
    CfnModel::DescribeStackResourceRequest describeRequest;
    describeRequest.SetLogicalResourceId("RestApi");
    describeRequest.SetStackName(ToAwsString(mainResources->GetStackName()));
    auto describeOutcome = m_cfnClient->DescribeStackResource(describeRequest);
    if (!describeOutcome.IsSuccess())
    {
        return GAMEKIT_ERROR_CLOUDFORMATION_DESCRIBE_RESOURCE_FAILED;
    }

    //  create a deployment
    auto restApiId = describeOutcome.GetResult().GetStackResourceDetail().GetPhysicalResourceId();
    ApiGwyModel::CreateDeploymentRequest createDeploymentRequest;
    createDeploymentRequest.SetRestApiId(restApiId);
    auto createDeploymentOutcome = m_apigwyClient->CreateDeployment(createDeploymentRequest);
    if (!createDeploymentOutcome.IsSuccess())
    {
        return GAMEKIT_ERROR_APIGATEWAY_DEPLOYMENT_CREATION_FAILED;
    }

    // create patch operation for stage deployment
    ApiGwyModel::PatchOperation patchOp;
    patchOp.SetPath("/deploymentId");
    patchOp.SetValue(createDeploymentOutcome.GetResult().GetId());
    patchOp.SetOp(ApiGwyModel::Op::replace);

    // update the stage with the new deployment
    ApiGwyModel::UpdateStageRequest updateRequest;
    updateRequest.SetRestApiId(restApiId);
    updateRequest.SetStageName(ToAwsString(m_accountInfo.environment.GetEnvironmentString()));
    updateRequest.AddPatchOperations(patchOp);
    auto updateStageOutcome = m_apigwyClient->UpdateStage(updateRequest);
    if (!updateStageOutcome.IsSuccess())
    {
        return GAMEKIT_ERROR_APIGATEWAY_STAGE_DEPLOYMENT_FAILED;
    }

    return GAMEKIT_SUCCESS;
}
#pragma endregion

#pragma region Private/Helper Methods
bool GameKitAccount::isLayersPathValid(TemplateType templateType)
{
    auto layersPath = m_baseLayersPath;
    if (templateType == TemplateType::Instance)
    {
        layersPath = m_instanceLayersPath;
    }

    return fs::exists(layersPath) && fs::is_directory(layersPath);
}

bool GameKitAccount::isFunctionsPathValid(TemplateType templateType)
{
    auto funcsPath = m_baseFunctionsPath;
    if (templateType == TemplateType::Instance)
    {
        funcsPath = m_instanceFunctionsPath;
    }
    
    bool funcsPathExists = fs::exists(funcsPath);
    bool funcsPathIsDir = fs::is_directory(funcsPath);

    std::stringstream ss;
    ss << funcsPath << " exists: " << funcsPathExists << ", is dir: " << funcsPathIsDir;
    Logger::Logging::Log(m_logCb, Level::Verbose, ss.str().c_str());
    return funcsPathExists && funcsPathIsDir;
}

bool GameKitAccount::isCloudFormationPathValid(TemplateType templateType)
{
    auto cfPath = m_baseCloudformationPath;
    if (templateType == TemplateType::Instance)
    {
        cfPath = m_instanceCloudformationPath;
    }

    return fs::exists(cfPath) && fs::is_directory(cfPath);
}

bool GameKitAccount::hasBootstrapBucket(const std::string& bootstrapBucketName)
{
    // Get all buckets
    S3Model::ListBucketsOutcome listOutcome = m_s3Client->ListBuckets();
    if (!listOutcome.IsSuccess())
    {
        // lookup failed
        Logging::Log(m_logCb, Level::Error, listOutcome.GetError().GetMessage().c_str());
        return false;
    }

    // Loop through returned buckets and check for bootstrapBucketName
    auto buckets = listOutcome.GetResult().GetBuckets();
    bool bootstrapBucketFound = false;
    S3Model::Bucket bootstrapBucket;
    for (S3Model::Bucket& bucket : buckets)
    {
        if (bucket.GetName().c_str() == bootstrapBucketName)
        {
            bootstrapBucket = bucket;
            bootstrapBucketFound = true;
            break;
        }
    }

    return bootstrapBucketFound;
}

unsigned int GameKitAccount::CreateOrUpdateMainStack()
{
    if (!isCloudFormationPathValid(TemplateType::Instance))
    {
        return GAMEKIT_ERROR_CLOUDFORMATION_PATH_NOT_FOUND;
    }

    std::string mainFeatureName = GameKit::GetFeatureTypeString(FeatureType::Main);
    std::string mainCloudFormationPath = m_instanceCloudformationPath + "/" + mainFeatureName;

    // instantiate a feature resource
    Aws::UniquePtr<GameKitFeatureResources> mainResources = Aws::MakeUnique<GameKitFeatureResources>(
        mainFeatureName.c_str(),
        m_accountInfo,
        m_credentials,
        FeatureType::Main,
        m_logCb);

    // set paths
    mainResources->SetCloudFormationClient(m_cfnClient, true);
    mainResources->SetLambdaClient(m_lambdaClient, true);
    mainResources->SetPluginRoot(m_pluginRoot);
    mainResources->SetGameKitRoot(m_gamekitRoot);

    // create/update main stack
    return mainResources->CreateOrUpdateFeatureStack();
}

unsigned int GameKitAccount::CreateOrUpdateFeatureStacks()
{
    if (!isCloudFormationPathValid(TemplateType::Instance))
    {
        return GAMEKIT_ERROR_FUNCTIONS_PATH_NOT_FOUND;
    }

    auto result = GAMEKIT_SUCCESS;
    fs::path p(m_instanceCloudformationPath);
    fs::directory_iterator end_iter;
    for (fs::directory_iterator iter(p); iter != end_iter; ++iter)
    {
        const fs::path cp = (*iter);
        std::string featureName = cp.stem().string();

        // skip main stack
        if (featureName == GameKit::GetFeatureTypeString(FeatureType::Main))
        {
            continue;
        }

        std::shared_ptr<GameKitFeatureResources> featureResources = Aws::MakeShared<GameKitFeatureResources>(
            featureName.c_str(),
            m_accountInfo,
            m_credentials,
            GameKit::GetFeatureTypeFromString(featureName),
            m_logCb);

        // set paths
        featureResources->SetCloudFormationClient(m_cfnClient, true);
        featureResources->SetLambdaClient(m_lambdaClient, true);
        featureResources->SetPluginRoot(m_pluginRoot);
        featureResources->SetGameKitRoot(m_gamekitRoot);

        // create/update feature stacks
        result = featureResources->CreateOrUpdateFeatureStack();

        if (result != GAMEKIT_SUCCESS)
        {
            break;
        }
    }

    return result;
}

unsigned int GameKitAccount::createSecret(const std::string& secretId, const std::string& secretValue)
{
    SecretsModel::CreateSecretRequest createRequest;
    createRequest.WithName(secretId.c_str());
    createRequest.SetSecretString(secretValue.c_str());
    auto createOutcome = m_secretsClient->CreateSecret(createRequest);

    if (!createOutcome.IsSuccess())
    {
        Logging::Log(m_logCb, Level::Error, createOutcome.GetError().GetMessage().c_str());
        return GAMEKIT_ERROR_SECRETSMANAGER_WRITE_FAILED;
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitAccount::updateSecret(const std::string& secretId, const std::string& secretValue)
{
    SecretsModel::UpdateSecretRequest updateRequest;
    updateRequest.SetSecretId(secretId.c_str());
    updateRequest.SetSecretString(secretValue.c_str());
    auto upddateOutcome = m_secretsClient->UpdateSecret(updateRequest);

    if (!upddateOutcome.IsSuccess())
    {
        Logging::Log(m_logCb, Level::Error, upddateOutcome.GetError().GetMessage().c_str());
        return GAMEKIT_ERROR_SECRETSMANAGER_WRITE_FAILED;
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitAccount::deleteSecret(const std::string& secretId)
{
    SecretsModel::DeleteSecretRequest deleteRequest;
    deleteRequest.SetSecretId(secretId.c_str());
    auto deleteOutcome = m_secretsClient->DeleteSecret(deleteRequest);

    if (!deleteOutcome.IsSuccess())
    {
        Logging::Log(m_logCb, Level::Error, deleteOutcome.GetError().GetMessage().c_str());
        return GAMEKIT_ERROR_SECRETSMANAGER_WRITE_FAILED;
    }

    return GAMEKIT_SUCCESS;
}

std::string GameKitAccount::getShortRegionCode()
{
    if (GetPluginRoot().empty())
    {
        return "";
    }
    AwsRegionMappings& regionMappings = AwsRegionMappings::getInstance(GetPluginRoot(), m_logCb);
    return regionMappings.getFiveLetterRegionCode(std::string(m_credentials.region));
}
#pragma endregion
