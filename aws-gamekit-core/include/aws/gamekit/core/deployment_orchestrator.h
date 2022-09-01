// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard library
#include <functional>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

// GameKit
#include <aws/gamekit/core/awsclients/api_initializer.h>
#include <aws/gamekit/core/enums.h>
#include <aws/gamekit/core/exports.h>
#include <aws/gamekit/core/feature_resources.h>
#include <aws/gamekit/core/gamekit_account.h>

namespace GameKit
{
    static const std::string IS_FACEBOOK_ENABLED = "is_facebook_enabled";
    static const std::string FACEBOOK_CLIENT_ID = "facebook_client_id";

    class GAMEKIT_API GameKitDeploymentOrchestrator
    {
    private:
        GameKitDeploymentOrchestrator(const GameKitDeploymentOrchestrator&) = delete;
        GameKitDeploymentOrchestrator& operator=(const GameKitDeploymentOrchestrator&) = delete;

        const std::string m_sourceEngine;
        const std::string m_pluginVersion;
        const std::string m_baseTemplatesFolder;
        const std::string m_instanceFilesFolder;
        FuncLogCallback m_logCb = nullptr;

        // Locally cached feature statuses
        mutable std::shared_timed_mutex m_featureStatusMutex;
        std::unordered_map<FeatureType, FeatureStatus> m_featureStatusMap;

        // Lazy-loaded FeatureResources instances
        std::mutex m_featureResourcesMutex;
        std::unordered_map<FeatureType, std::shared_ptr<GameKitFeatureResources>> m_featureResourcesMap;

        // Keep track of high-level deployment progress; basic feature states don't fully capture the intent,
        // as deployment of any feature first requires deployment of the main stack.
        // Only tracks local deployment state and intent.
        mutable std::shared_timed_mutex m_deploymentInProgressMutex;
        std::unordered_map<FeatureType, bool> m_deploymentInProgressMap;

        // Lazy-loaded Account instance
        std::mutex m_accountMutex;
        std::shared_ptr<GameKitAccount> m_account;

        AccountInfoCopy m_accountInfo;
        AccountCredentialsCopy m_accountCredentials;

        const std::unordered_map<FeatureType, std::unordered_set<FeatureType>> m_featureDependencies =
        {
            { FeatureType::Main, { } },
            { FeatureType::Identity, { } },
            { FeatureType::Achievements, { FeatureType::Identity } },
            { FeatureType::GameStateCloudSaving, { FeatureType::Identity } },
            { FeatureType::UserGameplayData, { FeatureType::Identity } }
            // Every feature has an implicit dependency on FeatureType::Main, which is enforced through the deployment processes
        };

        // Enabled GameKit features
        const std::unordered_set<FeatureType> m_availableFeatures { FeatureType::Main, FeatureType::Identity, FeatureType::Achievements, FeatureType::GameStateCloudSaving, FeatureType::UserGameplayData };

        // Status mappings
        const std::unordered_set<FeatureStatus> m_atRestStatuses { FeatureStatus::Deployed, FeatureStatus::Undeployed, FeatureStatus::Error, FeatureStatus::RollbackComplete };
        const std::unordered_set<FeatureStatus> m_featureUsableStatuses { FeatureStatus::Deployed, FeatureStatus::RollbackComplete };
        const std::unordered_set<FeatureStatus> m_createEnabledStatuses { FeatureStatus::Undeployed, FeatureStatus::Error };
        const std::unordered_set<FeatureStatus> m_redeployEnabledStatuses { FeatureStatus::Deployed, FeatureStatus::RollbackComplete, FeatureStatus::Error };
        const std::unordered_set<FeatureStatus> m_deleteEnabledStatuses { FeatureStatus::Deployed, FeatureStatus::RollbackComplete, FeatureStatus::Error };

        bool areUpstreamFeaturesDeployedAndUsable(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver = nullptr, CanExecuteDeploymentActionCallback callback = nullptr) const;

        std::unordered_set<FeatureType> getUndeployedUpstreamFeatures(FeatureType feature) const;
        std::unordered_set<FeatureType> getUnusableUpstreamFeatures(FeatureType feature) const;

        bool areCredentialsValid() const;

        bool isDeploymentStateValid(FeatureType feature, const std::function<bool(FeatureStatus)> statusHandler, DISPATCH_RECEIVER_HANDLE receiver = nullptr, CanExecuteDeploymentActionCallback callback = nullptr) const;

        bool isCreateStateValid(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver = nullptr, CanExecuteDeploymentActionCallback callback = nullptr) const;
        bool isRedeployStateValid(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver = nullptr, CanExecuteDeploymentActionCallback callback = nullptr) const;
        bool isDeleteStateValid(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver = nullptr, CanExecuteDeploymentActionCallback callback = nullptr) const;

        std::shared_ptr<GameKitAccount> getAccount();
        std::shared_ptr<GameKitFeatureResources> getFeatureResources(FeatureType feature);

        unsigned int deployFeature(FeatureType feature);
        unsigned int validateAndDeployFeature(FeatureType feature);
        unsigned int createOrRedeployFeatureAndMainStack(FeatureType feature, std::function<bool(FeatureType)> isFeatureStateValid);
        unsigned int validateFeatureSettings(FeatureType featureType) const;

        unsigned int invokeDeploymentResponseCallback(DISPATCH_RECEIVER_HANDLE receiver, DeploymentResponseCallback callback, unsigned int callStatus) const;
        bool invokeCanExecuteDeploymentActionCallback(DISPATCH_RECEIVER_HANDLE receiver, CanExecuteDeploymentActionCallback callback, FeatureType targetFeature, bool canExecuteAction, DeploymentActionBlockedReason reason, std::unordered_set<FeatureType> blockingFeatures = std::unordered_set<FeatureType>()) const;

    protected:
        void setFeatureStatus(FeatureType feature, FeatureStatus status);
        
        void setDeploymentInProgress(FeatureType feature, bool inProgress);
        std::unordered_set<FeatureType> getFeatureOrUpstreamDeploymentsInProgress(FeatureType feature) const;
        bool isFeatureOrUpstreamDeploymentInProgress(FeatureType feature) const;

        void setFeatureResources(FeatureType feature, std::shared_ptr<GameKitFeatureResources> featureResources);
        void setAccount(std::shared_ptr<GameKitAccount> account);

    public:
        GameKitDeploymentOrchestrator(const std::string& baseTemplatesFolder, const std::string& instanceFilesFolder, const std::string& sourceEngine, const std::string& pluginVersion, FuncLogCallback logCb);
        virtual ~GameKitDeploymentOrchestrator();

        virtual unsigned int SetCredentials(const AccountInfo& accountInfo, const AccountCredentials& accountCredentials);

        virtual FeatureStatus GetFeatureStatus(FeatureType feature) const;
        virtual FeatureStatusSummary GetFeatureStatusSummary(FeatureType feature) const;

        virtual bool IsFeatureDeploymentInProgress(FeatureType feature) const;
        virtual bool IsFeatureUpdating(FeatureType feature) const;
        virtual bool IsAnyFeatureUpdating() const;
        
        virtual unsigned int RefreshFeatureStatus(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver = nullptr, DeploymentResponseCallback callback = nullptr);
        virtual unsigned int RefreshFeatureStatuses(DISPATCH_RECEIVER_HANDLE receiver = nullptr, DeploymentResponseCallback callback = nullptr);
        
        virtual bool CanCreateFeature(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver = nullptr, CanExecuteDeploymentActionCallback callback = nullptr) const;
        virtual bool CanRedeployFeature(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver = nullptr, CanExecuteDeploymentActionCallback callback = nullptr) const;
        virtual bool CanDeleteFeature(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver = nullptr, CanExecuteDeploymentActionCallback callback = nullptr) const;

        virtual unsigned int CreateFeature(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver = nullptr, DeploymentResponseCallback callback = nullptr);
        virtual unsigned int RedeployFeature(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver = nullptr, DeploymentResponseCallback callback = nullptr);
        virtual unsigned int DeleteFeature(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver = nullptr, DeploymentResponseCallback callback = nullptr);

        virtual unsigned int DescribeFeatureResources(FeatureType feature, DISPATCH_RECEIVER_HANDLE receiver, DispatchedResourceInfoCallback callback);
    };
}