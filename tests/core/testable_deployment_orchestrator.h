// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// GameKit
#include <aws/gamekit/core/deployment_orchestrator.h>

namespace GameKit
{
    namespace Tests
    {
        namespace DeploymentOrchestrator
        {
            class TestableGameKitDeploymentOrchestrator : public GameKitDeploymentOrchestrator
            {
            public:
                TestableGameKitDeploymentOrchestrator(const std::string& baseTemplatesFolder, const std::string& instanceFilesFolder, const std::string& sourceEngine, const std::string& pluginVersion, FuncLogCallback logCb)
                    : GameKitDeploymentOrchestrator(baseTemplatesFolder, instanceFilesFolder, sourceEngine, pluginVersion, logCb) { }

                void SetFeatureStatus(FeatureType feature, FeatureStatus status)
                {
                    setFeatureStatus(feature, status);
                }

                void SetDeploymentInProgress(FeatureType feature, bool inProgress)
                {
                    setDeploymentInProgress(feature, inProgress);
                }

                bool IsFeatureOrUpstreamDeploymentInProgress(FeatureType feature) const
                {
                    return isFeatureOrUpstreamDeploymentInProgress(feature);
                }

                void SetFeatureResources(FeatureType feature, std::shared_ptr<GameKitFeatureResources> featureResources)
                {
                    setFeatureResources(feature, featureResources);
                }

                void SetAccount(std::shared_ptr<GameKitAccount> account)
                {
                    setAccount(account);
                }
            };
        }
    }
}