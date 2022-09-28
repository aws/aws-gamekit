// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// GameKit
#include <aws/gamekit/core/deployment_orchestrator.h>
#include <aws/gamekit/core/errors.h>
#include <aws/gamekit/core/gamekit_settings.h>

using namespace GameKit;

#pragma region Macros
#define SET_STATUS_AND_RETURN_IF_ERROR(result, feature, message) \
{ \
    if (result != GAMEKIT_SUCCESS) \
    { \
        setFeatureStatus(feature, FeatureStatus::Error); \
        Logger::Logging::Log(m_logCb, Logger::Level::Error, message); \
        return result; \
    } \
}
#pragma endregion

#pragma region Constructor / Destructor
GameKitDeploymentOrchestrator::GameKitDeploymentOrchestrator(const std::string& baseTemplatesFolder, const std::string& instanceFilesFolder, const std::string& sourceEngine, const std::string& pluginVersion, FuncLogCallback logCb) :
    m_baseTemplatesFolder(baseTemplatesFolder), m_instanceFilesFolder(instanceFilesFolder), m_logCb(logCb), m_sourceEngine(sourceEngine), m_pluginVersion(pluginVersion)
{
    Logging::Log(m_logCb, Level::Info, "GameKitDeploymentOrchestrator()", this);
    AwsApiInitializer::Initialize(m_logCb, this);
}

GameKitDeploymentOrchestrator::~GameKitDeploymentOrchestrator()
{
    Logging::Log(m_logCb, Level::Info, "~GameKitDeploymentOrchestrator()", this);
    AwsApiInitializer::Shutdown(m_logCb, this);
}
#pragma endregion

#pragma region Private Methods
bool GameKitDeploymentOrchestrator::areUpstreamFeaturesDeployedAndUsable(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver, CanExecuteDeploymentActionCallback callback) const
{
    std::unordered_set<FeatureType> upstreamFeatures = getUndeployedUpstreamFeatures(feature);
    if (!upstreamFeatures.empty())
    {
        return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, false, DeploymentActionBlockedReason::DependenciesMustBeCreated, upstreamFeatures);
    }

    upstreamFeatures = getUnusableUpstreamFeatures(feature);
    if (!upstreamFeatures.empty())
    {
        return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, false, DeploymentActionBlockedReason::DependenciesStatusIsInvalid, upstreamFeatures);
    }

    return true;
}

std::unordered_set<FeatureType> GameKitDeploymentOrchestrator::getUndeployedUpstreamFeatures(FeatureType feature) const
{
    std::unordered_set<FeatureType> undeployedUpstreamFeatures;
    const auto upstreamFeatures = m_featureDependencies.find(feature);
    if (upstreamFeatures == m_featureDependencies.end())
    {
        // No known dependencies for this feature
        return undeployedUpstreamFeatures;
    }

    for (const FeatureType upstreamFeature : upstreamFeatures->second)
    {
        const FeatureStatus upstreamFeatureStatus = GetFeatureStatus(upstreamFeature);
        if (upstreamFeatureStatus == FeatureStatus::Undeployed)
        {
            undeployedUpstreamFeatures.insert(upstreamFeature);
        }
    }

    return undeployedUpstreamFeatures;
}

std::unordered_set<FeatureType> GameKitDeploymentOrchestrator::getUnusableUpstreamFeatures(FeatureType feature) const
{
    std::unordered_set<FeatureType> unusableUpstreamFeatures;
    const auto upstreamFeatures = m_featureDependencies.find(feature);
    if (upstreamFeatures == m_featureDependencies.end())
    {
        // No known dependencies for this feature
        return unusableUpstreamFeatures;
    }

    for (const FeatureType upstreamFeature : upstreamFeatures->second)
    {
        const FeatureStatus upstreamFeatureStatus = GetFeatureStatus(upstreamFeature);
        if (m_featureUsableStatuses.find(upstreamFeatureStatus) == m_featureUsableStatuses.end())
        {
            unusableUpstreamFeatures.insert(upstreamFeature);
        }
    }

    return unusableUpstreamFeatures;
}

bool GameKitDeploymentOrchestrator::areCredentialsValid() const
{
    if (m_accountInfo.accountId.empty() || m_accountInfo.gameName.empty()
        || m_accountCredentials.accessKey.empty() || m_accountCredentials.accessSecret.empty() || m_accountCredentials.accountId.empty()
        || m_accountCredentials.region.empty() || m_accountCredentials.shortRegionCode.empty())
    {
        return false;
    }

    return true;
}

bool GameKitDeploymentOrchestrator::isDeploymentStateValid(
    FeatureType feature, 
    const std::function<bool(FeatureStatus)> customStatusValidationHandler,
    DISPATCH_RECEIVER_HANDLE receiver,
    CanExecuteDeploymentActionCallback callback) const
{
    if (!areUpstreamFeaturesDeployedAndUsable(feature, receiver, callback))
    {
        return false;
    }

    const FeatureStatus featureStatus = GetFeatureStatus(feature);
    if (featureStatus == FeatureStatus::Unknown)
    {
        return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, false, DeploymentActionBlockedReason::FeatureStatusIsUnknown);
    }

    // Run any additional validation the user supplied. This is expected to be different for feature creation and feature redeployment.
    if (!customStatusValidationHandler(featureStatus))
    {
        return false;
    }

    return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, true, DeploymentActionBlockedReason::NotBlocked);
}

bool GameKitDeploymentOrchestrator::isCreateStateValid(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver, CanExecuteDeploymentActionCallback callback) const
{
    return isDeploymentStateValid(feature, [this, receiver, callback, feature](FeatureStatus featureStatus)
        {
            if (m_createEnabledStatuses.find(featureStatus) == m_createEnabledStatuses.end())
            {
                // Feature already exists; must be deleted before we can create it
                return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, false, DeploymentActionBlockedReason::FeatureMustBeDeleted);
            }
            return true;
        }, receiver, callback);
}

bool GameKitDeploymentOrchestrator::isRedeployStateValid(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver, CanExecuteDeploymentActionCallback callback) const
{
    return isDeploymentStateValid(feature, [this, receiver, callback, feature](FeatureStatus featureStatus)
        {
            if (m_redeployEnabledStatuses.find(featureStatus) == m_redeployEnabledStatuses.end())
            {
                // Feature does not exist; must be created before we can redeploy it
                return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, false, DeploymentActionBlockedReason::FeatureMustBeCreated);
            }
            return true;
        }, receiver, callback);
}

bool GameKitDeploymentOrchestrator::isDeleteStateValid(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver, CanExecuteDeploymentActionCallback callback) const
{
    const FeatureStatus featureStatus = GetFeatureStatus(feature);
    if (featureStatus == FeatureStatus::Unknown)
    {
        return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, false, DeploymentActionBlockedReason::FeatureStatusIsUnknown);
    }

    if (m_deleteEnabledStatuses.find(featureStatus) == m_deleteEnabledStatuses.end())
    {
        return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, false, DeploymentActionBlockedReason::FeatureMustBeCreated);
    }

    // If the main stack is not in a usable state, disable delete as a downstream feature could be preparing to be deployed.
    const FeatureStatus mainStackStatus = GetFeatureStatus(FeatureType::Main);
    if (m_featureUsableStatuses.find(mainStackStatus) == m_featureUsableStatuses.end())
    {
        return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, false, DeploymentActionBlockedReason::MainStackNotReady);
    }

    // Ensure no features are currently consuming this feature
    std::unordered_set<FeatureType> deployedDownstreamFeatures;
    for (const std::pair<FeatureType, std::unordered_set<FeatureType>>& featureDependency : m_featureDependencies)
    {
        if (featureDependency.second.find(feature) != featureDependency.second.end())
        {
            // A feature depends on the feature we're trying to delete. Only proceed if it is not deployed.
            FeatureStatus downstreamFeatureStatus = GetFeatureStatus(featureDependency.first);
            if (downstreamFeatureStatus != FeatureStatus::Undeployed)
            {
                // The downstream dependency for the feature we're trying to delete has been deployed in some fashion; we cannot delete the upstream feature.
                deployedDownstreamFeatures.insert(featureDependency.first);
            }
        }
    }

    if (!deployedDownstreamFeatures.empty())
    {
        return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, false, DeploymentActionBlockedReason::DependenciesMustBeDeleted, deployedDownstreamFeatures);
    }

    return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, true, DeploymentActionBlockedReason::NotBlocked);
}

std::shared_ptr<GameKitAccount> GameKitDeploymentOrchestrator::getAccount()
{
    std::lock_guard guard(m_accountMutex);
    if (m_account == nullptr)
    {
        // Lazy load a new GameKitAccount instance
        m_account = std::make_shared<GameKitAccount>(m_accountInfo, m_accountCredentials, m_logCb);
        m_account->SetPluginRoot(m_baseTemplatesFolder);
        m_account->SetGameKitRoot(m_instanceFilesFolder);
        m_account->InitializeDefaultAwsClients();
    }

    return m_account;
}

std::shared_ptr<GameKitFeatureResources> GameKitDeploymentOrchestrator::getFeatureResources(FeatureType feature)
{
    std::lock_guard guard(m_featureResourcesMutex);

    const auto featureResourcesIter = m_featureResourcesMap.find(feature);
    if (featureResourcesIter != m_featureResourcesMap.end())
    {
        return featureResourcesIter->second;
    }

    // Lazy load a new feature resources instance
    std::shared_ptr<GameKitFeatureResources> featureResources = std::make_shared<GameKitFeatureResources>(m_accountInfo, m_accountCredentials, feature, m_logCb);
    featureResources->SetPluginRoot(m_baseTemplatesFolder);
    featureResources->SetGameKitRoot(m_instanceFilesFolder);
    
    m_featureResourcesMap[feature] = featureResources;

    return featureResources;
}

unsigned int GameKitDeploymentOrchestrator::deployFeature(FeatureType feature)
{
    const std::shared_ptr<GameKitFeatureResources> featureResources = getFeatureResources(feature);
    
    const FeatureStatus initialStatus = GetFeatureStatus(feature);
    unsigned int result = GAMEKIT_SUCCESS;

    setFeatureStatus(feature, FeatureStatus::GeneratingTemplates);
    
    if (initialStatus == FeatureStatus::Undeployed)
    {
        // Ensure all templates files have been copied to the instance files location
        // Don't overwrite any existing instance files
        if (!featureResources->IsCloudFormationInstanceTemplatePresent())
        {
            result = featureResources->SaveCloudFormationInstance(m_sourceEngine, m_pluginVersion);
            SET_STATUS_AND_RETURN_IF_ERROR(result, feature, "Failed to generate CloudFormation instance files");
        }

        if (!featureResources->AreLayerInstancesPresent())
        {
            result = featureResources->SaveLayerInstances();
            SET_STATUS_AND_RETURN_IF_ERROR(result, feature, "Failed to generate Lambda Layer instances files");
        }

        if (!featureResources->AreFunctionInstancesPresent())
        {
            result = featureResources->SaveFunctionInstances();
            SET_STATUS_AND_RETURN_IF_ERROR(result, feature, "Failed to generate Lambda Function instance files");
        }
    }

    setFeatureStatus(feature, FeatureStatus::UploadingDashboards);

    // Upload dashboard is expecting the instance cfn path for the feature, but without an ending slash
    std::string instanceCloudFormationPath = featureResources->GetInstanceCloudFormationPath();
    // Remove the final '/' character
    instanceCloudFormationPath.pop_back();

    result = featureResources->UploadDashboard(instanceCloudFormationPath);
    SET_STATUS_AND_RETURN_IF_ERROR(result, feature, "Failed to upload CloudFormation dashboard");

    setFeatureStatus(feature, FeatureStatus::UploadingLayers);
    result = featureResources->DeployFeatureLayers();
    SET_STATUS_AND_RETURN_IF_ERROR(result, feature, "Failed to upload Lambda Layers");

    setFeatureStatus(feature, FeatureStatus::UploadingFunctions);
    result = featureResources->DeployFeatureFunctions();
    SET_STATUS_AND_RETURN_IF_ERROR(result, feature, "Failed to upload Lambda Functions");

    setFeatureStatus(feature, FeatureStatus::DeployingResources);
    result = featureResources->CreateOrUpdateFeatureStack();
    SET_STATUS_AND_RETURN_IF_ERROR(result, feature, "Failed to deploy CloudFormation stack");

    result = getAccount()->DeployApiGatewayStage();
    SET_STATUS_AND_RETURN_IF_ERROR(result, feature, "Failed to deploy API Gateway stage");

    setFeatureStatus(feature, FeatureStatus::Deployed);
    return GAMEKIT_SUCCESS;
}

unsigned int GameKitDeploymentOrchestrator::validateAndDeployFeature(FeatureType feature)
{
    unsigned int result = validateFeatureSettings(feature);
    if (result != GAMEKIT_SUCCESS)
    {
        const std::string errorMessage = "Failed to validate settings for feature " + GetFeatureTypeString(feature) + ". Check the error log for more details.";
        Logger::Logging::Log(m_logCb, Logger::Level::Error, errorMessage.c_str());
        return result;
    }

    result = deployFeature(feature);
    if (result != GAMEKIT_SUCCESS)
    {
        const std::string errorMessage = "Failed to deploy feature " + GetFeatureTypeString(feature);
        Logger::Logging::Log(m_logCb, Logger::Level::Error, errorMessage.c_str());
        return result;
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitDeploymentOrchestrator::createOrRedeployFeatureAndMainStack(FeatureType feature, std::function<bool(FeatureType)> isFeatureStateValid)
{
    // We need to sync our statuses and (re)deploy the main stack before we can create the target feature.
    // Signal that we are working on deploying the main stack and the target feature.
    setDeploymentInProgress(FeatureType::Main, true);
    setDeploymentInProgress(feature, true);

    // Ensure all of our stack statuses are up to date to account for remote modifications
    RefreshFeatureStatuses();

    // Create or redeploy the main stack
    if (!isCreateStateValid(FeatureType::Main) && !isRedeployStateValid(FeatureType::Main))
    {
        const std::string errorMessage = "Cannot deploy the main stack, as it is in an invalid state for deployment";
        Logger::Logging::Log(m_logCb, Logger::Level::Error, errorMessage.c_str());
        setDeploymentInProgress(FeatureType::Main, false);
        setDeploymentInProgress(feature, false);

        return GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE;
    }

    unsigned int result = validateAndDeployFeature(FeatureType::Main);
    setDeploymentInProgress(FeatureType::Main, false);
    
    if (result != GAMEKIT_SUCCESS)
    {
        setDeploymentInProgress(feature, false);
        return result;
    }

    // Deploy the feature stack if we're in a valid state to do so.
    // This is expected to be different for feature creation and feature redeployment - let the caller decide what this looks like for their desired operation.
    if (!isFeatureStateValid(feature))
    {
        const std::string errorMessage = "Cannot deploy the feature " + GetFeatureTypeString(feature) + ", as it or one of its upstream dependencies are in an invalid state for deployment";
        Logger::Logging::Log(m_logCb, Logger::Level::Error, errorMessage.c_str());

        setDeploymentInProgress(feature, false);
        return GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE;
    }

    // Underlying deployment code is the same for create vs redeploy; deploy the feature
    result = validateAndDeployFeature(feature);
    setDeploymentInProgress(feature, false);

    return result;
}



unsigned int GameKitDeploymentOrchestrator::validateFeatureSettings(FeatureType feature) const
{
    const GameKitSettings settings = GameKitSettings(m_instanceFilesFolder, "1.1", m_accountInfo.gameName, m_accountInfo.environment.GetEnvironmentString(), m_logCb);
    std::map<std::string, std::string> variables = settings.GetFeatureVariables(feature);

    switch (feature)
    {
    case FeatureType::Identity:
        if (variables.find(IS_FACEBOOK_ENABLED) != variables.end() && "true" == variables.at(IS_FACEBOOK_ENABLED)
            && variables.find(FACEBOOK_CLIENT_ID) != variables.end() && variables.at(FACEBOOK_CLIENT_ID).empty())
        {
            const std::string errorMessage = "The '" + FACEBOOK_CLIENT_ID + "' setting must not be empty when '" + IS_FACEBOOK_ENABLED + "' is true";
            Logger::Logging::Log(m_logCb, Logger::Level::Error, errorMessage.c_str());
            return GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_SETTINGS;
        }
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKitDeploymentOrchestrator::invokeDeploymentResponseCallback(DISPATCH_RECEIVER_HANDLE receiver, DeploymentResponseCallback callback, unsigned int callStatus) const
{
    if (receiver != nullptr && callback != nullptr)
    {       
        // Copy feature statuses into parallel arrays
        std::vector<FeatureType> returnedFeatureTypes;
        std::vector<FeatureStatus> returnedFeatureStatuses;

        for (const FeatureType feature : m_availableFeatures)
        {
            returnedFeatureTypes.push_back(feature);
            returnedFeatureStatuses.push_back(GetFeatureStatus(feature));
        }

        // Send the most up to date feature statuses back to the user
        callback(receiver, returnedFeatureTypes.data(), returnedFeatureStatuses.data(), returnedFeatureTypes.size(), callStatus);
    }

    return callStatus;
}

bool GameKitDeploymentOrchestrator::invokeCanExecuteDeploymentActionCallback(
    DISPATCH_RECEIVER_HANDLE receiver,
    CanExecuteDeploymentActionCallback callback,
    FeatureType targetFeature,
    bool canExecuteAction,
    DeploymentActionBlockedReason reason,
    std::unordered_set<FeatureType> blockingFeatures) const
{
    if (receiver != nullptr && callback != nullptr)
    {
        // Copy blocking features into an array to return
        std::vector<FeatureType> returnedBlockingFeatures(blockingFeatures.begin(), blockingFeatures.end());

        // Send the most up to date feature statuses back to the user
        callback(receiver, targetFeature, canExecuteAction, reason, returnedBlockingFeatures.data(), returnedBlockingFeatures.size());
    }

    return canExecuteAction;
}
#pragma endregion

#pragma region Protected Methods
void GameKitDeploymentOrchestrator::setFeatureStatus(FeatureType feature, FeatureStatus status)
{
    std::unique_lock guard(m_featureStatusMutex);

    m_featureStatusMap[feature] = status;
}

void GameKitDeploymentOrchestrator::setDeploymentInProgress(FeatureType feature, bool inProgress)
{
    std::unique_lock guard(m_deploymentInProgressMutex);

    m_deploymentInProgressMap[feature] = inProgress;
}

std::unordered_set<FeatureType> GameKitDeploymentOrchestrator::getFeatureOrUpstreamDeploymentsInProgress(FeatureType feature) const
{
    std::unordered_set<FeatureType> deployingFeatures;

    // Search for ongoing deployments of upstream features
    const auto upstreamFeatures = m_featureDependencies.find(feature);
    if (upstreamFeatures != m_featureDependencies.end())
    {
        for (const FeatureType upstreamFeature : upstreamFeatures->second)
        {
            if (IsFeatureDeploymentInProgress(upstreamFeature))
            {
                deployingFeatures.insert(upstreamFeature);
            }
        }
    }

    // The main stack is an implicit upstream for all features; check if it's being deployed
    if (IsFeatureDeploymentInProgress(FeatureType::Main))
    {
        deployingFeatures.insert(FeatureType::Main);
    }

    // No ongoing deployments for upstream features; check if the target feature is being deployed
    if (IsFeatureDeploymentInProgress(feature))
    {
        deployingFeatures.insert(feature);
    }

    return deployingFeatures;
}

bool GameKitDeploymentOrchestrator::isFeatureOrUpstreamDeploymentInProgress(FeatureType feature) const
{
    const std::unordered_set<FeatureType> deployingFeatures = getFeatureOrUpstreamDeploymentsInProgress(feature);

    return !deployingFeatures.empty();
}

void GameKitDeploymentOrchestrator::setFeatureResources(FeatureType feature, std::shared_ptr<GameKitFeatureResources> featureResources)
{
    std::lock_guard guard(m_featureResourcesMutex);

    m_featureResourcesMap[feature] = featureResources;
}

void GameKitDeploymentOrchestrator::setAccount(std::shared_ptr<GameKitAccount> account)
{
    std::lock_guard guard(m_accountMutex);
    m_account = account;
}
#pragma endregion

#pragma region Public Methods
unsigned int GameKitDeploymentOrchestrator::SetCredentials(const AccountInfo& accountInfo, const AccountCredentials& accountCredentials)
{
    // Only allow changing credentials if a deployment is not in progress
    std::unique_lock deploymentGuard(m_deploymentInProgressMutex);
    for (const std::pair<FeatureType, bool>& isDeploymentInProgress : m_deploymentInProgressMap)
    {
        if (isDeploymentInProgress.second)
        {
            deploymentGuard.unlock();
            std::string errorMessage = "Cannot change credentials as a local deployment for feature " + GetFeatureTypeString(isDeploymentInProgress.first) + " is in progress";
            Logger::Logging::Log(m_logCb, Logger::Level::Error, errorMessage.c_str());
            return GAMEKIT_ERROR_ORCHESTRATION_DEPLOYMENT_IN_PROGRESS;
        }
    }

    AwsRegionMappings& regionMappings = AwsRegionMappings::getInstance(m_baseTemplatesFolder, m_logCb);
    const std::string shortRegionCode = regionMappings.getFiveLetterRegionCode(std::string(accountCredentials.region));
    
    if (shortRegionCode.empty())
    {
        std::string msg = "Could not retrieve short region code for: " + std::string(accountCredentials.region) + " which will forbid you from signing admin requests.";
        Logging::Log(m_logCb, Level::Error, msg.c_str());
        return GAMEKIT_ERROR_REGION_CODE_CONVERSION_FAILED;
    }

    m_accountInfo = CreateAccountInfoCopy(accountInfo);
    m_accountCredentials = CreateAccountCredentialsCopy(accountCredentials, shortRegionCode);
    m_accountCredentials.accountId = m_accountInfo.accountId;
    m_account = nullptr;

    // Reset local state, as the game, environment, and credentials may have all changed
    m_deploymentInProgressMap.clear();
    deploymentGuard.unlock();

    std::unique_lock featureResourcesGuard(m_featureResourcesMutex);
    m_featureResourcesMap.clear();
    featureResourcesGuard.unlock();

    std::unique_lock featureStatusGuard(m_featureStatusMutex);
    m_featureStatusMap.clear();
    featureStatusGuard.unlock();

    return GAMEKIT_SUCCESS;
}

FeatureStatus GameKitDeploymentOrchestrator::GetFeatureStatus(FeatureType feature) const
{
    std::shared_lock guard(m_featureStatusMutex);

    const auto featureStatus = m_featureStatusMap.find(feature);
    if (featureStatus == m_featureStatusMap.end())
    {
        return FeatureStatus::Unknown;
    }
    return featureStatus->second;
}

FeatureStatusSummary GameKitDeploymentOrchestrator::GetFeatureStatusSummary(FeatureType feature) const
{
    const FeatureStatus status = GetFeatureStatus(feature);

    return GetSummaryFromFeatureStatus(status);
}

bool GameKitDeploymentOrchestrator::IsFeatureDeploymentInProgress(FeatureType feature) const
{
    std::shared_lock guard(m_deploymentInProgressMutex);

    const auto deploymentInProgress = m_deploymentInProgressMap.find(feature);
    if (deploymentInProgress != m_deploymentInProgressMap.end())
    {
        return deploymentInProgress->second;
    }

    return false;
}

bool GameKitDeploymentOrchestrator::IsFeatureUpdating(FeatureType feature) const
{
    const FeatureStatus deploymentStatus = GetFeatureStatus(feature);
    return m_atRestStatuses.find(deploymentStatus) == m_atRestStatuses.end();
}

bool GameKitDeploymentOrchestrator::IsAnyFeatureUpdating() const
{
    for (const FeatureType feature : m_availableFeatures)
    {
        if (IsFeatureUpdating(feature))
        {
            return true;
        }
    }

    return false;
}

unsigned int GameKitDeploymentOrchestrator::RefreshFeatureStatus(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver, DeploymentResponseCallback callback)
{
    const std::shared_ptr<GameKitFeatureResources> featureResources = getFeatureResources(feature);
    const std::string cloudFormationStatus = featureResources->GetCurrentStackStatus();
    const FeatureStatus featureStatusFromCloudFormationStatus = GetFeatureStatusFromCloudFormationStackStatus(cloudFormationStatus);

    // For an in-progress feature deployment, the local running status (more descriptive) takes precedence over cloudformation running status
    if (!(IsFeatureDeploymentInProgress(feature) && IsFeatureUpdating(feature)))
    {
        setFeatureStatus(feature, featureStatusFromCloudFormationStatus);
    }

    return invokeDeploymentResponseCallback(receiver, callback, GAMEKIT_SUCCESS);
}

unsigned int GameKitDeploymentOrchestrator::RefreshFeatureStatuses(DISPATCH_RECEIVER_HANDLE receiver, DeploymentResponseCallback callback)
{
    for (const FeatureType feature : m_availableFeatures)
    {
        RefreshFeatureStatus(feature, nullptr, nullptr);
    }

    return invokeDeploymentResponseCallback(receiver, callback, GAMEKIT_SUCCESS);
}

bool GameKitDeploymentOrchestrator::CanCreateFeature(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver, CanExecuteDeploymentActionCallback callback) const
{
    if (!areCredentialsValid())
    {
        return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, false, DeploymentActionBlockedReason::CredentialsInvalid);
    }

    const std::unordered_set<FeatureType> deployingFeatures = getFeatureOrUpstreamDeploymentsInProgress(feature);
    if (!deployingFeatures.empty())
    {
        return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, false, DeploymentActionBlockedReason::OngoingDeployments, deployingFeatures);
    }

    return isCreateStateValid(feature, receiver, callback);
}

bool GameKitDeploymentOrchestrator::CanRedeployFeature(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver, CanExecuteDeploymentActionCallback callback) const
{
    if (!areCredentialsValid())
    {
        return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, false, DeploymentActionBlockedReason::CredentialsInvalid);
    }

    const std::unordered_set<FeatureType> deployingFeatures = getFeatureOrUpstreamDeploymentsInProgress(feature);
    if (!deployingFeatures.empty())
    {
        return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, false, DeploymentActionBlockedReason::OngoingDeployments, deployingFeatures);
    }

    return isRedeployStateValid(feature, receiver, callback);
}

bool GameKitDeploymentOrchestrator::CanDeleteFeature(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver, CanExecuteDeploymentActionCallback callback) const
{
    if (!areCredentialsValid())
    {
        return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, false, DeploymentActionBlockedReason::CredentialsInvalid);
    }

    if (IsFeatureDeploymentInProgress(feature))
    {
        return invokeCanExecuteDeploymentActionCallback(receiver, callback, feature, false, DeploymentActionBlockedReason::OngoingDeployments, std::unordered_set<FeatureType> { feature });
    }
    return isDeleteStateValid(feature, receiver, callback);
}

unsigned int GameKitDeploymentOrchestrator::CreateFeature(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver, DeploymentResponseCallback callback)
{
    if (!CanCreateFeature(feature))
    {
        const std::string errorMessage = "Cannot create feature " + GetFeatureTypeString(feature) + ", as it or one of its dependencies are in an invalid state for deployment";
        Logger::Logging::Log(m_logCb, Logger::Level::Warning, errorMessage.c_str());

        return invokeDeploymentResponseCallback(receiver, callback, GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE);
    }

    const unsigned int result = createOrRedeployFeatureAndMainStack(feature, [this] (FeatureType feature) { return isCreateStateValid(feature); });

    return invokeDeploymentResponseCallback(receiver, callback, result);
}

unsigned int GameKitDeploymentOrchestrator::RedeployFeature(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver, DeploymentResponseCallback callback)
{
    if (!CanRedeployFeature(feature))
    {
        const std::string errorMessage = "Cannot redeploy feature " + GetFeatureTypeString(feature) + ", as it or one of its dependencies are in an invalid state for deployment";
        Logger::Logging::Log(m_logCb, Logger::Level::Warning, errorMessage.c_str());

        return invokeDeploymentResponseCallback(receiver, callback, GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE);
    }

    const unsigned int result = createOrRedeployFeatureAndMainStack(feature, [this](FeatureType feature) { return isRedeployStateValid(feature); });

    return invokeDeploymentResponseCallback(receiver, callback, result);
}

unsigned int GameKitDeploymentOrchestrator::DeleteFeature(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver, DeploymentResponseCallback callback)
{
    if (!CanDeleteFeature(feature))
    {
        const std::string errorMessage = "Cannot create feature " + GetFeatureTypeString(feature) + ", as it or one of its downstream dependencies are in an invalid state for deletion";
        Logger::Logging::Log(m_logCb, Logger::Level::Warning, errorMessage.c_str());

        return invokeDeploymentResponseCallback(receiver, callback, GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE);
    }

    setDeploymentInProgress(feature, true);

    RefreshFeatureStatuses();

    if (!isDeleteStateValid(feature))
    {
        setDeploymentInProgress(feature, false);
        
        const std::string errorMessage = "Cannot delete feature " + GetFeatureTypeString(feature) + ", as it or one of its downstream dependencies are in an invalid state for deletion";
        Logger::Logging::Log(m_logCb, Logger::Level::Error, errorMessage.c_str());

        return invokeDeploymentResponseCallback(receiver, callback, GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE);
    }

    std::shared_ptr<GameKitFeatureResources> featureResources = getFeatureResources(feature);
    setFeatureStatus(feature, FeatureStatus::DeletingResources);
    const unsigned int result = featureResources->DeleteFeatureStack();
    setDeploymentInProgress(feature, false);

    if (result != GAMEKIT_SUCCESS)
    {
        setFeatureStatus(feature, FeatureStatus::Error);

        const std::string errorMessage = "Failed to delete feature " + GetFeatureTypeString(feature);
        Logger::Logging::Log(m_logCb, Logger::Level::Error, errorMessage.c_str());
    }
    else
    {
        setFeatureStatus(feature, FeatureStatus::Undeployed);
    }

    return invokeDeploymentResponseCallback(receiver, callback, result);
}

unsigned int GameKitDeploymentOrchestrator::DescribeFeatureResources(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver, DispatchedResourceInfoCallback callback)
{
    std::shared_ptr<GameKitFeatureResources> featureResources = getFeatureResources(feature);
    return featureResources->DescribeStackResources(receiver, callback);
}

#pragma endregion