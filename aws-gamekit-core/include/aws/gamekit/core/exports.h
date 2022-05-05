// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

/** @file
 * @brief The C interface for the Core library.
 */

#pragma once

// Standard Library
#include <exception>
#include <string>

// GameKit
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/feature_resources_callback.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/model/account_credentials.h>
#include <aws/gamekit/core/model/account_info.h>

namespace GameKit { namespace Utils { class STSUtils; } }

/**
 * @brief  GameKitAccount instance handle created by calling #GameKitAccountInstanceCreate()
*/
typedef void* GAMEKIT_ACCOUNT_INSTANCE_HANDLE;

/**
 * @brief GameKitFeatureResources instance handle created by calling #GameKitResourcesInstanceCreate()
*/
typedef void* GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE;

/**
 * @brief GameKitSettings instance handle created by calling #GameKitSettingsInstanceCreate()
*/
typedef void* GAMEKIT_SETTINGS_INSTANCE_HANDLE;

/**
 * @brief A pointer to an instance of a class that can receive a callback.
 *
 * @details The callback method signature is specified by each API which uses a DISPATCH_RECEIVER_HANDLE.
 *
 * For example: GameKitSettingsGetGameName() uses a callback signature of CharPtrCallback,
 * whereas GameKitSettingsGetCustomEnvironments() uses a callback signature of KeyValueCharPtrCallbackDispatcher.
 */
typedef void* DISPATCH_RECEIVER_HANDLE;


extern "C"
{
    /**
     * @brief A static dispatcher function pointer that receives a character array.
     *
     * @param dispatchReceiver A pointer to an instance of a class where the results will be dispatched to.
     * This instance must have a method signature of void ReceiveResult(const char* charPtr);
     * @param charPtr The character array pointer that the callback function receives.
    */
    typedef void(*CharPtrCallback)(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* charPtr);

    /**
     * @brief A static dispatcher function pointer that receives key/value pairs, one invocation at a time.
     *
     * @param dispatchReceiver A pointer to an instance of a class where the results will be dispatched to.
     * This instance must have have a method signature of void ReceiveResult(const char* charKey, const char* charValue);
     * @param charKey The key that the callback function receives.
     * @param charValue The value that the callback function receives.
    */
    typedef void(*KeyValueCharPtrCallbackDispatcher)(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* charKey, const char* charValue);

    /**
     * @breif A static dispatcher function pointer that receives a single key/value pairs.
     *
     * @param dispatchReceiver A pointer to an instance of a class where the results will be dispatched to.
     * This instance must have have a method signature of void ReceiveResult(const char* responseAwsAccessKey, const char* responseAwsSecret)
     * @param responseAwsAccessKey The value of the Aws access key that the callback function receives.
     * @param responseAwsSecret The value of the Aws secret key that the callback function receives.
     */
    typedef void(*FuncAwsProfileResponseCallback)(DISPATCH_RECEIVER_HANDLE dispatchReceiver, const char* responseAwsAccessKey, const char* responseAwsSecret);
}

extern "C"
{
#pragma region AWS SDK API Initialization / Shutdown
    /**
     * @brief Explicitly initialize the AWS SDK
     */
    GAMEKIT_API unsigned int GameKitInitializeAwsSdk(FuncLogCallback logCb);

    /**
     * @brief Explicitly shuts down the AWS SDK. Note: this will force shutdown the SDK even if there are active SDK clients using it.
     This is meant to be called only when quiting the main game instance.
     */
    GAMEKIT_API unsigned int GameKitShutdownAwsSdk(FuncLogCallback logCb);
#pragma endregion

#pragma region GameKitAccount
    // -------- Static functions, these don't require a GameKitAccount instance handle
    /**
     * @brief Get the AWS Account ID which corresponds to the provided Access Key and Secret Key.
     *
     * @details For more information about AWS access keys and secret keys, see: https://docs.aws.amazon.com/general/latest/gr/aws-sec-cred-types.html#access-keys-and-secret-access-keys
     *
     * @param dispatchReceiver Pointer to the caller object (object that will handle the callback function).
     * @param resultCb Pointer to the callback function to invoke on completion.
     * @param accessKey The AWS Access Key.
     * @param secretKey The AWS Secret Key.
     * @param logCb Callback function for logging information and errors.
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitGetAwsAccountId(DISPATCH_RECEIVER_HANDLE dispatchReceiver, CharPtrCallback resultCb, const char* accessKey, const char* secretKey, FuncLogCallback logCb);

    // -------- Instance functions, these require a GameKitAccount instance handle
    /**
     * @brief Create a GameKitAccount instance, which can be used to access the GameKitAccount API.
     *
     * @details Make sure to call GameKitAccountInstanceRelease() to destroy the returned object when finished with it.
     *
     * @param accountInfo Struct holding account id, game name, and deployment environment.
     * @param credentials Struct holding account id, region, access key, and secret key.
     * @param logCb Callback function for logging information and errors.
     * @return Pointer to the new GameKitAccount instance.
     */
     // Deprecated
    GAMEKIT_API GAMEKIT_ACCOUNT_INSTANCE_HANDLE GameKitAccountInstanceCreate(const GameKit::AccountInfo accountInfo, const GameKit::AccountCredentials credentials, FuncLogCallback logCb);

    /**
     * @brief Create a GameKitAccount instance, which can be used to access the GameKitAccount API. Also sets the plugin and game root paths.
     *
     * @details Make sure to call GameKitAccountInstanceRelease() to destroy the returned object when finished with it.
     *
     * @param accountInfo Struct holding account id, game name, and deployment environment.
     * @param credentials Struct holding account id, region, access key, and secret key.
     * @param rootPath New path for GAMEKIT_ROOT
     * @param pluginRootPath New path for the plugin root directory.
     * @param logCb Callback function for logging information and errors.
     * @return Pointer to the new GameKitAccount instance.
     */
    GAMEKIT_API GAMEKIT_ACCOUNT_INSTANCE_HANDLE GameKitAccountInstanceCreateWithRootPaths(const GameKit::AccountInfo accountInfo, const GameKit::AccountCredentials credentials, const char* rootPath, const char* pluginRootPath, FuncLogCallback logCb);

    /**
     * @brief Destroy the provided GameKitAccount instance.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     */
    GAMEKIT_API void GameKitAccountInstanceRelease(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);

    /**
     * @brief Get the GAMEKIT_ROOT path where the "instance" templates and settings are going to be stored.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @return GAMEKIT_ROOT path, or an empty string if GameKitAccountSetRootPath() hasn't been called yet.
     */
    GAMEKIT_API const char* GameKitAccountGetRootPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);

    /**
     * @brief Set the GAMEKIT_ROOT path where the "instance" templates and settings are going to be stored.
     *
     * @details This value can be fetched by GameKitAccountGetRootPath() and defaults to an empty string until this method is called.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @param rootPath New path for GAMEKIT_ROOT.
     */
    GAMEKIT_API void GameKitAccountSetRootPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance, const char* rootPath);

    /**
     * @brief Get the root directory of the GAMEKIT plugin's installation.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @return Plugin root path, or an empty string if GameKitAccountSetPluginRootPath() hasn't been called yet.
     */
    GAMEKIT_API const char* GameKitAccountGetPluginRootPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);

    /**
     * @brief Set the root directory where the GAMEKIT plugin was installed.
     *
     * @details This value can be fetched by GameKitAccountGetPluginRootPath() and defaults to an empty string until this method is called.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @param pluginRootPath New path for the plugin root directory.
     */
    GAMEKIT_API void GameKitAccountSetPluginRootPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance, const char* pluginRootPath);

    /**
     * @brief Get the path where the "base" CloudFormation templates are stored.
     *
     * @details This path is set by calling GameKitAccountSetPluginRootPath(), and is equal to GameKitAccountGetPluginRootPath()+ResourceDirectories::CLOUDFORMATION_DIRECTORY.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @return Path to the "base" CloudFormation templates, or an empty string if GameKitAccountSetPluginRootPath() hasn't been called yet.
     */
    GAMEKIT_API const char* GameKitAccountGetBaseCloudFormationPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);

    /**
     * @brief Get the path where the "base" Lambda functions are stored.
     *
     * @details This path is set by calling GameKitAccountSetPluginRootPath(), and is equal to GameKitAccountGetPluginRootPath()+ResourceDirectories::FUNCTIONS_DIRECTORY.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @return Path to the "base" Lambda functions, or an empty string if GameKitAccountSetPluginRootPath() hasn't been called yet.
     */
    GAMEKIT_API const char* GameKitAccountGetBaseFunctionsPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);

    /**
     * @brief Get the path where the "instance" CloudFormation templates are stored.
     *
     * @details This path is set by calling GameKitAccountSetRootPath(), and is equal to GameKitAccountGetRootPath()+<gameName>+ResourceDirectories::CLOUDFORMATION_DIRECTORY;.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @return Path to the "instance" CloudFormation templates, or an empty string if GameKitAccountSetRootPath() hasn't been called yet.
     */
    GAMEKIT_API const char* GameKitAccountGetInstanceCloudFormationPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);

    /**
     * @brief Get the path where the "instance" Lambda functions are stored.
     *
     * @details This path is set by calling GameKitAccountSetRootPath(), and is equal to GameKitAccountGetRootPath()+<gameName>+ResourceDirectories::FUNCTIONS_DIRECTORY.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @return Path to the "instance" Lambda functions, or an empty string if GameKitAccountSetPluginRootPath() hasn't been called yet.
     */
    GAMEKIT_API const char* GameKitAccountGetInstanceFunctionsPath(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);

    /**
     * @brief Return True if the provided GameKitAccount instance has valid AWS credentials (access key, secret key, and AWS region), return False otherwise.
     *
     * @details In this case, "valid" means the credentials are allowed to list S3 buckets (i.e. the IAM Role has "s3:ListAllMyBuckets" permission).
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @return True if the credentials are valid, false otherwise.
     */
    GAMEKIT_API bool GameKitAccountHasValidCredentials(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);

    /**
     * @brief Create a bootstrap bucket in the AWS account if it doesn't already exist.
     *
     * @details The bootstrap bucket must be created before deploying any stacks or Lambda functions.
     * There needs to be a unique bootstrap bucket for each combination of Environment, Region, Account ID, and GameName.
     *
     * @details The bucket name will be "do-not-delete-gamekit-<env>-<5_letter_aws_region_code>-<base36_account_id>-<gamename>"
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitAccountInstanceBootstrap(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);

    /**
     * @brief Create or update a secret in AWS SecretsManager (https://aws.amazon.com/secrets-manager/).
     *
     * @details The secret name will be "gamekit_<environment>_<gameName>_<secretName>", for example: "gamekit_dev_mygame_amazon_client_secret".
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @param secretName Name of the secret. Will be prefixed as described in the details.
     * @param secretValue Value of the secret.
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitAccountSaveSecret(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance, const char* secretName, const char* secretValue);

    /**
     * @brief Checks if a secret exists in AWS SecretsManager (https://aws.amazon.com/secrets-manager/).
     *
     * @details The secret name will be "gamekit_<environment>_<gameName>_<secretName>", for example: "gamekit_dev_mygame_amazon_client_secret".
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @param secretName Name of the secret. Will be prefixed as described in the details.
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitAccountCheckSecretExists(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance, const char* secretName);

    /**
     * @brief Delete a secret in AWS SecretsManager (https://aws.amazon.com/secrets-manager/).
     *
     * @details The secret name will be "gamekit_<environment>_<gameName>_<secretName>", for example: "gamekit_dev_mygame_amazon_client_secret".
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @param secretName Name of the secret. Will be prefixed as described in the details.
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful or non-existent, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitAccountDeleteSecret(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance, const char* secretName);

    /**
     * @brief Create or overwrite the instance template files for every feature.
     *
     *
     * @details Creates the following instance files for each feature: CloudFormation template, CloudFormation parameters, and Lambda functions.
     * Instance files are written to the "GAMEKIT_ROOT" path, and are created as copies of the "Base" path files with the placeholder variables filled in.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitAccountSaveFeatureInstanceTemplates(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);

    /**
     * @brief Upload the Dashboard configuration file for every feature to the bootstrap bucket.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitAccountUploadAllDashboards(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);

    /**
     * @brief Upload the Lambda instance layers for every feature to the bootstrap bucket.
     *
     * @details GameKitAccountSaveFeatureInstanceTemplates() should be called first to create the Lambda instance templates.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitAccountUploadLayers(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);

    /**
     * @brief Upload the Lambda instance functions for every feature to the bootstrap bucket.
     *
     * @details GameKitAccountSaveFeatureInstanceTemplates() should be called first to create the Lambda instance templates.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitAccountUploadFunctions(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);

    /**
     * @brief Deploy the "main" CloudFormation stack to AWS.
     *
     * @details Should call GameKitAccountSaveFeatureInstanceTemplates() first to create the instance templates.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitAccountCreateOrUpdateMainStack(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);

    /**
     * @brief Deploy all CloudFormation stacks to AWS (i.e. the "main" stack and all feature stacks).
     *
     * @details Should call GameKitAccountSaveFeatureInstanceTemplates() first to create the instance templates.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitAccountCreateOrUpdateStacks(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);

    /**
     * @brief Deploy the API Gateway stage of the "main" CloudFormation stack.
     *
     * @param accountInstance Pointer to a GameKitAccount instance created with GameKitAccountInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitAccountDeployApiGatewayStage(GAMEKIT_ACCOUNT_INSTANCE_HANDLE accountInstance);
#pragma endregion

#pragma region GameKitFeatureResources
    /**
     * @brief Create a GameKitFeatureResources instance, which can be used to access the GameKitFeatureResources API.
     *
     * @details Make sure to call GameKitResourcesInstanceRelease() to destroy the returned object when finished with it.
     *
     * @param accountInfo Struct holding account id, game name, and deployment environment.
     * @param credentials Struct holding account id, region, access key, and secret key.
     * @param featureType The GAMEKIT feature to work with.
     * @param logCb Callback function for logging information and errors.
     * @return Pointer to the new GameKitFeatureResources instance.
     */
    // Deprecated
    GAMEKIT_API GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE GameKitResourcesInstanceCreate(const GameKit::AccountInfo accountInfo, const GameKit::AccountCredentials credentials, GameKit::FeatureType featureType, FuncLogCallback logCb);

    /**
     * @brief Create a GameKitFeatureResources instance, which can be used to access the GameKitFeatureResources API. Also sets the root and pluginRoot paths.
     *
     * @details Make sure to call GameKitResourcesInstanceRelease() to destroy the returned object when finished with it.
     *
     * @param accountInfo Struct holding account id, game name, and deployment environment.
     * @param credentials Struct holding account id, region, access key, and secret key.
     * @param featureType The GAMEKIT feature to work with.
     * @param rootPath New path for GAMEKIT_ROOT.
     * @param pluginRootPath New path for the plugin root directory.
     * @param logCb Callback function for logging information and errors.
     * @return Pointer to the new GameKitFeatureResources instance.
     */
    GAMEKIT_API GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE GameKitResourcesInstanceCreateWithRootPaths(const GameKit::AccountInfo accountInfo, const GameKit::AccountCredentials credentials, GameKit::FeatureType featureType, const char* rootPath, const char* pluginRootPath, FuncLogCallback logCb);

    /**
     * @brief Destroy the provided GameKitFeatureResources instance.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     */
    GAMEKIT_API void GameKitResourcesInstanceRelease(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Get the GAMEKIT_ROOT path where the "instance" templates and settings are going to be stored.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return GAMEKIT_ROOT path, or an empty string if GameKitResourcesSetRootPath() hasn't been called yet.
     */
    GAMEKIT_API const char* GameKitResourcesGetRootPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Set the GAMEKIT_ROOT path where the "instance" templates and settings are going to be stored.
     *
     * @details This value can be fetched by GameKitResourcesGetRootPath() and defaults to an empty string until this method is called.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @param rootPath New path for GAMEKIT_ROOT.
     */
    GAMEKIT_API void GameKitResourcesSetRootPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance, const char* rootPath);

    /**
     * @brief Get the root directory of the GAMEKIT plugin's installation.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return Plugin root path, or an empty string if GameKitResourcesSetPluginRootPath() hasn't been called yet.
     */
    GAMEKIT_API const char* GameKitResourcesGetPluginRootPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Set the root directory where the GAMEKIT plugin was installed.
     *
     * @details This value can be fetched by GameKitResourcesGetPluginRootPath() and defaults to an empty string until this method is called.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @param pluginRootPath New path for the plugin root directory.
     */
    GAMEKIT_API void GameKitResourcesSetPluginRootPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance, const char* pluginRootPath);

    /**
     * @brief Get the path where this feature's "base" CloudFormation template is stored.
     *
     * @details This path is set by calling GameKitResourcesSetPluginRootPath(), and is equal to GameKitResourcesGetPluginRootPath()+ResourceDirectories::CLOUDFORMATION_DIRECTORY+<feature_name>.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return Path to this feature's "base" CloudFormation template, or an empty string if GameKitResourcesSetPluginRootPath() hasn't been called yet.
     */
    GAMEKIT_API const char* GameKitResourcesGetBaseCloudFormationPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Get the path where this feature's "base" Lambda functions are stored.
     *
     * @details This path is set by calling GameKitResourcesSetPluginRootPath(), and is equal to GameKitResourcesGetPluginRootPath()+ResourceDirectories::FUNCTIONS_DIRECTORY+<feature_name>.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return Path to this feature's "base" Lambda functions, or an empty string if GameKitResourcesGetPluginRootPath() hasn't been called yet.
     */
    GAMEKIT_API const char* GameKitResourcesGetBaseFunctionsPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Get the path where this feature's "instance" CloudFormation template is stored.
     *
     * @details This path is set by calling GameKitResourcesSetRootPath(), and is equal to GameKitResourcesGetRootPath()+<gameName>+ResourceDirectories::CLOUDFORMATION_DIRECTORY+<feature_name>;.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return Path to this feature's "instance" CloudFormation template, or an empty string if GameKitResourcesGetRootPath() hasn't been called yet.
     */
    GAMEKIT_API const char* GameKitResourcesGetInstanceCloudFormationPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Get the path where this feature's "instance" Lambda functions are stored.
     *
     * @details This path is set by calling GameKitResourcesSetRootPath(), and is equal to GameKitResourcesGetRootPath()+<gameName>+ResourceDirectories::FUNCTIONS_DIRECTORY+<feature_name>.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return Path to this feature's "instance" Lambda functions, or an empty string if GameKitResourcesGetRootPath() hasn't been called yet.
     */
    GAMEKIT_API const char* GameKitResourcesGetInstanceFunctionsPath(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Creates an empty client configuration file.
     *
     * @details Use to bootstrap a GameKit config file as soon as an environment has been selected.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitResourcesCreateEmptyConfigFile(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Deploy this feature's CloudFormation stack to AWS.
     *
     * @details Deploys the instance CloudFormation template. Creates a new stack if no stack exists, or updates an existing stack.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitResourcesInstanceCreateOrUpdateStack(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Delete this feature's CloudFormation stack from AWS.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitResourcesInstanceDeleteStack(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Get the status of this feature's deployed CloudFormation stack, such as "CREATE_COMPLETE", "UPDATE_IN_PROGRESS", or "UNDEPLOYED" if the stack is not deployed.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @param receiver A pointer to an instance of a class where the results will be dispatched to.
     * This instance must have a method signature of void ReceiveResult(const char* charPtr);
     * @param resultsCb A static dispatcher function pointer that receives a character array.
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitResourcesGetCurrentStackStatus(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance, DISPATCH_RECEIVER_HANDLE receiver, CharPtrCallback resultsCb);

    /**
     * @brief Checks if feature's CloudFormation template is present in the feature's instance directory.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return true if the feature's CloudFormation template is present in the feature's instance directory.
     */
    GAMEKIT_API bool GameKitResourcesIsCloudFormationInstanceTemplatePresent(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Retrieves the feature's deployed CloudFormation template and saves it to the feature's instance directory.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitResourcesSaveDeployedCloudFormationTemplate(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Retrieves the feature's deployed CloudFormation parameters and saves it to the feature's instance directory.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @param parametersCb A function pointer with a signature of void(const char*, const char*);
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitResourcesSaveDeployedCloudformationParameters(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance, DeployedParametersCallback parametersCb);

    /**
     * @brief Create or overwrite this feature's instance CloudFormation template file and CloudFormation parameters file.
     *
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitResourcesSaveCloudFormationInstance(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Update this feature's instance CloudFormation parameters file.
     *
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitResourcesUpdateCloudFormationParameters(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Create or overwrite this feature's instance Lambda layer files.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitResourcesSaveLayerInstances(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Create or overwrite this feature's instance Lambda function files.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitResourcesSaveFunctionInstances(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Upload this feature's instance Lambda layers to the S3 bootstrap bucket.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitResourcesUploadFeatureLayers(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Upload this feature's instance Lambda functions to the S3 bootstrap bucket.
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitResourcesUploadFeatureFunctions(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance);

    /**
     * @brief Log the current status of all the resources in this feature's CloudFormation stack.
     *
     * @details For example, a resource's status may look like: "CognitoLambdaRole, AWS::IAM::Role, CREATE_COMPLETE".
     *
     * @param resourceInstance Pointer to a GameKitFeatureResources instance created with GameKitResourcesInstanceCreate().
     * @param resourceInfoCb Callback function for logging the resources statuses.
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitResourcesDescribeStackResources(GAMEKIT_FEATURERESOURCES_INSTANCE_HANDLE resourceInstance, FuncResourceInfoCallback resourceInfoCb);
#pragma endregion

#pragma region GameKitSettings
    /**
     * @brief Create a GameKitSettings instance and load the settings from the GAMEKIT Settings YAML file.
     *
     * @details Make sure to call GameKitSettingsInstanceRelease() to destroy the returned object when finished with it.
     *
     * @param rootPath The GAMEKIT_ROOT path where the "instance" templates and settings are stored.
     * @param pluginVersion The GAMEKIT plugin version.
     * @param currentEnvironment The current active environment eg "dev", "qa", custom
     * @param shortGameName A shortened version of the game name.
     * @param logCb Callback function for logging information and errors.
     * @return Pointer to the new GameKitSettings instance.
     */
    GAMEKIT_API GAMEKIT_SETTINGS_INSTANCE_HANDLE GameKitSettingsInstanceCreate(const char* rootPath, const char* pluginVersion, const char* shortGameName, const char* currentEnvironment, FuncLogCallback logCb);

    /**
     * @brief Destroy the provided GameKitSettings instance.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     */
    GAMEKIT_API void GameKitSettingsInstanceRelease(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance);

    /**
     * @brief Set the game's name.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param gameName The new name.
     */
    GAMEKIT_API void GameKitSettingsSetGameName(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* gameName);

    /**
     * @brief Set the last used region.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param region The region.
     */
    GAMEKIT_API void GameKitSettingsSetLastUsedRegion(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* region);

    /**
     * @brief Set the last used environment.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param gameName The environment code.
     */
    GAMEKIT_API void GameKitSettingsSetLastUsedEnvironment(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* envCode);

    /**
     * @brief Add a custom deployment environment to the AWS Control Center menu.
     *
     * @details This custom environment will be available to select from the dropdown menu in the plugin's AWS Control Center,
     * alongside the default environments of "Development", "QA", "Staging", and "Production".
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param envCode Three letter code for the environment name. This code will be prefixed on all AWS resources that are
     * deployed to this environment. Ex: "gam" for "Gamma".
     * @param envDescription The environment name that will be displayed in the AWS Control Center. Ex: "Gamma".
     */
    GAMEKIT_API void GameKitSettingsAddCustomEnvironment(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* envCode, const char* envDescription);

    /**
     * @brief Remove a custom deployment environment from the AWS Control Center menu.
     *
     * @details Note: If you intend to delete the stacks deployed to this environment, you should delete them first
     * before deleting the custom environment. Otherwise you'll have to manually delete them from the AWS CloudFormation webpage.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param envCode Three letter code for the custom environment to delete. Ex: "gam" for "Gamma".
     */
    GAMEKIT_API void GameKitSettingsDeleteCustomEnvironment(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* envCode);

    /**
     * @brief Activate a GAMEKIT feature (ex: "Achievements").
     *
     * @details After activating, the feature will be available to configure and deploy.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param featureType The feature to activate.
     */
    GAMEKIT_API void GameKitSettingsActivateFeature(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, GameKit::FeatureType featureType);

    /**
     * @brief Deactivate a GAMEKIT feature (ex: "Achievements").
     *
     * @details After deactivating, the feature will not be able to be configured or deployed.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param featureType The feature to deactivate.
     */
    GAMEKIT_API void GameKitSettingsDeactivateFeature(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, GameKit::FeatureType featureType);

    /**
     * @brief Add key-value pairs to the feature's variables map, overwriting existing keys.
     *
     * @details The parameters "varKeys", "varValues", and "numKeys" represent a map<string, string>, where varKeys[N] maps to varValues[N], and numKeys is the total number of key-value pairs.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param featureType The feature to set the variables for.
     * @param varKeys The variable names.
     * @param varValues The variable values.
     * @param numKeys The number of key-value pairs. The length of varKeys, varValues, and numKeys should be equal.
     */
    GAMEKIT_API void GameKitSettingsSetFeatureVariables(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, GameKit::FeatureType featureType, const char* const* varKeys, const char* const* varValues, size_t numKeys);

    /**
     * @brief Delete a key-value pair from the feature's variables map.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param featureType The feature to delete the variable from.
     * @param varName The variable name to delete.
     */
    GAMEKIT_API void GameKitSettingsDeleteFeatureVariable(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, GameKit::FeatureType featureType, const char* varName);

    /**
     * @brief Write the GAMEKIT Settings YAML file to disk.
     *
     * @details Call this to persist any changes made through the "Set", "Add/Delete", "Activate/Deactivate" methods.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @return The result code of the operation. GAMEKIT_SUCCESS if successful, else a non-zero value in case of error. Consult errors.h file for details.
     */
    GAMEKIT_API unsigned int GameKitSettingsSave(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance);

    /**
     * @brief Get the game's full name, example: "My Full Game Name".
     *
     * @details The game name is returned through the callback and receiver.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param receiver A pointer to an instance of a class where the results will be dispatched to.
     * This instance must have a method signature of void ReceiveResult(const char* charPtr);
     * @param resultsCb A static dispatcher function pointer that receives a character array.
     */
    GAMEKIT_API void GameKitSettingsGetGameName(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, CharPtrCallback resultsCb);

    /**
     * @brief Get the developer's last submitted region, example: "us-west-2".
     *
     * @details The region is returned through the callback and receiver. If no last used region exists, defaults to us-east-1.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param receiver A pointer to an instance of a class where the results will be dispatched to.
     * This instance must have a method signature of void ReceiveResult(const char* charPtr);
     * @param resultsCb A static dispatcher function pointer that receives a character array.
     */
    GAMEKIT_API void GameKitSettingsGetLastUsedRegion(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, CharPtrCallback resultsCb);

    /**
   * @brief Get the developers last submitted environment code, example: "dev".
   *
   * @details The environment code is returned through the callback and receiver. If no last used environment exists, defaults to the dev environment.
   *
   * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
   * @param receiver A pointer to an instance of a class where the results will be dispatched to.
   * This instance must have a method signature of void ReceiveResult(const char* charPtr);
   * @param resultsCb A static dispatcher function pointer that receives a character array.
   */
    GAMEKIT_API void GameKitSettingsGetLastUsedEnvironment(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, CharPtrCallback resultsCb);


    /**
     * @brief Get all the custom environment key-value pairs (ex: "gam", "Gamma").
     *
     * @details The custom environments are returned through the callback and receiver.
     * The callback is invoked once for each custom environment.
     * The returned keys are 3-letter environment codes (ex: "gam"), and the values are corresponding environment descriptions (ex: "Gamma").
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param receiver A pointer to an instance of a class where the results will be dispatched to.
     * This instance must have have a method signature of void ReceiveResult(const char* charKey, const char* charValue);
     * @param resultsCb A static dispatcher function pointer that receives key/value pairs.
     */
    GAMEKIT_API void GameKitSettingsGetCustomEnvironments(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, KeyValueCharPtrCallbackDispatcher resultsCb);

    /**
     * @brief Get the custom environment description (ex: "Gamma") for the provided environment code (ex: "gam").
     *
     * @details The environment description is returned through the callback and receiver.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param receiver A pointer to an instance of a class where the results will be dispatched to.
     * This instance must have a method signature of void ReceiveResult(const char* charPtr);
     * @param envCode The 3-letter environment code to get the description for.
     * @param resultsCb A static dispatcher function pointer that receives a character array.
     */
    GAMEKIT_API void GameKitSettingsGetCustomEnvironmentDescription(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, const char* envCode, CharPtrCallback resultsCb);

    /**
     * @brief Return True if the feature is active, return false if not active.
     *
     * @details A feature can be activated/deactivated with the functions: GameKitSettingsActivateFeature() and GameKitSettingsDeactivateFeature()
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param featureType The feature to check.
     * @return True if the feature is active, false if not active.
     */
    GAMEKIT_API bool GameKitSettingsIsFeatureActive(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, GameKit::FeatureType featureType);

    /**
     * @brief Get all of the feature's variables as key-value pairs.
     *
     * @details The variables are returned through the callback and receiver.
     * The callback is invoked once for each variable. The variables are returned as key-value pairs of (variableName, variableValue).
     * The callback will not be invoked if the feature is missing from the settings file.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param receiver A pointer to an instance of a class where the results will be dispatched to.
     * This instance must have have a method signature of void ReceiveResult(const char* charKey, const char* charValue);
     * @param featureType The feature to get the variables for.
     * @param resultsCb A static dispatcher function pointer that receives key/value pairs.
     */
    GAMEKIT_API void GameKitSettingsGetFeatureVariables(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, GameKit::FeatureType featureType, KeyValueCharPtrCallbackDispatcher resultsCb);

    /**
     * @brief Get the value of the specified feature's variable.
     *
     * @details The value is returned through the callback and receiver.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param receiver A pointer to an instance of a class where the results will be dispatched to.
     * This instance must have a method signature of void ReceiveResult(const char* charPtr);
     * @param featureType The feature to get the variable for.
     * @param varName The name of the variable to get the value for.
     * @param resultsCb A static dispatcher function pointer that receives a character array.
     */
    GAMEKIT_API void GameKitSettingsGetFeatureVariable(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, GameKit::FeatureType featureType, const char* varName, CharPtrCallback resultsCb);

    /**
     * @brief Get the path to the "saveInfo.yml" settings file.
     *
     * @details The path is equal to "GAMEKIT_ROOT/shortGameName/saveInfo.yml".
     * The path is returned through the callback and receiver.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param receiver A pointer to an instance of a class where the results will be dispatched to.
     * This instance must have a method signature of void ReceiveResult(const char* charPtr);
     * @param resultsCb A static dispatcher function pointer that receives a character array.
     */
    GAMEKIT_API void GameKitSettingsGetSettingsFilePath(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, DISPATCH_RECEIVER_HANDLE receiver, CharPtrCallback resultsCb);

    /**
     * @brief Reload the GAMEKIT Settings YAML file from disk.
     *
     * @details Overwrites any changes made through the "Set", "Add/Delete", "Activate/Deactivate" methods.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     */
    GAMEKIT_API void GameKitSettingsReload(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance);

    /**
     * @brief Save a new profile to the Aws credentials file.
     *
     * @details If the profile already exists, will update the access key and secret key to match those passed in through this method.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param profileName The name of the profile we are saving or updating in the credentials ini file.
     * @param accessKey The access key of the Aws IAM role we are saving.
     * @param secretKey The secret key of the Aws IAM role we are saving.
     * @param logCb Callback function for logging information and errors.
     */
    GAMEKIT_API unsigned int GameKitSaveAwsCredentials(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* profileName, const char* accessKey, const char* secretKey, FuncLogCallback logCb);

    /**
     * @brief Sets the Aws access key of an existing profile.
     *
     * @details If the profile passed in does not exist, will not automatically create the profile and will return an error.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().
     * @param profileName The name of the profile we are updating in the credentials ini file.
     * @param newAccessKey The new access key that will be assigned to this profile.
     * @param logCb Callback function for logging information and errors.
     */
    GAMEKIT_API unsigned int GameKitSetAwsAccessKey(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* profileName, const char* newAccessKey, FuncLogCallback logCb);

    /**
     * @brief Sets the Aws secret key of an existing profile.
     *
     * @details If the profile passed in does not exist, will not automatically create the profile and will return an error.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().'
     * @param profileName The name of the profile we are saving our updating in the credentials ini file.
     * @param newSecretKey The new secret key that will be assigned to this profile.
     * @param logCb Callback function for logging information and errors.
     */
    GAMEKIT_API unsigned int GameKitSetAwsSecretKey(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* profileName, const char* newSecretKey, FuncLogCallback logCb);


    /**
     * @brief Gets the access key and secret key corresponding to a pre-existing profile in the Aws credentials file.
     *
     * @param settingsInstance Pointer to a GameKitSettings instance created with GameKitSettingsInstanceCreate().'
     * @param profileName The name of the profile we are getting the access key and secret from.
     * @param receiver A pointer to an instance of a class where the results will be dispatched to.
     * @param responseCallback A static dispatcher function pointer that receives two char* values corresponding to the access and secret key.
     * @param logCb Callback function for logging information and errors.
     */
    GAMEKIT_API unsigned int GameKitGetAwsProfile(GAMEKIT_SETTINGS_INSTANCE_HANDLE settingsInstance, const char* profileName, DISPATCH_RECEIVER_HANDLE receiver, FuncAwsProfileResponseCallback responseCallback, FuncLogCallback logCb);
#pragma endregion
}
