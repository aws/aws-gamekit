// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <fstream>
#include <iostream>
#include <regex>

// AWS SDK
#include <aws/core/utils/base64/Base64.h>
#include <aws/cloudformation/CloudFormationClient.h>
#include <aws/cloudformation/model/CreateStackRequest.h>
#include <aws/cloudformation/model/DeleteStackRequest.h>
#include <aws/cloudformation/model/DescribeStackEventsRequest.h>
#include <aws/cloudformation/model/DescribeStackResourcesRequest.h>
#include <aws/cloudformation/model/DescribeStacksRequest.h>
#include <aws/cloudformation/model/GetTemplateRequest.h>
#include <aws/cloudformation/model/StackStatus.h>
#include <aws/cloudformation/model/UpdateStackRequest.h>
#include <aws/lambda/LambdaClient.h>
#include <aws/lambda/model/DeleteLayerVersionRequest.h>
#include <aws/lambda/model/PublishLayerVersionRequest.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/ssm/SSMClient.h>
#include <aws/ssm/model/GetParameterRequest.h>
#include <aws/ssm/model/PutParameterRequest.h>

// GameKit
#include <aws/gamekit/core/awsclients/api_initializer.h>
#include <aws/gamekit/core/awsclients/default_clients.h>
#include <aws/gamekit/core/enums.h>
#include <aws/gamekit/core/feature_resources_callback.h>
#include <aws/gamekit/core/gamekit_feature.h>
#include <aws/gamekit/core/model/account_credentials.h>
#include <aws/gamekit/core/model/account_info.h>
#include <aws/gamekit/core/model/config_consts.h>
#include <aws/gamekit/core/model/template_consts.h>
#include <aws/gamekit/core/paramstore_keys.h>
#include <aws/gamekit/core/zipper.h>
#include <aws/gamekit/core/utils/file_utils.h>
#include <aws/gamekit/core/aws_region_mappings.h>

// yaml-cpp
#include <yaml-cpp/yaml.h>

namespace GameKit
{
    /**
     * GameKitFeatureResources offers methods for working on the AWS resources of a single GAMEKIT feature (ex: "achievements").
     *
     * Each instance of this class operates on one feature, which is specified in it's constructor.
     *
     * For example, it can deploy/delete the feature's CloudFormation stack and create it's instance template files for deployment.
     */
    class GAMEKIT_API GameKitFeatureResources
    {
    private:
        AccountInfoCopy m_accountInfo;
        AccountCredentialsCopy m_credentials;
        FeatureType m_featureType;
        FuncLogCallback m_logCb = nullptr;

        Aws::S3::S3Client* m_s3Client;
        Aws::SSM::SSMClient* m_ssmClient;
        Aws::CloudFormation::CloudFormationClient* m_cfClient;
        Aws::Lambda::LambdaClient* m_lambdaClient;

        bool m_isUsingSharedS3Client = false;
        bool m_isUsingSharedSSMClient = false;
        bool m_isUsingSharedCfClient = false;
        bool m_isUsingSharedLambdaClient = false;

        std::string m_stackName;
        std::string m_layersReplacementId;
        std::string m_functionsReplacementId;

        std::string m_pluginRoot;
        std::string m_gamekitRoot;
        std::string m_baseLayersPath;
        std::string m_baseFunctionsPath;
        std::string m_baseCloudformationPath;
        std::string m_baseConfigOutputsPath;
        std::string m_instanceLayersPath;
        std::string m_instanceFunctionsPath;
        std::string m_instanceCloudformationPath;

        std::unordered_map<std::string, bool> m_resouceStatusMap;

        Aws::Vector<Aws::CloudFormation::Model::Parameter> getStackParameters(TemplateType templateType) const;
        std::string getRawStackParameters(TemplateType templateType) const;
        std::string getFeatureDashboardTemplate(TemplateType templateType) const;
        std::string getCloudFormationTemplate(TemplateType templateType) const;
        unsigned int createStack() const;
        unsigned int updateStack() const;
        unsigned int deleteStack() const;
        Aws::CloudFormation::Model::StackStatus periodicallyDescribeStackEvents();
        void describeStackEvents();
        unsigned int getDeployedTemplateBody(const std::string& stackName, std::string& templateBody) const;
        bool isTerminalState(Aws::CloudFormation::Model::StackStatus status);
        bool isFailedState(Aws::CloudFormation::Model::StackStatus status);
        std::string getTempLayersPath() const;
        std::string getTempFunctionsPath() const;
        YAML::Node getClientConfigYaml() const;
        std::vector<std::tuple<std::string, std::string>> getConfigOutputParameters() const;
        unsigned int writeCloudFormationParameterInstance(const std::string& cfParams) const;
        unsigned int writeCloudFormationTemplateInstance(const std::string& cfParams) const;
        unsigned int writeCloudFormationDashboardInstance(const std::string& cfTemplate) const;
        std::string getClientConfigFilePath() const;
        unsigned int writeClientConfigYamlToDisk(const YAML::Node& paramsYml) const;
        unsigned int removeOutputsFromClientConfiguration() const;
        unsigned int writeClientConfigurationWithOutputs(Aws::Vector<Aws::CloudFormation::Model::Output> outputs) const;
        std::string getFeatureLayerNameFromDirName(const std::string& layerDirName) const;
        Aws::Lambda::Model::PublishLayerVersionOutcome createFeatureLayer(const std::string& layerDirName, const std::string& s3ObjectName);
        bool lambdaLayerHashChanged(const std::string& layerName, const std::string& layerHash) const;
        unsigned int createAndSetLambdaLayerHash(const std::string& layerName, const std::string& layerHash) const;
        unsigned int createAndSetLambdaLayerArn(const std::string& layerName, const std::string& layerArn) const;
        std::string getShortRegionCode();

    public:
        GameKitFeatureResources(const AccountInfo accountInfo, const AccountCredentials credentials, FeatureType featureType, FuncLogCallback logCb);
        GameKitFeatureResources(const AccountInfoCopy& accountInfo, const AccountCredentialsCopy& credentials, FeatureType featureType, FuncLogCallback logCb);
        ~GameKitFeatureResources();

        // Clients initialized with his method will be deleted in the class destructor.
        void InitializeDefaultAwsClients();

        // Returns Account Info
        AccountInfoCopy GetAccountInfo()
        {
            return m_accountInfo;
        }

        // Returns Account Credentials
        AccountCredentialsCopy GetAccountCredentials()
        {
            return m_credentials;
        }

        // Sets the root directory of the plugin's installation
        inline void SetPluginRoot(const std::string& pluginRoot)
        {
            m_pluginRoot = pluginRoot;
            m_baseLayersPath = pluginRoot + ResourceDirectories::LAYERS_DIRECTORY + GetFeatureTypeString(m_featureType) + "/";
            m_baseFunctionsPath = pluginRoot + ResourceDirectories::FUNCTIONS_DIRECTORY + GetFeatureTypeString(m_featureType) + "/";
            m_baseCloudformationPath = pluginRoot + ResourceDirectories::CLOUDFORMATION_DIRECTORY + GetFeatureTypeString(m_featureType) + "/";
            m_baseConfigOutputsPath = pluginRoot + ResourceDirectories::CONFIG_OUTPUTS_DIRECTORY + GetFeatureTypeString(m_featureType) + "/";
        }

        // Returns the root directory of the plugin's installation
        inline const std::string& GetPluginRoot()
        {
            return m_pluginRoot;
        }

        // The value GAMEKIT_ROOT where instance templates and settings are going to be stored
        inline void SetGameKitRoot(const std::string& gamekitRoot)
        {
            m_gamekitRoot = gamekitRoot;
            m_instanceLayersPath = gamekitRoot + "/" + m_accountInfo.gameName + "/" + m_accountInfo.environment.GetEnvironmentString() + ResourceDirectories::LAYERS_DIRECTORY + GetFeatureTypeString(m_featureType) + "/";
            m_instanceFunctionsPath = gamekitRoot + "/" + m_accountInfo.gameName + "/" + m_accountInfo.environment.GetEnvironmentString() + ResourceDirectories::FUNCTIONS_DIRECTORY + GetFeatureTypeString(m_featureType) + "/";
            m_instanceCloudformationPath = gamekitRoot + "/" + m_accountInfo.gameName + "/" + m_accountInfo.environment.GetEnvironmentString() + ResourceDirectories::CLOUDFORMATION_DIRECTORY + GetFeatureTypeString(m_featureType) + "/";
        }

        // Returns the GAMEKIT_ROOT where instance templates and settings are stored
        inline const std::string& GetGameKitRoot()
        {
            return m_gamekitRoot;
        }

        // Sets the base CloudFormation path
        inline void SetBaseCloudFormationPath(const std::string& cloudFormationPath)
        {
            m_baseCloudformationPath = cloudFormationPath + GetFeatureTypeString(m_featureType) + "/";
        }

        // Sets the base Lambda Layers path
        inline void SetBaseLayersPath(const std::string& layersPath)
        {
            m_baseLayersPath = layersPath + GetFeatureTypeString(m_featureType) + "/";
        }

        // Sets the base Lambda Functions path
        inline void SetBaseFunctionsPath(const std::string& functionsPath)
        {
            m_baseFunctionsPath = functionsPath + GetFeatureTypeString(m_featureType) + "/";
        }

        // Sets the instance CloudFormation path
        inline void SetInstanceCloudFormationPath(const std::string& cloudFormationPath)
        {
            m_instanceCloudformationPath = cloudFormationPath + GetFeatureTypeString(m_featureType) + "/";
        }

        // Sets the instance Lambda Layers path
        void SetInstanceLayersPath(const std::string& layersPath)
        {
            m_instanceLayersPath = layersPath + GetFeatureTypeString(m_featureType) + "/";
        }

        // Sets the instance Lambda Functions path
        void SetInstanceFunctionsPath(const std::string& functionsPath)
        {
            m_instanceFunctionsPath = functionsPath + GetFeatureTypeString(m_featureType) + "/";
        }

        // Returns the base Lambda Functions path
        inline const std::string& GetBaseFunctionsPath() const
        {
            return m_baseFunctionsPath;
        }

        // Returns the base CloudFormation path
        inline const std::string& GetBaseCloudFormationPath() const
        {
            return m_baseCloudformationPath;
        }

        // Returns the instance Lambda Functions path
        inline const std::string& GetInstanceFunctionsPath() const
        {
            return m_instanceFunctionsPath;
        }

        // Returns the instance CloudFormation path
        inline const std::string& GetInstanceCloudFormationPath() const
        {
            return m_instanceCloudformationPath;
        }

        bool IsCloudFormationInstanceTemplatePresent() const;

        unsigned int SaveDeployedCloudFormationTemplate() const;
        unsigned int GetDeployedCloudFormationParameters(DeployedParametersCallback callback) const;
        unsigned int SaveCloudFormationInstance();
        unsigned int UpdateCloudFormationParameters();
        unsigned int SaveLayerInstances() const;
        unsigned int SaveFunctionInstances() const;

        std::string GetStackName() const;
        void SetLayersReplacementId(const std::string& replacementId);
        void SetFunctionsReplacementId(const std::string& replacementId);
        unsigned int CreateAndSetLayersReplacementId();
        unsigned int CreateAndSetFunctionsReplacementId();
        unsigned int UploadDashboard(const std::string& path);
        unsigned int CompressFeatureLayers();
        unsigned int UploadFeatureLayers();
        unsigned int CompressFeatureFunctions();
        unsigned int UploadFeatureFunctions();
        void CleanupTempFiles();
        std::string GetCurrentStackStatus() const;
        unsigned int DescribeStackResources(FuncResourceInfoCallback resourceInfoCb) const;
        unsigned int CreateOrUpdateFeatureStack();
        unsigned int WriteEmptyClientConfiguration() const;
        unsigned int WriteClientConfiguration() const;
        unsigned int DeleteFeatureStack();

        inline std::string GetLambdaFunctionReplacementIDParamName() const
        {
            return std::string(GAMEKIT_LAMBDA_FUNCTIONS_REPLACEMENT_ID_PREFIX)
                .append(GetFeatureTypeString(m_featureType))
                .append("_")
                .append(m_accountInfo.gameName)
                .append("_")
                .append(m_accountInfo.environment.GetEnvironmentString());
        }

        inline std::string GetLambdaLayerReplacementIDParamName() const
        {
            return std::string(GAMEKIT_LAMBDA_LAYERS_REPLACEMENT_ID_PREFIX)
                .append(GetFeatureTypeString(m_featureType))
                .append("_")
                .append(m_accountInfo.gameName)
                .append("_")
                .append(m_accountInfo.environment.GetEnvironmentString());
        }

        inline std::string GetLambdaLayerARNParamName(const std::string& layerName) const
        {
            return std::string(GAMEKIT_LAMBDA_LAYER_ARN_PREFIX)
                .append(GetFeatureTypeString(m_featureType))
                .append("_")
                .append(layerName)
                .append("_")
                .append(m_accountInfo.gameName)
                .append("_")
                .append(m_accountInfo.environment.GetEnvironmentString());
        }

        inline std::string GetLambdaLayerHashParamName(const std::string& layerName) const
        {
            return std::string(GAMEKIT_LAMBDA_LAYER_HASH_PREFIX)
                .append(GetFeatureTypeString(m_featureType))
                .append("_")
                .append(layerName)
                .append("_")
                .append(m_accountInfo.gameName)
                .append("_")
                .append(m_accountInfo.environment.GetEnvironmentString());
        }

        inline void SetS3Client(Aws::S3::S3Client* s3Client, bool isShared)
        {
            m_isUsingSharedS3Client = isShared;
            m_s3Client = s3Client;
        }

        inline void SetSSMClient(Aws::SSM::SSMClient* ssmClient, bool isShared)
        {
            m_isUsingSharedSSMClient = isShared;
            m_ssmClient = ssmClient;
        }

        inline void SetCloudFormationClient(Aws::CloudFormation::CloudFormationClient* cfClient, bool isShared)
        {
            m_isUsingSharedCfClient = isShared;
            m_cfClient = cfClient;
        }

        inline void SetLambdaClient(Aws::Lambda::LambdaClient* lambdaClient, bool isShared)
        {
            m_isUsingSharedLambdaClient = isShared;
            m_lambdaClient = lambdaClient;
        }
    };
}
