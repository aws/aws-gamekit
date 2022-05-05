// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/core/exports.h>

#include <aws/gamekit/core/awsclients/api_initializer.h>
#include <aws/gamekit/core/feature_resources.h>
#include <aws/gamekit/core/gamekit_account.h>
#include <aws/gamekit/core/gamekit_settings.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/awsclients/default_clients.h>
#include <aws/gamekit/core/model/account_credentials.h>
#include <aws/gamekit/core/model/account_info.h>
#include <aws/gamekit/core/utils/sts_utils.h>

#include <fstream>
#include <sstream>

using namespace GameKit::Logger;

#pragma region AWS SDK API Initialization / Shutdown
unsigned int GameKitInitializeAwsSdk(FuncLogCallback logCb)
{
    GameKit::AwsApiInitializer::Initialize(logCb);
    return GameKit::GAMEKIT_SUCCESS;
}

unsigned int GameKitShutdownAwsSdk(FuncLogCallback logCb)
{
    GameKit::AwsApiInitializer::Shutdown(logCb, nullptr, true);
    return GameKit::GAMEKIT_SUCCESS;
}
#pragma endregion

#pragma region GameKitAccount Methods
unsigned int GameKitGetAwsAccountId(DISPATCH_RECEIVER_HANDLE caller, CharPtrCallback resultCallback, const char* accessKey, const char* secretKey, FuncLogCallback logCb)
{
    if (accessKey == nullptr || strlen(accessKey) == 0 || secretKey == nullptr || strlen(secretKey) == 0)
    {
        Logging::Log(logCb, Level::Error, "Invalid access key and/or secret.");

        return GameKit::GAMEKIT_ERROR_GENERAL;
    }

    GameKit::Utils::STSUtils stsUtils(accessKey, secretKey, logCb);
    const std::string accountId = stsUtils.GetAwsAccountId();

    if (!accountId.empty() && resultCallback != nullptr)
    {
        resultCallback(caller, accountId.c_str());

        return GameKit::GAMEKIT_SUCCESS;
    }

    return GameKit::GAMEKIT_ERROR_GENERAL;
}

// Deprecated (Use GameKitAccountInstanceCreateWithRootPaths)
GAMEKIT_ACCOUNT_INSTANCE_HANDLE GameKitAccountInstanceCreate(const GameKit::AccountInfo accountInfo, const GameKit::AccountCredentials credentials, FuncLogCallback logCb)
{
    GameKit::GameKitAccount* gamekitAccount = new GameKit::GameKitAccount(accountInfo, credentials, logCb);
    gamekitAccount->InitializeDefaultAwsClients();
    return gamekitAccount;
}

GAMEKIT_ACCOUNT_INSTANCE_HANDLE GameKitAccountInstanceCreateWithRootPaths(const GameKit::AccountInfo accountInfo, const GameKit::AccountCredentials credentials, const char* rootPath, const char* pluginRootPath, FuncLogCallback logCb)
{
    GameKit::GameKitAccount* gamekitAccount = new GameKit::GameKitAccount(accountInfo, credentials, logCb);
    gamekitAccount->SetGameKitRoot(rootPath);
    gamekitAccount->SetPluginRoot(pluginRootPath);
    gamekitAccount->InitializeDefaultAwsClients();
    return gamekitAccount;
}

void GameKitAccountInstanceRelease(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    delete((GameKit::GameKitAccount*)accountInstance);
}

unsigned int GameKitAccountInstanceBootstrap(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    return ((GameKit::GameKitAccount*)accountInstance)->Bootstrap();
}

const char* GameKitAccountGetRootPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    return ((GameKit::GameKitAccount*)accountInstance)->GetGameKitRoot().c_str();
}

const char* GameKitAccountGetPluginRootPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    return ((GameKit::GameKitAccount*)accountInstance)->GetPluginRoot().c_str();
}

const char* GameKitAccountGetBaseCloudFormationPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    return ((GameKit::GameKitAccount*)accountInstance)->GetBaseCloudFormationPath().c_str();
}

const char* GameKitAccountGetBaseFunctionsPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    return ((GameKit::GameKitAccount*)accountInstance)->GetBaseFunctionsPath().c_str();
}

const char* GameKitAccountGetInstanceCloudFormationPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    return ((GameKit::GameKitAccount*)accountInstance)->GetInstanceCloudFormationPath().c_str();
}

const char* GameKitAccountGetInstanceFunctionsPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    return ((GameKit::GameKitAccount*)accountInstance)->GetInstanceFunctionsPath().c_str();
}

void GameKitAccountSetRootPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance, const char* rootPath)
{
    ((GameKit::GameKitAccount*)accountInstance)->SetGameKitRoot(rootPath);
}

void GameKitAccountSetPluginRootPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance, const char* pluginRootPath)
{
    ((GameKit::GameKitAccount*)accountInstance)->SetPluginRoot(pluginRootPath);
}

bool GameKitAccountHasValidCredentials(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    return ((GameKit::GameKitAccount*)accountInstance)->HasValidCredentials();
}

unsigned int GameKitAccountCheckSecretExists(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance, const char* secretName)
{
    return ((GameKit::GameKitAccount*)accountInstance)->CheckSecretExists(secretName);
}

unsigned int GameKitAccountSaveSecret(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance, const char* secretName, const char* secretValue)
{
    return ((GameKit::GameKitAccount*)accountInstance)->SaveSecret(secretName, secretValue);
}

unsigned int GameKitAccountDeleteSecret(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance, const char* secretName)
{
    return ((GameKit::GameKitAccount*)accountInstance)->DeleteSecret(secretName);
}

unsigned int GameKitAccountSaveFeatureInstanceTemplates(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    return ((GameKit::GameKitAccount*)accountInstance)->SaveFeatureInstanceTemplates();
}

unsigned int GameKitAccountUploadAllDashboards(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    return ((GameKit::GameKitAccount*)accountInstance)->UploadDashboards();
}

GAMEKIT_API unsigned int GameKitAccountUploadLayers(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    return ((GameKit::GameKitAccount*)accountInstance)->UploadLayers();
}

unsigned int GameKitAccountUploadFunctions(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    return ((GameKit::GameKitAccount*)accountInstance)->UploadFunctions();
}

unsigned int GameKitAccountCreateOrUpdateMainStack(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    return ((GameKit::GameKitAccount*)accountInstance)->CreateOrUpdateMainStack();
}

unsigned int GameKitAccountCreateOrUpdateStacks(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    return ((GameKit::GameKitAccount*)accountInstance)->CreateOrUpdateStacks();
}

unsigned int GameKitAccountDeployApiGatewayStage(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance)
{
    return ((GameKit::GameKitAccount*)accountInstance)->DeployApiGatewayStage();
}

// Deprecated (Use GameKitResourcesInstanceCreateWithRootPaths)
GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE GameKitResourcesInstanceCreate(const GameKit::AccountInfo accountInfo, const GameKit::AccountCredentials credentials, GameKit::FeatureType featureType, FuncLogCallback logCb)
{
	GameKit::GameKitFeatureResources* gamekitFeature = new GameKit::GameKitFeatureResources(accountInfo, credentials, featureType, logCb);
	return gamekitFeature;
}

GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE GameKitResourcesInstanceCreateWithRootPaths(const GameKit::AccountInfo accountInfo, const GameKit::AccountCredentials credentials, GameKit::FeatureType featureType, const char* rootPath, const char* pluginRootPath, FuncLogCallback logCb)
{
    GameKit::GameKitFeatureResources* gamekitFeature = new GameKit::GameKitFeatureResources(accountInfo, credentials, featureType, logCb);
    gamekitFeature->SetGameKitRoot(rootPath);
    gamekitFeature->SetPluginRoot(pluginRootPath);
    return gamekitFeature;
}
#pragma endregion

#pragma region GameKitResources Methods
void GameKitResourcesInstanceRelease(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    delete((GameKit::GameKitFeatureResources*)resourceInstance);
}

const char* GameKitResourcesGetRootPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->GetGameKitRoot().c_str();
}

const char* GameKitResourcesGetPluginRootPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->GetPluginRoot().c_str();
}

const char* GameKitResourcesGetBaseCloudFormationPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->GetBaseCloudFormationPath().c_str();

}
const char* GameKitResourcesGetBaseFunctionsPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->GetBaseFunctionsPath().c_str();
}

const char* GameKitResourcesGetInstanceCloudFormationPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->GetInstanceCloudFormationPath().c_str();
}

const char* GameKitResourcesGetInstanceFunctionsPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->GetInstanceFunctionsPath().c_str();
}

void GameKitResourcesSetRootPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance, const char* rootPath)
{
    ((GameKit::GameKitFeatureResources*)resourceInstance)->SetGameKitRoot(rootPath);
}

void GameKitResourcesSetPluginRootPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance, const char* pluginRootPath)
{
    ((GameKit::GameKitFeatureResources*)resourceInstance)->SetPluginRoot(pluginRootPath);
}

unsigned int GameKitResourcesCreateEmptyConfigFile(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->WriteEmptyClientConfiguration();
}

unsigned int GameKitResourcesInstanceCreateOrUpdateStack(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->CreateOrUpdateFeatureStack();
}

unsigned int GameKitResourcesInstanceDeleteStack(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->DeleteFeatureStack();
}

unsigned int GameKitResourcesGetCurrentStackStatus(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance, DISPATCH_RECEIVER_HANDLE receiver, CharPtrCallback resultsCb)
{
    std::string currStatus = ((GameKit::GameKitFeatureResources*)resourceInstance)->GetCurrentStackStatus();
    resultsCb(receiver, currStatus.c_str());

    if (currStatus.compare(GameKit::ERR_STACK_CURRENT_STATUS_UNDEPLOYED) == 0)
    {
        return GameKit::GAMEKIT_ERROR_CLOUDFORMATION_NO_CURRENT_STACK_STATUS;
    }

    return GameKit::GAMEKIT_SUCCESS;
}

bool GameKitResourcesIsCloudFormationInstanceTemplatePresent(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->IsCloudFormationInstanceTemplatePresent();
}

unsigned int GameKitResourcesSaveDeployedCloudFormationTemplate(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->SaveDeployedCloudFormationTemplate();
}

unsigned int GameKitResourcesSaveDeployedCloudformationParameters(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance, DeployedParametersCallback parametersCb)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->GetDeployedCloudFormationParameters(parametersCb);
}

unsigned int GameKitResourcesSaveCloudFormationInstance(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->SaveCloudFormationInstance();
}

GAMEKIT_API unsigned int GameKitResourcesUpdateCloudFormationParameters(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->UpdateCloudFormationParameters();
}

GAMEKIT_API unsigned int GameKitResourcesSaveLayerInstances(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->SaveLayerInstances();
}

unsigned int GameKitResourcesSaveFunctionInstances(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->SaveFunctionInstances();
}

GAMEKIT_API unsigned int GameKitResourcesUploadFeatureLayers(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    GameKit::GameKitFeatureResources* resources = ((GameKit::GameKitFeatureResources*)resourceInstance);

    // create/set replacement id
    unsigned int result = resources->CreateAndSetLayersReplacementId();
    if (result != GameKit::GAMEKIT_SUCCESS)
    {
        return result;
    }

    // compress feature functions
    result = resources->CompressFeatureLayers();
    if (result != GameKit::GAMEKIT_SUCCESS)
    {
        return result;
    }

    // upload feature functions
    result = resources->UploadFeatureLayers();
    if (result != GameKit::GAMEKIT_SUCCESS)
    {
        return result;
    }

    // cleanup
    resources->CleanupTempFiles();

    return result;
}

unsigned int GameKitResourcesUploadFeatureFunctions(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance)
{
    GameKit::GameKitFeatureResources* resources = ((GameKit::GameKitFeatureResources*)resourceInstance);

    // create/set replacement id
    unsigned int result = resources->CreateAndSetFunctionsReplacementId();
    if (result != GameKit::GAMEKIT_SUCCESS)
    {
        return result;
    }

    // compress feature functions
    result = resources->CompressFeatureFunctions();
    if (result != GameKit::GAMEKIT_SUCCESS)
    {
        return result;
    }

    // upload feature functions
    result = resources->UploadFeatureFunctions();
    if (result != GameKit::GAMEKIT_SUCCESS)
    {
        return result;
    }

    // cleanup
    resources->CleanupTempFiles();

    return result;
}

unsigned int GameKitResourcesDescribeStackResources(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance, FuncResourceInfoCallback resourceInfoCb)
{
    return ((GameKit::GameKitFeatureResources*)resourceInstance)->DescribeStackResources(resourceInfoCb);
}
#pragma endregion

#pragma region GameKitSettings Methods
GAMEKIT_SETTINGS_INSTANCE_HANDLE GameKitSettingsInstanceCreate(const char* rootPath, const char* pluginVersion, const char* shortGameName, const char* currentEnvironment, FuncLogCallback logCb)
{
    GameKit::GameKitSettings* gamekitSettings = new GameKit::GameKitSettings(rootPath, pluginVersion, shortGameName, currentEnvironment, logCb);
    return gamekitSettings;
}

void GameKitSettingsInstanceRelease(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance)
{
    delete((GameKit::GameKitSettings*)settingsInstance);
}

void GameKitSettingsSetGameName(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* gameName)
{
    ((GameKit::GameKitSettings*)settingsInstance)->SetGameName(gameName);
}

void GameKitSettingsSetLastUsedRegion(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* region)
{
    ((GameKit::GameKitSettings*)settingsInstance)->SetLastUsedRegion(region);
}

void GameKitSettingsSetLastUsedEnvironment(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* envCode)
{
    ((GameKit::GameKitSettings*)settingsInstance)->SetLastUsedEnvironment(envCode);
}

void GameKitSettingsAddCustomEnvironment(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* envCode, const char* envDescription)
{
    ((GameKit::GameKitSettings*)settingsInstance)->AddCustomEnvironment(envCode, envDescription);
}

void GameKitSettingsDeleteCustomEnvironment(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* envCode)
{
    ((GameKit::GameKitSettings*)settingsInstance)->DeleteCustomEnvironment(envCode);
}

void GameKitSettingsActivateFeature(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, GameKit::FeatureType featureType)
{
    ((GameKit::GameKitSettings*)settingsInstance)->ActivateFeature(featureType);
}

void GameKitSettingsDeactivateFeature(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, GameKit::FeatureType featureType)
{
    ((GameKit::GameKitSettings*)settingsInstance)->DeactivateFeature(featureType);
}

void GameKitSettingsSetFeatureVariables(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, GameKit::FeatureType featureType, const char* const* varKeys, const char* const* varValues, size_t numKeys)
{
    std::map<std::string, std::string> vars;
    for (size_t i = 0; i < numKeys; i++)
    {
        vars.insert({ varKeys[i], varValues[i] });
    }

    ((GameKit::GameKitSettings*)settingsInstance)->SetFeatureVariables(featureType, vars);
}

void GameKitSettingsDeleteFeatureVariable(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, GameKit::FeatureType featureType, const char* varName)
{
    ((GameKit::GameKitSettings*)settingsInstance)->DeleteFeatureVariable(featureType, varName);
}

unsigned int GameKitSettingsSave(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance)
{
    return ((GameKit::GameKitSettings*)settingsInstance)->SaveSettings();
}

void GameKitSettingsGetGameName(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, CharPtrCallback resultsCb)
{
    const std::string gameInfo = ((GameKit::GameKitSettings*)settingsInstance)->GetGameName();
    resultsCb(receiver, gameInfo.c_str());
}

void GameKitSettingsGetLastUsedRegion(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, CharPtrCallback resultsCb)
{
    const std::string region = ((GameKit::GameKitSettings*)settingsInstance)->GetLastUsedRegion();
    resultsCb(receiver, region.c_str());
}

void GameKitSettingsGetLastUsedEnvironment(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, CharPtrCallback resultsCb)
{
    const std::string envCode = ((GameKit::GameKitSettings*)settingsInstance)->GetLastUsedEnvironment();
    resultsCb(receiver, envCode.c_str());
}

void GameKitSettingsGetCustomEnvironments(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, KeyValueCharPtrCallbackDispatcher resultsCb)
{
    std::map<std::string, std::string> customEnvs = ((GameKit::GameKitSettings*)settingsInstance)->GetCustomEnvironments();
    for (auto const& entry : customEnvs)
    {
        resultsCb(receiver, entry.first.c_str(), entry.second.c_str());
    }
}

void GameKitSettingsGetCustomEnvironmentDescription(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, const char* envCode, CharPtrCallback resultsCb)
{
    const std::string envDesc = ((GameKit::GameKitSettings*)settingsInstance)->GetCustomEnvironmentDescription(envCode);
    resultsCb(receiver, envDesc.c_str());
}

bool GameKitSettingsIsFeatureActive(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, GameKit::FeatureType featureType)
{
    return ((GameKit::GameKitSettings*)settingsInstance)->IsFeatureActive(featureType);
}

void GameKitSettingsGetFeatureVariables(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, GameKit::FeatureType featureType, KeyValueCharPtrCallbackDispatcher resultsCb)
{
    std::map<std::string, std::string> featureVars = ((GameKit::GameKitSettings*)settingsInstance)->GetFeatureVariables(featureType);
    for (auto const& entry : featureVars)
    {
        resultsCb(receiver, entry.first.c_str(), entry.second.c_str());
    }
}

void GameKitSettingsGetFeatureVariable(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, GameKit::FeatureType featureType, const char* varName, CharPtrCallback resultsCb)
{
    const std::string varValue = ((GameKit::GameKitSettings*)settingsInstance)->GetFeatureVariable(featureType, varName);
    resultsCb(receiver, varValue.c_str());
}

void GameKitSettingsGetSettingsFilePath(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, CharPtrCallback resultsCb)
{
    const std::string filePath = ((GameKit::GameKitSettings*)settingsInstance)->GetSettingsFilePath();
    resultsCb(receiver, filePath.c_str());
}

void GameKitSettingsReload(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance)
{
    ((GameKit::GameKitSettings*)settingsInstance)->Reload();
}

unsigned int GameKitSaveAwsCredentials(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* profileName, const char* accessKey, const char* secretKey, FuncLogCallback logCb)
{
    return ((GameKit::GameKitSettings*)settingsInstance)->SaveAwsCredentials(profileName, accessKey, secretKey, logCb);
}

unsigned int GameKitSetAwsAccessKey(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* profileName, const char* newAccessKey, FuncLogCallback logCb)
{
    return ((GameKit::GameKitSettings*)settingsInstance)->SetAwsAccessKey(profileName, newAccessKey, logCb);
}

unsigned int GameKitSetAwsSecretKey(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* profileName, const char* newSecretKey, FuncLogCallback logCb)
{
    return ((GameKit::GameKitSettings*)settingsInstance)->SetAwsSecretKey(profileName, newSecretKey, logCb);
}

unsigned int GameKitGetAwsProfile(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* profileName, DISPATCH_RECEIVER_HANDLE receiver, FuncAwsProfileResponseCallback responseCallback, FuncLogCallback logCb)
{
    return ((GameKit::GameKitSettings*)settingsInstance)->GetAwsProfile(profileName, receiver, responseCallback, logCb);
}
#pragma endregion
