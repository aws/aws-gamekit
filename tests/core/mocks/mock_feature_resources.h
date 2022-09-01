// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// GTest
#include <gmock/gmock.h>

// GameKit
#include <aws/gamekit/core/feature_resources.h>
#include <aws/gamekit/core/gamekit_account.h>

using namespace GameKit;

class MockGameKitFeatureResources : public GameKitFeatureResources
{
public:
    MockGameKitFeatureResources(const AccountInfo accountInfo, const AccountCredentials credentials, FeatureType featureType, FuncLogCallback logCb)
        : GameKitFeatureResources(accountInfo, credentials, featureType, logCb) {}
    ~MockGameKitFeatureResources() {}

    MOCK_METHOD(bool, IsCloudFormationInstanceTemplatePresent, (), (override, const));
    MOCK_METHOD(bool, AreLayerInstancesPresent, (), (override, const));
    MOCK_METHOD(bool, AreFunctionInstancesPresent, (), (override, const));

    MOCK_METHOD(unsigned int, SaveDeployedCloudFormationTemplate, (), (override, const));
    MOCK_METHOD(unsigned int, GetDeployedCloudFormationParameters, (DeployedParametersCallback callback), (override, const));
    MOCK_METHOD(unsigned int, SaveCloudFormationInstance, (), (override));
    MOCK_METHOD(unsigned int, SaveCloudFormationInstance, (std::string, std::string), (override));
    MOCK_METHOD(unsigned int, UpdateCloudFormationParameters, (), (override));
    MOCK_METHOD(unsigned int, SaveLayerInstances, (), (override, const));
    MOCK_METHOD(unsigned int, SaveFunctionInstances, (), (override, const));

    MOCK_METHOD(unsigned int, UploadDashboard, (const std::string& path), (override));
    MOCK_METHOD(unsigned int, UploadFeatureLayers, (), (override));
    MOCK_METHOD(unsigned int, UploadFeatureFunctions, (), (override));

    MOCK_METHOD(unsigned int, DeployFeatureLayers, (), (override));
    MOCK_METHOD(unsigned int, DeployFeatureFunctions, (), (override));

    MOCK_METHOD(std::string, GetCurrentStackStatus, (), (override, const));
    MOCK_METHOD(void, UpdateDashboardDeployStatus, (std::unordered_set<FeatureType>), (override, const));

    MOCK_METHOD(unsigned int, CreateOrUpdateFeatureStack, (), (override));
    MOCK_METHOD(unsigned int, DeleteFeatureStack, (), (override));
};