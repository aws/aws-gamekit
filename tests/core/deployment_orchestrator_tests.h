// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Tests
#include "test_common.h"
#include "testable_deployment_orchestrator.h"

namespace GameKit
{
    namespace Tests
    {
        namespace DeploymentOrchestrator
        {
            static const std::string BASE_TEMPLATES_FOLDER = "../core/test_data/sampleplugin/base";
            static const std::string INSTANCE_FILES_FOLDER = "../core/test_data/sampleplugin/instance";
            static const std::string UNKNOWN = "UNKNOWN";

            struct Dispatcher
            {
                std::unordered_map<FeatureType, FeatureStatus> featureStatuses;

                unsigned int callCount = 0;
                unsigned int callStatus = -1;
                std::vector<unsigned int> callStatuses;

                void CallbackHandler(const FeatureType* features, const FeatureStatus* statuses, unsigned int featureCount, unsigned int result)
                {
                    for (unsigned int i = 0; i < featureCount; i++)
                    {
                        featureStatuses[features[i]] = statuses[i];
                    }

                    ++callCount;

                    callStatus = result;
                    callStatuses.push_back(result);
                }

                FeatureType targetFeature;
                bool canExecuteAction;
                DeploymentActionBlockedReason blockedReason;
                std::unordered_set<FeatureType> blockingFeatures;

                void CallbackHandler(FeatureType feature, bool canExecute, DeploymentActionBlockedReason reason, const FeatureType* features, unsigned int featureCount)
                {
                    targetFeature = feature;
                    canExecuteAction = canExecute;
                    blockedReason = reason;
                    
                    blockingFeatures.clear();
                    for (unsigned int i = 0; i < featureCount; i++)
                    {
                        blockingFeatures.insert(features[i]);
                    }

                    ++callCount;
                }
            };

            class GameKitDeploymentOrchestratorTestFixture : public ::testing::Test
            {
            public:
                GameKitDeploymentOrchestratorTestFixture() {};
                virtual ~GameKitDeploymentOrchestratorTestFixture() override {};

                void SetUp() override;
                void TearDown() override;
            protected:
                // Test dependencies
                typedef TestLog<GameKitDeploymentOrchestratorTestFixture> TestLogger;
                TestStackInitializer testStack;

                // Mocks
                std::shared_ptr<MockGameKitAccount> accountMock;
                std::unordered_map<FeatureType, std::shared_ptr<MockGameKitFeatureResources>> featureResourcesMocks;

                // Features to test
                std::unordered_set<FeatureType> availableFeatures = { FeatureType::Main, FeatureType::Identity, FeatureType::Achievements, FeatureType::GameStateCloudSaving, FeatureType::UserGameplayData };

                // Target instance
                std::unique_ptr<TestableGameKitDeploymentOrchestrator> deploymentOrchestrator;

                // Helper methods
                std::shared_ptr<MockGameKitFeatureResources> getFeatureResourcesMock(FeatureType feature);
                void setUpFeatureForDeployment(FeatureType feature, bool isUndeployed, bool shouldInstanceFilesExist);
                void setFeatureStatus(FeatureType feature, FeatureStatus status);
                void setAllFeatureStatuses(FeatureStatus status);
                void setDeploymentInProgress(FeatureType feature, bool inProgress);
                bool isDeploymentInProgress(FeatureType feature);

                // Callback
                Dispatcher dispatcher;
                DeploymentResponseCallback deploymentResponseCallback = [](DISPATCH_RECEIVER_HANDLE dispatchReceiver, const FeatureType* features, const FeatureStatus* statuses, unsigned int featureCount, unsigned int result)
                {
                    static_cast<Dispatcher*>(dispatchReceiver)->CallbackHandler(features, statuses, featureCount, result);
                };

                CanExecuteDeploymentActionCallback canExecuteDeploymentActionCallback = [](DISPATCH_RECEIVER_HANDLE dispatchReceiver, FeatureType targetFeature, bool canExecute, DeploymentActionBlockedReason reason, const FeatureType* features, unsigned int featureCount)
                {
                    static_cast<Dispatcher*>(dispatchReceiver)->CallbackHandler(targetFeature, canExecute, reason, features, featureCount);
                };
            };
        }
    }
}
