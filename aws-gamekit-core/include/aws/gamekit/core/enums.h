// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

/** @file
 * @brief aws-gamekit-core enums.
 */

#include <string>

#pragma once
namespace GameKit
{
    enum class TokenType { AccessToken = 0, RefreshToken, IdToken, IamSessionToken, TokenType_COUNT /* TokenType_COUNT must be the last item in this enum */ };
    enum class FeatureType { Main, Identity, Authentication, Achievements, GameStateCloudSaving, UserGameplayData };
    enum class TemplateType { Base, Instance };
    enum class EnvironmentType { Development = 0, QA = 1, Staging = 2, Production = 3, Custom = 4 };
    enum class FederatedIdentityProvider { Facebook = 0, Google = 1, Apple = 2, Amazon = 3 };
    enum class FeatureStatus { Deployed = 0, Undeployed, Error, RollbackComplete, Running, GeneratingTemplates, UploadingDashboards, UploadingLayers, UploadingFunctions, DeployingResources, DeletingResources, Unknown };
    enum class FeatureStatusSummary { Deployed = 0, Undeployed, Error, Running, Unknown };
    enum class DeploymentActionBlockedReason { NotBlocked = 0, FeatureMustBeCreated, FeatureMustBeDeleted, FeatureStatusIsUnknown, OngoingDeployments, DependenciesMustBeCreated, DependenciesMustBeDeleted, DependenciesStatusIsInvalid, CredentialsInvalid, MainStackNotReady };

    inline std::string GetFeatureStatusString(FeatureStatus status)
    {
        switch (status)
        {
        case FeatureStatus::Deployed: return "Deployed";
        case FeatureStatus::Undeployed: return "Undeployed";
        case FeatureStatus::Error: return "Error";
        case FeatureStatus::RollbackComplete: return "Rollback Complete";
        case FeatureStatus::Running: return "Running";
        case FeatureStatus::GeneratingTemplates: return "Generating Templates";
        case FeatureStatus::UploadingDashboards: return "Uploading Dashboards";
        case FeatureStatus::UploadingLayers: return "Uploading Layers";
        case FeatureStatus::UploadingFunctions: return "Uploading Functions";
        case FeatureStatus::DeployingResources: return "Deploying Resources";
        case FeatureStatus::DeletingResources: return "Deleting Resources";
        default: return "Unknown";
        }
    }

    inline FeatureStatus GetFeatureStatusFromString(const std::string& status)
    {
        if (status == "Deployed") return FeatureStatus::Deployed;
        if (status == "Undeployed") return FeatureStatus::Undeployed;
        if (status == "Error") return FeatureStatus::Error;
        if (status == "Rollback Complete") return FeatureStatus::RollbackComplete;
        if (status == "Running") return FeatureStatus::Running;
        if (status == "Generating Templates") return FeatureStatus::GeneratingTemplates;
        if (status == "Uploading Dashboards") return FeatureStatus::UploadingDashboards;
        if (status == "Uploading Layers") return FeatureStatus::UploadingLayers;
        if (status == "Uploading Functions") return FeatureStatus::UploadingFunctions;
        if (status == "Deploying Resources") return FeatureStatus::DeployingResources;
        if (status == "Deleting Resources") return FeatureStatus::DeletingResources;
        return FeatureStatus::Unknown;
    }

    inline FeatureStatus GetFeatureStatusFromCloudFormationStackStatus(const std::string& status) {

        if (status == "ROLLBACK_COMPLETE" ||
            status == "UPDATE_ROLLBACK_COMPLETE" ||
            status == "IMPORT_ROLLBACK_COMPLETE")
        {
            return FeatureStatus::RollbackComplete;
        }

        if (status == "DELETE_COMPLETE" ||
            status == "UNDEPLOYED" ||
            status == "")
        {
            return FeatureStatus::Undeployed;
        }

        if (status.find("COMPLETE") != std::string::npos)
        {
            return FeatureStatus::Deployed;
        }

        if (status.find("IN_PROGRESS") != std::string::npos)
        {
            return FeatureStatus::Running;
        }

        if (status.find("FAILED") != std::string::npos)
        {
            return FeatureStatus::Error;
        }

        // All the enum values (https://sdk.amazonaws.com/cpp/api/LATEST/_stack_status_8h_source.html) should be covered except NOT_SET
        return FeatureStatus::Undeployed;
    }

    inline FeatureStatusSummary GetSummaryFromFeatureStatus(FeatureStatus status)
    {
        switch (status)
        {
        case FeatureStatus::Deployed: return FeatureStatusSummary::Deployed;
        case FeatureStatus::Undeployed: return FeatureStatusSummary::Undeployed;
        case FeatureStatus::RollbackComplete: // fall through
        case FeatureStatus::Error: return FeatureStatusSummary::Error;
        case FeatureStatus::Unknown: return FeatureStatusSummary::Unknown;
        default: return FeatureStatusSummary::Running;
        }
    }

    inline std::string GetFeatureTypeString(FeatureType feature)
    {
        switch (feature)
        {
        case FeatureType::Main: return "main";
        case FeatureType::Identity: return "identity";
        case FeatureType::Authentication: return "authentication";
        case FeatureType::Achievements: return "achievements";
        case FeatureType::GameStateCloudSaving: return "gamesaving";
        case FeatureType::UserGameplayData: return "usergamedata";
        default: return "main";
        }
    }

    inline FeatureType GetFeatureTypeFromString(const std::string& feature)
    {
        if (feature == "identity") return FeatureType::Identity;
        if (feature == "authentication") return FeatureType::Authentication;
        if (feature == "achievements") return FeatureType::Achievements;
        if (feature == "gamesaving") return FeatureType::GameStateCloudSaving;
        if (feature == "usergamedata") return FeatureType::UserGameplayData;

        return FeatureType::Main;
    }
}
