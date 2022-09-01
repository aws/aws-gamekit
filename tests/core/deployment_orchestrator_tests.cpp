// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// Tests
#include "deployment_orchestrator_tests.h"
#include "test_log.h"
#include "test_stack.h"

using namespace GameKit::Tests::DeploymentOrchestrator;
using namespace testing;

#pragma region Test Fixture

void GameKitDeploymentOrchestratorTestFixture::SetUp()
{
    ::testing::internal::CaptureStdout();
    TestLogger::Clear();
    testStack.Initialize();

    deploymentOrchestrator = std::make_unique<TestableGameKitDeploymentOrchestrator>(BASE_TEMPLATES_FOLDER, INSTANCE_FILES_FOLDER, UNKNOWN, UNKNOWN, TestLogger::Log);

    const AccountInfo accountInfo = { "dev", "123456789012", "TestCompany", "testgame" };
    const AccountCredentials accountCredentials = { "us-west-2", "AKIA...", "naRg8H..." };

    deploymentOrchestrator->SetCredentials(accountInfo, accountCredentials);

    for (FeatureType feature : availableFeatures)
    {
        featureResourcesMocks[feature] = std::make_shared<MockGameKitFeatureResources>(accountInfo, accountCredentials, feature, TestLogger::Log);
        featureResourcesMocks[feature]->SetPluginRoot(BASE_TEMPLATES_FOLDER);
        featureResourcesMocks[feature]->SetGameKitRoot(INSTANCE_FILES_FOLDER);

        deploymentOrchestrator->SetFeatureResources(feature, featureResourcesMocks[feature]);
    }

    accountMock = std::make_shared<MockGameKitAccount>(accountInfo, accountCredentials, TestLogger::Log);
    deploymentOrchestrator->SetAccount(accountMock);
}

void GameKitDeploymentOrchestratorTestFixture::TearDown()
{
    std::string capturedStdout = ::testing::internal::GetCapturedStdout();
    
    testStack.Cleanup();

    // Verify expectations on all feature resource mocks
    for (const std::pair<FeatureType, std::shared_ptr<GameKitFeatureResources>>& featureResourceMock : featureResourcesMocks)
    {
        ASSERT_TRUE(Mock::VerifyAndClearExpectations(featureResourceMock.second.get()));
    }

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(accountMock.get()));
}

std::shared_ptr<MockGameKitFeatureResources> GameKitDeploymentOrchestratorTestFixture::getFeatureResourcesMock(FeatureType featureType)
{
    return featureResourcesMocks[featureType];
}

void GameKitDeploymentOrchestratorTestFixture::setUpFeatureForDeployment(FeatureType feature, bool isUndeployed, bool shouldInstanceFilesExist)
{
    const std::shared_ptr<MockGameKitFeatureResources> featureResources = getFeatureResourcesMock(feature);

    // The following calls are only made for resource creation
    if (isUndeployed)
    {
        EXPECT_CALL(*featureResources, IsCloudFormationInstanceTemplatePresent()).WillOnce(Return(shouldInstanceFilesExist));
        EXPECT_CALL(*featureResources, AreLayerInstancesPresent()).WillOnce(Return(shouldInstanceFilesExist));
        EXPECT_CALL(*featureResources, AreFunctionInstancesPresent()).WillOnce(Return(shouldInstanceFilesExist));
        
        if (!shouldInstanceFilesExist)
        {
            EXPECT_CALL(*featureResources, SaveCloudFormationInstance()).WillOnce(Return(GAMEKIT_SUCCESS));
            EXPECT_CALL(*featureResources, SaveLayerInstances()).WillOnce(Return(GAMEKIT_SUCCESS));
            EXPECT_CALL(*featureResources, SaveFunctionInstances()).WillOnce(Return(GAMEKIT_SUCCESS));
        }
    }

    EXPECT_CALL(*featureResources, UploadDashboard(_)).WillOnce(Return(GAMEKIT_SUCCESS));
    EXPECT_CALL(*featureResources, DeployFeatureLayers()).WillOnce(Return(GAMEKIT_SUCCESS));
    EXPECT_CALL(*featureResources, DeployFeatureFunctions()).WillOnce(Return(GAMEKIT_SUCCESS));
    EXPECT_CALL(*featureResources, CreateOrUpdateFeatureStack()).WillOnce(Return(GAMEKIT_SUCCESS));
}

void GameKitDeploymentOrchestratorTestFixture::setFeatureStatus(FeatureType feature, FeatureStatus status)
{
    deploymentOrchestrator->SetFeatureStatus(feature, status);
}

void GameKitDeploymentOrchestratorTestFixture::setAllFeatureStatuses(FeatureStatus status)
{
    for (FeatureType feature : availableFeatures)
    {
        setFeatureStatus(feature, status);
    }
}

void GameKitDeploymentOrchestratorTestFixture::setDeploymentInProgress(FeatureType feature, bool inProgress)
{
    deploymentOrchestrator->SetDeploymentInProgress(feature, inProgress);
}

bool GameKitDeploymentOrchestratorTestFixture::isDeploymentInProgress(FeatureType feature)
{
    return deploymentOrchestrator->IsFeatureDeploymentInProgress(feature);
}
#pragma endregion

#pragma region SetCredentials
TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenUnknownRegion_SetCredentials_FailsToConvertShortRegionCode)
{
    // Act
    unsigned int result = deploymentOrchestrator->SetCredentials(
        AccountInfo{ "dev", "123456789012", "TestCompany", "testgame" },
        AccountCredentials{ "ab-cdef-5", "AKIA...", "naRg8H..." });

    // Assert
    ASSERT_EQ(result, GAMEKIT_ERROR_REGION_CODE_CONVERSION_FAILED);
}
#pragma endregion

#pragma region GetFeatureStatus
TEST_F(GameKitDeploymentOrchestratorTestFixture, WhenKnown_GetFeatureStatus_ReturnsCurrentStatus)
{
    // Arrange
    const FeatureType identity = FeatureType::Identity;
    const FeatureStatus deploying = FeatureStatus::DeployingResources;

    setFeatureStatus(identity, deploying);

    // Act
    FeatureStatus result = deploymentOrchestrator->GetFeatureStatus(identity);

    // Assert
    ASSERT_EQ(result, deploying);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, WhenUnknown_GetFeatureStatus_ReturnsUnknown)
{
    // Act
    FeatureStatus result = deploymentOrchestrator->GetFeatureStatus(FeatureType::Identity);

    // Assert
    ASSERT_EQ(result, FeatureStatus::Unknown);
}
#pragma endregion

#pragma region GetFeatureStatusSummary
TEST_F(GameKitDeploymentOrchestratorTestFixture, WhenKnown_GetFeatureStatusSummary_ConvertsSummaryProperly)
{
    // Arrange
    const FeatureType identity = FeatureType::Identity;
    setFeatureStatus(identity, FeatureStatus::DeployingResources);

    // Act
    FeatureStatusSummary result = deploymentOrchestrator->GetFeatureStatusSummary(identity);

    // Assert
    ASSERT_EQ(result, FeatureStatusSummary::Running);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, WhenUnknown_GetFeatureStatusSummary_ReturnsUnknown)
{
    // Act
    FeatureStatusSummary result = deploymentOrchestrator->GetFeatureStatusSummary(FeatureType::Identity);

    // Assert
    ASSERT_EQ(result, FeatureStatusSummary::Unknown);
}
#pragma endregion

#pragma region IsFeatureUpdating
TEST_F(GameKitDeploymentOrchestratorTestFixture, WhenNotUpdating_IsFeatureUpdating_ReturnsFalse)
{
    // Arrange
    const FeatureType identity = FeatureType::Identity;
    setFeatureStatus(identity, FeatureStatus::Deployed);

    // Act
    const bool result = deploymentOrchestrator->IsFeatureUpdating(identity);

    // Assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, WhenUpdating_IsFeatureUpdating_ReturnsTrue)
{
    // Arrange
    const FeatureType identity = FeatureType::Identity;
    setFeatureStatus(identity, FeatureStatus::DeployingResources);

    // Act
    const bool result = deploymentOrchestrator->IsFeatureUpdating(identity);

    // Assert
    ASSERT_TRUE(result);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, WhenUnknown_IsFeatureUpdating_ReturnsTrue)
{
    // Act
    const bool result = deploymentOrchestrator->IsFeatureUpdating(FeatureType::Identity);

    // Assert
    ASSERT_TRUE(result);
}
#pragma endregion

#pragma region IsAnyFeatureUpdating
TEST_F(GameKitDeploymentOrchestratorTestFixture, WhenNotUpdating_IsAnyFeatureUpdating_ReturnsFalse)
{
    // Arrange
    setAllFeatureStatuses(FeatureStatus::Deployed);

    // Act
    const bool result = deploymentOrchestrator->IsAnyFeatureUpdating();

    // Assert
    ASSERT_FALSE(result);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, WhenUpdating_IsAnyFeatureUpdating_ReturnsTrue)
{
    // Arrange
    const FeatureType identity = FeatureType::Identity;
    setFeatureStatus(identity, FeatureStatus::DeployingResources);

    // Act
    const bool result = deploymentOrchestrator->IsFeatureUpdating(identity);

    // Assert
    ASSERT_TRUE(result);
}
#pragma endregion

#pragma region RefreshFeatureStatus
TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenNoErrors_RefreshFeatureStatus_UpdatesStatus)
{
    // Arrange
    const FeatureType identity = FeatureType::Identity;
    setFeatureStatus(identity, FeatureStatus::Undeployed);
    
    EXPECT_CALL(*getFeatureResourcesMock(identity), GetCurrentStackStatus()).WillOnce(Return("COMPLETE"));

    // Act
    const unsigned int result = deploymentOrchestrator->RefreshFeatureStatus(identity, &dispatcher, deploymentResponseCallback);

    // Assert
    ASSERT_EQ(result, GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callCount, 1);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(identity), FeatureStatus::Deployed);
    ASSERT_EQ(dispatcher.featureStatuses[identity], FeatureStatus::Deployed);
}
#pragma endregion

#pragma region RefreshFeatureStatuses
TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenNoErrors_RefreshFeatureStatuses_UpdatesAllFeatureStatuses)
{
    // Arrange
    setAllFeatureStatuses(FeatureStatus::Unknown);

    for (FeatureType feature : availableFeatures)
    {
        EXPECT_CALL(*getFeatureResourcesMock(feature), GetCurrentStackStatus()).WillOnce(Return("COMPLETE"));
    }

    // Act
    const unsigned int result = deploymentOrchestrator->RefreshFeatureStatuses(&dispatcher, deploymentResponseCallback);

    // Assert
    ASSERT_EQ(result, GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callCount, 1);

    for (FeatureType feature : availableFeatures)
    {
        ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(feature), FeatureStatus::Deployed);
        ASSERT_EQ(dispatcher.featureStatuses[feature], FeatureStatus::Deployed);
    }
}
#pragma endregion

#pragma region CanCreateFeature
TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenFeatureStatus_CanCreateFeature_ReturnsCorrectResult)
{
    // Arrange
    const std::tuple<FeatureStatus, bool, DeploymentActionBlockedReason> testCases[] = 
    { 
        std::make_tuple(FeatureStatus::Deployed, false, DeploymentActionBlockedReason::FeatureMustBeDeleted),
        std::make_tuple(FeatureStatus::Undeployed, true, DeploymentActionBlockedReason::NotBlocked),
        std::make_tuple(FeatureStatus::Error, true, DeploymentActionBlockedReason::NotBlocked),
        std::make_tuple(FeatureStatus::Unknown, false, DeploymentActionBlockedReason::FeatureStatusIsUnknown)
    };

    setFeatureStatus(FeatureType::Main, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::Identity, FeatureStatus::Deployed);

    for (std::tuple<FeatureStatus, bool, DeploymentActionBlockedReason> testCase : testCases)
    {
        setFeatureStatus(FeatureType::GameStateCloudSaving, std::get<0>(testCase));

        // Act
        const bool result = deploymentOrchestrator->CanCreateFeature(FeatureType::GameStateCloudSaving, &dispatcher, canExecuteDeploymentActionCallback);

        // Assert
        ASSERT_EQ(result, std::get<1>(testCase));
        
        ASSERT_EQ(dispatcher.targetFeature, FeatureType::GameStateCloudSaving);
        ASSERT_EQ(dispatcher.blockedReason, std::get<2>(testCase));
        ASSERT_EQ(dispatcher.blockingFeatures.size(), 0);
    }
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenUpstreamStatus_CanCreateFeature_ReturnsCorrectResult)
{
    // Arrange
    const std::tuple<FeatureStatus, bool, DeploymentActionBlockedReason> testCases[] =
    {
        std::make_tuple(FeatureStatus::Deployed, true, DeploymentActionBlockedReason::NotBlocked),
        std::make_tuple(FeatureStatus::RollbackComplete, true, DeploymentActionBlockedReason::NotBlocked),
        std::make_tuple(FeatureStatus::Undeployed, false, DeploymentActionBlockedReason::DependenciesMustBeCreated),
        std::make_tuple(FeatureStatus::Error, false, DeploymentActionBlockedReason::DependenciesStatusIsInvalid),
        std::make_tuple(FeatureStatus::Unknown, false, DeploymentActionBlockedReason::DependenciesStatusIsInvalid)
    };

    setFeatureStatus(FeatureType::Main, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::GameStateCloudSaving, FeatureStatus::Undeployed);

    for (std::tuple<FeatureStatus, bool, DeploymentActionBlockedReason> testCase : testCases)
    {
        setFeatureStatus(FeatureType::Identity, std::get<0>(testCase));

        // Act
        const bool result = deploymentOrchestrator->CanCreateFeature(FeatureType::GameStateCloudSaving, &dispatcher, canExecuteDeploymentActionCallback);

        // Assert
        ASSERT_EQ(result, std::get<1>(testCase));

        ASSERT_EQ(dispatcher.targetFeature, FeatureType::GameStateCloudSaving);
        ASSERT_EQ(dispatcher.blockedReason, std::get<2>(testCase));

        // If there is an error, make sure the upstream feature is listed as the culprit dependency
        if (!std::get<1>(testCase))
        {
            ASSERT_EQ(dispatcher.blockingFeatures.size(), 1);
            ASSERT_TRUE(dispatcher.blockingFeatures.find(FeatureType::Identity) != dispatcher.blockingFeatures.end());
        }
        else
        {
            ASSERT_EQ(dispatcher.blockingFeatures.size(), 0);
        }
    }
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenInvalidCredentials_CanCreateFeature_ReturnsFalse)
{
    // Arrange
    deploymentOrchestrator = std::make_unique<TestableGameKitDeploymentOrchestrator>(BASE_TEMPLATES_FOLDER,
        INSTANCE_FILES_FOLDER, UNKNOWN, UNKNOWN,
        TestLogger::Log);

    setFeatureStatus(FeatureType::Main, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::Identity, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::GameStateCloudSaving, FeatureStatus::Undeployed);

    // Act
    const bool result = deploymentOrchestrator->CanCreateFeature(FeatureType::GameStateCloudSaving, &dispatcher, canExecuteDeploymentActionCallback);

    // Assert
    ASSERT_FALSE(result);

    ASSERT_EQ(dispatcher.targetFeature, FeatureType::GameStateCloudSaving);
    ASSERT_EQ(dispatcher.blockedReason, DeploymentActionBlockedReason::CredentialsInvalid);
    ASSERT_EQ(dispatcher.blockingFeatures.size(), 0);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, WhileDeploying_CanCreateFeature_ReturnsFalse)
{
    // Arrange
    setFeatureStatus(FeatureType::Main, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::Identity, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::GameStateCloudSaving, FeatureStatus::Undeployed);
    setDeploymentInProgress(FeatureType::GameStateCloudSaving, true);

    // Act
    const bool result = deploymentOrchestrator->CanCreateFeature(FeatureType::GameStateCloudSaving, &dispatcher, canExecuteDeploymentActionCallback);

    // Assert
    ASSERT_FALSE(result);

    ASSERT_EQ(dispatcher.targetFeature, FeatureType::GameStateCloudSaving);
    ASSERT_EQ(dispatcher.blockedReason, DeploymentActionBlockedReason::OngoingDeployments);
    ASSERT_EQ(dispatcher.blockingFeatures.size(), 1);
    ASSERT_TRUE(dispatcher.blockingFeatures.find(FeatureType::GameStateCloudSaving) != dispatcher.blockingFeatures.end());
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, WhileUpstreamDeploying_CanCreateFeature_ReturnsFalse)
{
    // Arrange
    setFeatureStatus(FeatureType::Main, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::Identity, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::GameStateCloudSaving, FeatureStatus::Undeployed);
    setDeploymentInProgress(FeatureType::Identity, true);

    // Act
    const bool result = deploymentOrchestrator->CanCreateFeature(FeatureType::GameStateCloudSaving, &dispatcher, canExecuteDeploymentActionCallback);

    // Assert
    ASSERT_FALSE(result);

    ASSERT_EQ(dispatcher.targetFeature, FeatureType::GameStateCloudSaving);
    ASSERT_EQ(dispatcher.blockedReason, DeploymentActionBlockedReason::OngoingDeployments);
    ASSERT_EQ(dispatcher.blockingFeatures.size(), 1);
    ASSERT_TRUE(dispatcher.blockingFeatures.find(FeatureType::Identity) != dispatcher.blockingFeatures.end());
}
#pragma endregion

#pragma region CanRedeployFeature
TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenFeatureStatus_CanRedeployFeature_ReturnsCorrectResult)
{
    // Arrange
    const std::tuple<FeatureStatus, bool, DeploymentActionBlockedReason> testCases[] = 
    { 
        std::make_tuple(FeatureStatus::Deployed, true, DeploymentActionBlockedReason::NotBlocked),
        std::make_tuple(FeatureStatus::RollbackComplete, true, DeploymentActionBlockedReason::NotBlocked),
        std::make_tuple(FeatureStatus::Error, true, DeploymentActionBlockedReason::NotBlocked),
        std::make_tuple(FeatureStatus::Undeployed, false, DeploymentActionBlockedReason::FeatureMustBeCreated),
        std::make_tuple(FeatureStatus::Unknown, false, DeploymentActionBlockedReason::FeatureStatusIsUnknown)
    };

    setFeatureStatus(FeatureType::Main, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::Identity, FeatureStatus::Deployed);

    for (std::tuple<FeatureStatus, bool, DeploymentActionBlockedReason> testCase : testCases)
    {
        setFeatureStatus(FeatureType::GameStateCloudSaving, std::get<0>(testCase));

        // Act
        const bool result = deploymentOrchestrator->CanRedeployFeature(FeatureType::GameStateCloudSaving, &dispatcher, canExecuteDeploymentActionCallback);

        // Assert
        ASSERT_EQ(result, std::get<1>(testCase));

        ASSERT_EQ(dispatcher.targetFeature, FeatureType::GameStateCloudSaving);
        ASSERT_EQ(dispatcher.blockedReason, std::get<2>(testCase));
        ASSERT_EQ(dispatcher.blockingFeatures.size(), 0);
    }
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenUpstreamStatus_CanRedeployFeature_ReturnsCorrectResult)
{
    // Arrange
    const std::tuple<FeatureStatus, bool, DeploymentActionBlockedReason> testCases[] =
    {
        std::make_tuple(FeatureStatus::Deployed, true, DeploymentActionBlockedReason::NotBlocked),
        std::make_tuple(FeatureStatus::RollbackComplete, true, DeploymentActionBlockedReason::NotBlocked),
        std::make_tuple(FeatureStatus::Undeployed, false, DeploymentActionBlockedReason::DependenciesMustBeCreated),
        std::make_tuple(FeatureStatus::Error, false, DeploymentActionBlockedReason::DependenciesStatusIsInvalid),
        std::make_tuple(FeatureStatus::Unknown, false, DeploymentActionBlockedReason::DependenciesStatusIsInvalid)
    };

    setFeatureStatus(FeatureType::Main, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::GameStateCloudSaving, FeatureStatus::Deployed);

    for (std::tuple<FeatureStatus, bool, DeploymentActionBlockedReason> testCase : testCases)
    {
        setFeatureStatus(FeatureType::Identity, std::get<0>(testCase));

        // Act
        const bool result = deploymentOrchestrator->CanRedeployFeature(FeatureType::GameStateCloudSaving, &dispatcher, canExecuteDeploymentActionCallback);

        // Assert
        ASSERT_EQ(result, std::get<1>(testCase));

        ASSERT_EQ(dispatcher.targetFeature, FeatureType::GameStateCloudSaving);
        ASSERT_EQ(dispatcher.blockedReason, std::get<2>(testCase));

        // If there is an error, make sure the upstream feature is listed as the culprit dependency
        if (!std::get<1>(testCase))
        {
            ASSERT_EQ(dispatcher.blockingFeatures.size(), 1);
            ASSERT_TRUE(dispatcher.blockingFeatures.find(FeatureType::Identity) != dispatcher.blockingFeatures.end());
        }
        else
        {
            ASSERT_EQ(dispatcher.blockingFeatures.size(), 0);
        }
    }
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenInvalidCredentials_CanRedeployFeature_ReturnsFalse)
{
    // Arrange
    deploymentOrchestrator = std::make_unique<TestableGameKitDeploymentOrchestrator>(BASE_TEMPLATES_FOLDER,
        INSTANCE_FILES_FOLDER, UNKNOWN, UNKNOWN,
        TestLogger::Log);

    setFeatureStatus(FeatureType::Main, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::Identity, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::GameStateCloudSaving, FeatureStatus::Deployed);

    // Act
    const bool result = deploymentOrchestrator->CanRedeployFeature(FeatureType::GameStateCloudSaving, &dispatcher, canExecuteDeploymentActionCallback);

    // Assert
    ASSERT_FALSE(result);

    ASSERT_EQ(dispatcher.targetFeature, FeatureType::GameStateCloudSaving);
    ASSERT_EQ(dispatcher.blockedReason, DeploymentActionBlockedReason::CredentialsInvalid);
    ASSERT_EQ(dispatcher.blockingFeatures.size(), 0);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, WhileDeploying_CanRedeployFeature_ReturnsFalse)
{
    // Arrange
    setFeatureStatus(FeatureType::Main, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::Identity, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::GameStateCloudSaving, FeatureStatus::Undeployed);
    setDeploymentInProgress(FeatureType::GameStateCloudSaving, true);

    // Act
    const bool result = deploymentOrchestrator->CanRedeployFeature(FeatureType::GameStateCloudSaving, &dispatcher, canExecuteDeploymentActionCallback);

    // Assert
    ASSERT_FALSE(result);

    ASSERT_EQ(dispatcher.targetFeature, FeatureType::GameStateCloudSaving);
    ASSERT_EQ(dispatcher.blockedReason, DeploymentActionBlockedReason::OngoingDeployments);
    ASSERT_EQ(dispatcher.blockingFeatures.size(), 1);
    ASSERT_TRUE(dispatcher.blockingFeatures.find(FeatureType::GameStateCloudSaving) != dispatcher.blockingFeatures.end());
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, WhileUpstreamDeploying_CanRedeployFeature_ReturnsFalse)
{
    // Arrange
    setFeatureStatus(FeatureType::Main, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::Identity, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::GameStateCloudSaving, FeatureStatus::Deployed);
    setDeploymentInProgress(FeatureType::Identity, true);

    // Act
    const bool result = deploymentOrchestrator->CanRedeployFeature(FeatureType::GameStateCloudSaving, &dispatcher, canExecuteDeploymentActionCallback);

    // Assert
    ASSERT_FALSE(result);

    ASSERT_EQ(dispatcher.targetFeature, FeatureType::GameStateCloudSaving);
    ASSERT_EQ(dispatcher.blockedReason, DeploymentActionBlockedReason::OngoingDeployments);
    ASSERT_EQ(dispatcher.blockingFeatures.size(), 1);
    ASSERT_TRUE(dispatcher.blockingFeatures.find(FeatureType::Identity) != dispatcher.blockingFeatures.end());
}
#pragma endregion

#pragma region CanDeleteFeature
TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenDownstreamStatus_CanDeleteFeature_ReturnsCorrectResult)
{
    // Arrange
    const std::tuple<FeatureStatus, bool, DeploymentActionBlockedReason> testCases[] = 
    { 
        std::make_tuple(FeatureStatus::Deployed, false, DeploymentActionBlockedReason::DependenciesMustBeDeleted),
        std::make_tuple(FeatureStatus::Undeployed, true, DeploymentActionBlockedReason::NotBlocked)
    };

    // Ensure that Game Saving is the only dependency consuming Identity
    setAllFeatureStatuses(FeatureStatus::Undeployed);
    setFeatureStatus(FeatureType::Main, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::Identity, FeatureStatus::Deployed);

    for (std::tuple<FeatureStatus, bool, DeploymentActionBlockedReason> testCase : testCases)
    {
        setFeatureStatus(FeatureType::GameStateCloudSaving, std::get<0>(testCase));

        // Act
        const bool result = deploymentOrchestrator->CanDeleteFeature(FeatureType::Identity, &dispatcher, canExecuteDeploymentActionCallback);

        // Assert
        ASSERT_EQ(result, std::get<1>(testCase));

        ASSERT_EQ(dispatcher.targetFeature, FeatureType::Identity);
        ASSERT_EQ(dispatcher.blockedReason, std::get<2>(testCase));
        // If blocked, ensure downstream is listed as culprit dependency
        if (!std::get<1>(testCase))
        {
            ASSERT_EQ(dispatcher.blockingFeatures.size(), 1);
            ASSERT_TRUE(dispatcher.blockingFeatures.find(FeatureType::GameStateCloudSaving) != dispatcher.blockingFeatures.end());
        }
        else
        {
            ASSERT_EQ(dispatcher.blockingFeatures.size(), 0);
        }
    }
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenFeatureStatus_CanDeleteFeature_ReturnsCorrectResult)
{
    // Arrange
    const std::tuple<FeatureStatus, bool, DeploymentActionBlockedReason> testCases[] =
    { 
        std::make_tuple(FeatureStatus::Deployed, true, DeploymentActionBlockedReason::NotBlocked),
        std::make_tuple(FeatureStatus::RollbackComplete, true, DeploymentActionBlockedReason::NotBlocked),
        std::make_tuple(FeatureStatus::Error, true, DeploymentActionBlockedReason::NotBlocked),
        std::make_tuple(FeatureStatus::Undeployed, false, DeploymentActionBlockedReason::FeatureMustBeCreated),
        std::make_tuple(FeatureStatus::Unknown, false, DeploymentActionBlockedReason::FeatureStatusIsUnknown)
    };

    // Ensure that Game Saving is the only dependency consuming Identity
    setAllFeatureStatuses(FeatureStatus::Undeployed);
    setFeatureStatus(FeatureType::Main, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::GameStateCloudSaving, FeatureStatus::Undeployed);

    for (std::tuple<FeatureStatus, bool, DeploymentActionBlockedReason> testCase : testCases)
    {
        setFeatureStatus(FeatureType::Identity, std::get<0>(testCase));

        // Act
        const bool result = deploymentOrchestrator->CanDeleteFeature(FeatureType::Identity, &dispatcher, canExecuteDeploymentActionCallback);

        // Assert
        ASSERT_EQ(result, std::get<1>(testCase));

        ASSERT_EQ(dispatcher.targetFeature, FeatureType::Identity);
        ASSERT_EQ(dispatcher.blockedReason, std::get<2>(testCase));
        ASSERT_EQ(dispatcher.blockingFeatures.size(), 0);
    }
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenInvalidCredentials_CanDeleteFeature_ReturnsFalse)
{
    // Arrange
    deploymentOrchestrator = std::make_unique<TestableGameKitDeploymentOrchestrator>(BASE_TEMPLATES_FOLDER,
        INSTANCE_FILES_FOLDER, UNKNOWN, UNKNOWN,
        TestLogger::Log);

    setFeatureStatus(FeatureType::Main, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::Identity, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::GameStateCloudSaving, FeatureStatus::Deployed);

    // Act
    const bool result = deploymentOrchestrator->CanDeleteFeature(FeatureType::GameStateCloudSaving, &dispatcher, canExecuteDeploymentActionCallback);

    // Assert
    ASSERT_FALSE(result);

    ASSERT_EQ(dispatcher.targetFeature, FeatureType::GameStateCloudSaving);
    ASSERT_EQ(dispatcher.blockedReason, DeploymentActionBlockedReason::CredentialsInvalid);
    ASSERT_EQ(dispatcher.blockingFeatures.size(), 0);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, WhileDeploying_CanDeleteFeature_ReturnsFalse)
{
    // Arrange
    setFeatureStatus(FeatureType::Main, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::Identity, FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::GameStateCloudSaving, FeatureStatus::Undeployed);
    setDeploymentInProgress(FeatureType::GameStateCloudSaving, true);

    // Act
    const bool result = deploymentOrchestrator->CanDeleteFeature(FeatureType::GameStateCloudSaving, &dispatcher, canExecuteDeploymentActionCallback);

    // Assert
    ASSERT_FALSE(result);

    ASSERT_EQ(dispatcher.targetFeature, FeatureType::GameStateCloudSaving);
    ASSERT_EQ(dispatcher.blockedReason, DeploymentActionBlockedReason::OngoingDeployments);
    ASSERT_EQ(dispatcher.blockingFeatures.size(), 1);
    ASSERT_TRUE(dispatcher.blockingFeatures.find(FeatureType::GameStateCloudSaving) != dispatcher.blockingFeatures.end());
}
#pragma endregion

#pragma region CreateFeature
TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenNoInstanceFiles_CreateFeature_CopiesInstanceFilesAndCreatesFeature)
{
    // Arrange
    // Clean slate - everything is undeployed
    setAllFeatureStatuses(FeatureStatus::Undeployed);
    for (FeatureType feature : availableFeatures)
    {
        const std::shared_ptr<MockGameKitFeatureResources> featureResources = getFeatureResourcesMock(feature);
        EXPECT_CALL(*featureResources, GetCurrentStackStatus()).WillOnce(Return("UNDEPLOYED"));
    }

    setUpFeatureForDeployment(FeatureType::Main, false, false);
    setUpFeatureForDeployment(FeatureType::Identity, false, false);

    EXPECT_CALL(*accountMock, DeployApiGatewayStage()).Times(2);

    // Act
    const unsigned int result = deploymentOrchestrator->CreateFeature(FeatureType::Identity, &dispatcher, deploymentResponseCallback);

    // Assert
    ASSERT_EQ(result, GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callCount, 1);
    
    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Main), FeatureStatus::Deployed);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Main), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Main], FeatureStatus::Deployed);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Identity), FeatureStatus::Deployed);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Identity), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Identity], FeatureStatus::Deployed);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenMainStackExists_CreateFeature_DeploysFeatureAndRedeploysMainStack)
{
    // Arrange
    // Main deployed, all other features undeployed
    setAllFeatureStatuses(FeatureStatus::Undeployed);
    setFeatureStatus(FeatureType::Main, FeatureStatus::Deployed);
    for (FeatureType feature : availableFeatures)
    {
        const std::shared_ptr<MockGameKitFeatureResources> featureResources = getFeatureResourcesMock(feature);
        if (feature == FeatureType::Main)
        {
            EXPECT_CALL(*featureResources, GetCurrentStackStatus()).WillOnce(Return("COMPLETE"));
        }
        else
        {
            EXPECT_CALL(*featureResources, GetCurrentStackStatus()).WillOnce(Return("UNDEPLOYED"));
        }
    }

    setUpFeatureForDeployment(FeatureType::Main, false, false);
    // For test coverage, assume that Identity isn't deployed /but/ the instance files already exist
    setUpFeatureForDeployment(FeatureType::Identity, true, true);

    EXPECT_CALL(*accountMock, DeployApiGatewayStage()).Times(2);

    // Act
    const unsigned int result = deploymentOrchestrator->CreateFeature(FeatureType::Identity, &dispatcher, deploymentResponseCallback);

    // Assert
    ASSERT_EQ(result, GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callCount, 1);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Main), FeatureStatus::Deployed);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Main), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Main], FeatureStatus::Deployed);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Identity), FeatureStatus::Deployed);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Identity), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Identity], FeatureStatus::Deployed);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenCannotCreateAfterStatusRefresh_CreateFeature_FailsAndDoesNotDeploy)
{
    // Arrange
    setAllFeatureStatuses(FeatureStatus::Undeployed);
    for (FeatureType feature : availableFeatures)
    {
        const std::shared_ptr<MockGameKitFeatureResources> featureResources = getFeatureResourcesMock(feature);
        if (feature == FeatureType::Main)
        {
            // Main will be marked as running after status is refreshed
            EXPECT_CALL(*featureResources, GetCurrentStackStatus()).WillOnce(Return("IN_PROGRESS"));
        }
        else
        {
            EXPECT_CALL(*featureResources, GetCurrentStackStatus()).WillOnce(Return("UNDEPLOYED"));
        }
    }

    // Act
    const unsigned int result = deploymentOrchestrator->CreateFeature(FeatureType::Identity, &dispatcher, deploymentResponseCallback);

    // Assert
    ASSERT_EQ(result, GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE);
    ASSERT_EQ(dispatcher.callStatus, GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE);
    ASSERT_EQ(dispatcher.callCount, 1);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Main), FeatureStatus::Running);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Main), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Main], FeatureStatus::Running);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Identity), FeatureStatus::Undeployed);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Identity), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Identity], FeatureStatus::Undeployed);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenCannotCreateFeature_CreateFeature_FailsAndDoesNotDeploy)
{
    // Arrange
    // Main deployed, all other features undeployed
    setAllFeatureStatuses(FeatureStatus::Undeployed);
    setFeatureStatus(FeatureType::Main, FeatureStatus::DeployingResources);
    setDeploymentInProgress(FeatureType::Main, true);

    // Act
    const unsigned int result = deploymentOrchestrator->CreateFeature(FeatureType::Identity, &dispatcher, deploymentResponseCallback);

    // Assert
    ASSERT_EQ(result, GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE);
    ASSERT_EQ(dispatcher.callStatus, GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE);
    ASSERT_EQ(dispatcher.callCount, 1);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Main), FeatureStatus::DeployingResources);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Main), true);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Main], FeatureStatus::DeployingResources);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Identity), FeatureStatus::Undeployed);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Identity), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Identity], FeatureStatus::Undeployed);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenFeatureResourcesThrowsAnError_CreateFeature_FailsAndDoesNotDeploy)
{
    // Arrange
    // Clean slate - everything is undeployed
    setAllFeatureStatuses(FeatureStatus::Undeployed);
    for (FeatureType feature : availableFeatures)
    {
        const std::shared_ptr<MockGameKitFeatureResources> featureResources = getFeatureResourcesMock(feature);
        EXPECT_CALL(*featureResources, GetCurrentStackStatus()).WillOnce(Return("UNDEPLOYED"));
    }

    const std::shared_ptr<MockGameKitFeatureResources> mainResources = getFeatureResourcesMock(FeatureType::Main);

    EXPECT_CALL(*mainResources, IsCloudFormationInstanceTemplatePresent()).WillOnce(Return(false));
    EXPECT_CALL(*mainResources, SaveCloudFormationInstance(UNKNOWN, UNKNOWN)).WillOnce(Return(GAMEKIT_ERROR_PARAMETERS_FILE_SAVE_FAILED));

    // Act
    const unsigned int result = deploymentOrchestrator->CreateFeature(FeatureType::Identity, &dispatcher, deploymentResponseCallback);

    // Assert
    ASSERT_EQ(result, GAMEKIT_ERROR_PARAMETERS_FILE_SAVE_FAILED);
    ASSERT_EQ(dispatcher.callStatus, GAMEKIT_ERROR_PARAMETERS_FILE_SAVE_FAILED);
    ASSERT_EQ(dispatcher.callCount, 1);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Main), FeatureStatus::Error);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Main), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Main], FeatureStatus::Error);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Identity), FeatureStatus::Undeployed);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Identity), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Identity], FeatureStatus::Undeployed);
}
#pragma endregion

#pragma region RedeployFeature
TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenStacksExist_RedeployFeature_RedeploysMainAndFeatureStack)
{
    // Arrange
    // All deployed
    setAllFeatureStatuses(FeatureStatus::Deployed);
    for (FeatureType feature : availableFeatures)
    {
        const std::shared_ptr<MockGameKitFeatureResources> featureResources = getFeatureResourcesMock(feature);
        EXPECT_CALL(*featureResources, GetCurrentStackStatus()).WillOnce(Return("COMPLETE"));
    }

    setUpFeatureForDeployment(FeatureType::Main, false, true);
    setUpFeatureForDeployment(FeatureType::Identity, false, true);

    EXPECT_CALL(*accountMock, DeployApiGatewayStage()).Times(2);

    // Act
    const unsigned int result = deploymentOrchestrator->RedeployFeature(FeatureType::Identity, &dispatcher, deploymentResponseCallback);

    // Assert
    ASSERT_EQ(result, GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callCount, 1);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Main), FeatureStatus::Deployed);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Main), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Main], FeatureStatus::Deployed);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Identity), FeatureStatus::Deployed);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Identity), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Identity], FeatureStatus::Deployed);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenFeatureStackInError_RedeployFeature_RedeploysMainAndFeatureStack)
{
    // Arrange
    // All deployed, game saving in error
    setAllFeatureStatuses(FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::GameStateCloudSaving, FeatureStatus::Error);
    for (FeatureType feature : availableFeatures)
    {
        const std::shared_ptr<MockGameKitFeatureResources> featureResources = getFeatureResourcesMock(feature);
        if (feature == FeatureType::GameStateCloudSaving)
        {
            EXPECT_CALL(*featureResources, GetCurrentStackStatus()).WillOnce(Return("FAILED"));
        }
        else
        {
            EXPECT_CALL(*featureResources, GetCurrentStackStatus()).WillOnce(Return("COMPLETE"));
        }
    }

    setUpFeatureForDeployment(FeatureType::Main, false, true);
    setUpFeatureForDeployment(FeatureType::GameStateCloudSaving, false, true);

    EXPECT_CALL(*accountMock, DeployApiGatewayStage()).Times(2);

    // Act
    const unsigned int result = deploymentOrchestrator->RedeployFeature(FeatureType::GameStateCloudSaving, &dispatcher, deploymentResponseCallback);

    // Assert
    ASSERT_EQ(result, GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callCount, 1);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Main), FeatureStatus::Deployed);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Main), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Main], FeatureStatus::Deployed);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::GameStateCloudSaving), FeatureStatus::Deployed);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::GameStateCloudSaving), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::GameStateCloudSaving], FeatureStatus::Deployed);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenCannotRedeployFeature_RedeployFeature_FailsAndDoesNotDeploy)
{
    // Arrange
    // All deployed, aside from main which is currently deploying
    setAllFeatureStatuses(FeatureStatus::Deployed);
    setFeatureStatus(FeatureType::Main, FeatureStatus::DeployingResources);
    setDeploymentInProgress(FeatureType::Main, true);

    // Act
    const unsigned int result = deploymentOrchestrator->RedeployFeature(FeatureType::Identity, &dispatcher, deploymentResponseCallback);

    // Assert
    ASSERT_EQ(result, GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE);
    ASSERT_EQ(dispatcher.callStatus, GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE);
    ASSERT_EQ(dispatcher.callCount, 1);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Main), FeatureStatus::DeployingResources);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Main), true);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Main], FeatureStatus::DeployingResources);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Identity), FeatureStatus::Deployed);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Identity), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Identity], FeatureStatus::Deployed);
}
#pragma endregion

#pragma region DeleteFeature
TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenCannotDeleteFeature_DeleteFeature_FailsAndDoesNotDelete)
{
    // Arrange
    // All deployed, can't delete Identity as the others depend on it
    setAllFeatureStatuses(FeatureStatus::Deployed);

    // Act
    const unsigned int result = deploymentOrchestrator->DeleteFeature(FeatureType::Identity, &dispatcher, deploymentResponseCallback);

    // Assert
    ASSERT_EQ(result, GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE);
    ASSERT_EQ(dispatcher.callStatus, GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE);
    ASSERT_EQ(dispatcher.callCount, 1);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::Identity), FeatureStatus::Deployed);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::Identity), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::Identity], FeatureStatus::Deployed);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenCannotDeleteFeatureAfterStatusRefresh_DeleteFeature_FailsAndDoesNotDelete)
{
    // Arrange
    setAllFeatureStatuses(FeatureStatus::Deployed);
    for (FeatureType feature : availableFeatures)
    {
        const std::shared_ptr<MockGameKitFeatureResources> featureResources = getFeatureResourcesMock(feature);
        if (feature == FeatureType::GameStateCloudSaving)
        {
            EXPECT_CALL(*featureResources, GetCurrentStackStatus()).WillOnce(Return("IN_PROGRESS"));
        }
        else
        {
            EXPECT_CALL(*featureResources, GetCurrentStackStatus()).WillOnce(Return("COMPLETE"));
        }
    }

    // Act
    const unsigned int result = deploymentOrchestrator->DeleteFeature(FeatureType::GameStateCloudSaving, &dispatcher, deploymentResponseCallback);

    // Assert
    ASSERT_EQ(result, GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE);
    ASSERT_EQ(dispatcher.callStatus, GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE);
    ASSERT_EQ(dispatcher.callCount, 1);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::GameStateCloudSaving), FeatureStatus::Running);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::GameStateCloudSaving), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::GameStateCloudSaving], FeatureStatus::Running);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenCanDeleteFeature_DeleteFeature_DeletesFeature)
{
    // Arrange
    setAllFeatureStatuses(FeatureStatus::Deployed);
    for (FeatureType feature : availableFeatures)
    {
        const std::shared_ptr<MockGameKitFeatureResources> featureResources = getFeatureResourcesMock(feature);
        EXPECT_CALL(*featureResources, GetCurrentStackStatus()).WillOnce(Return("COMPLETE"));
    }

    const std::shared_ptr<MockGameKitFeatureResources> gameSavingResources = getFeatureResourcesMock(FeatureType::GameStateCloudSaving);
    EXPECT_CALL(*gameSavingResources, DeleteFeatureStack()).WillOnce(Return(GAMEKIT_SUCCESS));

    // Act
    const unsigned int result = deploymentOrchestrator->DeleteFeature(FeatureType::GameStateCloudSaving, &dispatcher, deploymentResponseCallback);

    // Assert
    ASSERT_EQ(result, GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(dispatcher.callCount, 1);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::GameStateCloudSaving), FeatureStatus::Undeployed);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::GameStateCloudSaving), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::GameStateCloudSaving], FeatureStatus::Undeployed);
}

TEST_F(GameKitDeploymentOrchestratorTestFixture, GivenDeleteThrowsAnError_DeleteFeature_ReturnsErrorAndDoesNotDeleteFeature)
{
    // Arrange
    setAllFeatureStatuses(FeatureStatus::Deployed);
    for (FeatureType feature : availableFeatures)
    {
        const std::shared_ptr<MockGameKitFeatureResources> featureResources = getFeatureResourcesMock(feature);
        EXPECT_CALL(*featureResources, GetCurrentStackStatus()).WillOnce(Return("COMPLETE"));
    }

    const std::shared_ptr<MockGameKitFeatureResources> gameSavingResources = getFeatureResourcesMock(FeatureType::GameStateCloudSaving);
    EXPECT_CALL(*gameSavingResources, DeleteFeatureStack()).WillOnce(Return(GAMEKIT_ERROR_CLOUDFORMATION_STACK_DELETE_FAILED));

    // Act
    const unsigned int result = deploymentOrchestrator->DeleteFeature(FeatureType::GameStateCloudSaving, &dispatcher, deploymentResponseCallback);

    // Assert
    ASSERT_EQ(result, GAMEKIT_ERROR_CLOUDFORMATION_STACK_DELETE_FAILED);
    ASSERT_EQ(dispatcher.callStatus, GAMEKIT_ERROR_CLOUDFORMATION_STACK_DELETE_FAILED);
    ASSERT_EQ(dispatcher.callCount, 1);

    ASSERT_EQ(deploymentOrchestrator->GetFeatureStatus(FeatureType::GameStateCloudSaving), FeatureStatus::Error);
    ASSERT_EQ(isDeploymentInProgress(FeatureType::GameStateCloudSaving), false);
    ASSERT_EQ(dispatcher.featureStatuses[FeatureType::GameStateCloudSaving], FeatureStatus::Error);
}
#pragma endregion