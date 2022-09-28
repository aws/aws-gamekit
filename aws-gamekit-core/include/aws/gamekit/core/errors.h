// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

/** @file
 * @brief The GameKit status codes.
 *
 * @details View the status codes by clicking "Go to the source code of this file" at the top of the page.
 */

#pragma once
namespace GameKit
{
    // Standard status codes (0-500)
    static const unsigned int GAMEKIT_SUCCESS = 0x0;
    static const unsigned int GAMEKIT_ERROR_INVALID_PROVIDER = 0x2;
    static const unsigned int GAMEKIT_ERROR_PARAMETERS_FILE_SAVE_FAILED = 0x3;
    static const unsigned int GAMEKIT_ERROR_CLOUDFORMATION_FILE_SAVE_FAILED = 0x4;
    static const unsigned int GAMEKIT_ERROR_SETTINGS_FILE_SAVE_FAILED = 0x5;
    static const unsigned int GAMEKIT_ERROR_NO_ID_TOKEN = 0x6;
    static const unsigned int GAMEKIT_ERROR_HTTP_REQUEST_FAILED = 0x7;
    static const unsigned int GAMEKIT_ERROR_PARSE_JSON_FAILED = 0x8;
    static const unsigned int GAMEKIT_ERROR_SIGN_REQUEST_FAILED= 0x9;
    static const unsigned int GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED = 0xA;
    static const unsigned int GAMEKIT_ERROR_FILE_OPEN_FAILED = 0xB;
    static const unsigned int GAMEKIT_ERROR_FILE_WRITE_FAILED = 0xC;
    static const unsigned int GAMEKIT_ERROR_FILE_READ_FAILED = 0xD;
    static const unsigned int GAMEKIT_ERROR_DIRECTORY_CREATE_FAILED = 0xE;
    static const unsigned int GAMEKIT_ERROR_DIRECTORY_NOT_FOUND = 0xF;
    static const unsigned int GAMEKIT_ERROR_FUNCTIONS_COPY_FAILED = 0x10;
    static const unsigned int GAMEKIT_ERROR_METHOD_NOT_IMPLEMENTED = 0x15E;
    static const unsigned int GAMEKIT_ERROR_GENERAL = 0x15F;
    static const unsigned int GAMEKIT_ERROR_REGION_CODE_CONVERSION_FAILED = 0x160;
    static const unsigned int GAMEKIT_ERROR_CREDENTIALS_FILE_NOT_FOUND = 0x161;
    static const unsigned int GAMEKIT_ERROR_CREDENTIALS_FILE_SAVE_FAILED = 0x162;
    static const unsigned int GAMEKIT_ERROR_CREDENTIALS_NOT_FOUND = 0x163;
    static const unsigned int GAMEKIT_ERROR_CREDENTIALS_FILE_MALFORMED = 0x164;
    static const unsigned int GAMEKIT_ERROR_REQUEST_TIMED_OUT = 0x165;
    static const unsigned int GAMEKIT_ERROR_SETTINGS_MISSING = 0x166;
   
    // Bootstrapping status codes (501-1000)
    static const unsigned int GAMEKIT_ERROR_BOOTSTRAP_BUCKET_LOOKUP_FAILED = 0x1F5;
    static const unsigned int GAMEKIT_ERROR_BOOTSTRAP_BUCKET_CREATION_FAILED = 0x1F6;
    static const unsigned int GAMEKIT_ERROR_BOOTSTRAP_INVALID_REGION_CODE = 0x1F7;
    static const unsigned int GAMEKIT_ERROR_BOOTSTRAP_MISSING_PLUGIN_ROOT = 0x1F8;
    static const unsigned int GAMEKIT_ERROR_BOOTSTRAP_REGION_CODE_CONVERSION_FAILED = 0x1F9;
    static const unsigned int GAMEKIT_ERROR_BOOTSTRAP_TOO_MANY_BUCKETS = 0x1FA;

    // Resource creation status codes (1001-1500)
    static const unsigned int GAMEKIT_ERROR_FUNCTIONS_PATH_NOT_FOUND = 0x3E9;
    static const unsigned int GAMEKIT_ERROR_CLOUDFORMATION_PATH_NOT_FOUND = 0x3EA;
    static const unsigned int GAMEKIT_ERROR_FUNCTION_ZIP_INIT_FAILED = 0x3EB;
    static const unsigned int GAMEKIT_ERROR_FUNCTION_ZIP_WRITE_FAILED = 0x3EC;
    static const unsigned int GAMEKIT_ERROR_PARAMSTORE_WRITE_FAILED = 0x3ED;
    static const unsigned int GAMEKIT_ERROR_BOOTSTRAP_BUCKET_UPLOAD_FAILED = 0x3EE;
    static const unsigned int GAMEKIT_ERROR_SECRETSMANAGER_WRITE_FAILED = 0x3EF;
    static const unsigned int GAMEKIT_ERROR_CLOUDFORMATION_STACK_CREATION_FAILED = 0x3F0;
    static const unsigned int GAMEKIT_ERROR_CLOUDFORMATION_STACK_UPDATE_FAILED = 0x3F1;
    static const unsigned int GAMEKIT_ERROR_CLOUDFORMATION_RESOURCE_CREATION_FAILED = 0x3F2;
    static const unsigned int GAMEKIT_ERROR_CLOUDFORMATION_STACK_DELETE_FAILED = 0x3F3;
    static const unsigned int GAMEKIT_ERROR_CLOUDFORMATION_DESCRIBE_RESOURCE_FAILED = 0x3F4;
    static const unsigned int GAMEKIT_ERROR_CLOUDFORMATION_DESCRIBE_STACKS_FAILED = 0x3F5;
    static const unsigned int GAMEKIT_ERROR_APIGATEWAY_DEPLOYMENT_CREATION_FAILED = 0x3F6;
    static const unsigned int GAMEKIT_ERROR_APIGATEWAY_STAGE_DEPLOYMENT_FAILED = 0x3F7;
    static const unsigned int GAMEKIT_ERROR_LAYERS_PATH_NOT_FOUND = 0x3F8;
    static const unsigned int GAMEKIT_ERROR_LAYER_ZIP_INIT_FAILED = 0x3F9;
    static const unsigned int GAMEKIT_ERROR_LAYER_ZIP_WRITE_FAILED = 0x3FA;
    static const unsigned int GAMEKIT_ERROR_LAYER_CREATION_FAILED = 0x3FB;
    static const unsigned int GAMEKIT_ERROR_CLOUDFORMATION_GET_TEMPLATE_FAILED = 0x3FC;
    static const unsigned int GAMEKIT_ERROR_PARAMSTORE_READ_FAILED = 0x3FD;
    static const unsigned int GAMEKIT_ERROR_CLOUDFORMATION_NO_CURRENT_STACK_STATUS = 0x3FE;
    static const unsigned int GAMEKIT_ERROR_FEATURE_NOT_AVAILABLE = 0x3FF;

    // Orchestration status codes (1501 - 2000)
    static const unsigned int GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_STATE = 0x5DD;
    static const unsigned int GAMEKIT_ERROR_ORCHESTRATION_INVALID_FEATURE_SETTINGS = 0x5DE;
    static const unsigned int GAMEKIT_ERROR_ORCHESTRATION_DEPLOYMENT_IN_PROGRESS = 0x5DF;

    // Identity status codes (0x10000 - 0x103FF)
    static const unsigned int GAMEKIT_ERROR_REGISTER_USER_FAILED = 0x10000;
    static const unsigned int GAMEKIT_ERROR_CONFIRM_REGISTRATION_FAILED = 0x10001;
    static const unsigned int GAMEKIT_ERROR_RESEND_CONFIRMATION_CODE_FAILED = 0x10002;
    static const unsigned int GAMEKIT_ERROR_LOGIN_FAILED = 0x10003;
    static const unsigned int GAMEKIT_ERROR_FORGOT_PASSWORD_FAILED = 0x10004;
    static const unsigned int GAMEKIT_ERROR_CONFIRM_FORGOT_PASSWORD_FAILED = 0x10005;
    static const unsigned int GAMEKIT_ERROR_LOGOUT_FAILED = 0x10007;
    static const unsigned int GAMEKIT_ERROR_MALFORMED_USERNAME = 0x10008;
    static const unsigned int GAMEKIT_ERROR_MALFORMED_PASSWORD = 0x10009;
    static const unsigned int GAMEKIT_ERROR_INVALID_FEDERATED_IDENTITY_PROVIDER = 0x10010;

    // Authentication status codes (0x10400 - 0x107FF)

    // Achievements status codes (0x10800 - 0x10BFF)
    static const unsigned int GAMEKIT_ERROR_ACHIEVEMENTS_ICON_UPLOAD_FAILED = 0x10800;
    static const unsigned int GAMEKIT_ERROR_ACHIEVEMENTS_INVALID_ID = 0x10801;
    static const unsigned int GAMEKIT_ERROR_ACHIEVEMENTS_PAYLOAD_TOO_LARGE = 0x10802;

    // User Gameplay Data status codes (0x10C00 - 0x10FFF)
    static const unsigned int GAMEKIT_ERROR_USER_GAMEPLAY_DATA_PAYLOAD_INVALID = 0x010C00;
    static const unsigned int GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_FAILED = 0x010C01;
    static const unsigned int GAMEKIT_ERROR_USER_GAMEPLAY_DATA_API_CALL_DROPPED = 0x010C02;
    static const unsigned int GAMEKIT_WARNING_USER_GAMEPLAY_DATA_API_CALL_ENQUEUED = 0x010C03;
    static const unsigned int GAMEKIT_ERROR_MALFORMED_BUNDLE_NAME = 0x010C04;
    static const unsigned int GAMEKIT_ERROR_MALFORMED_BUNDLE_ITEM_KEY = 0x010C05;
    static const unsigned int GAMEKIT_ERROR_USER_GAMEPLAY_DATA_CACHE_WRITE_FAILED = 0x010C06;
    static const unsigned int GAMEKIT_ERROR_USER_GAMEPLAY_DATA_CACHE_READ_FAILED = 0x010C07;
    static const unsigned int GAMEKIT_ERROR_USER_GAMEPLAY_DATA_UNPROCESSED_ITEMS = 0x010C08;
    static const unsigned int GAMEKIT_ERROR_USER_GAMEPLAY_DATA_PAYLOAD_TOO_LARGE = 0x010C09;

    // Game Saving status codes (0x11000 - 0x113FF)
    static const unsigned int GAMEKIT_ERROR_GAME_SAVING_SLOT_NOT_FOUND = 0x11000;
    static const unsigned int GAMEKIT_ERROR_GAME_SAVING_CLOUD_SLOT_IS_NEWER = 0x11001;
    static const unsigned int GAMEKIT_ERROR_GAME_SAVING_SYNC_CONFLICT = 0x11002;
    static const unsigned int GAMEKIT_ERROR_GAME_SAVING_DOWNLOAD_SLOT_ALREADY_IN_SYNC = 0x11003;
    static const unsigned int GAMEKIT_ERROR_GAME_SAVING_UPLOAD_SLOT_ALREADY_IN_SYNC = 0x11004;
    static const unsigned int GAMEKIT_ERROR_GAME_SAVING_EXCEEDED_MAX_SIZE = 0x11005;
    static const unsigned int GAMEKIT_ERROR_GAME_SAVING_FILE_EMPTY = 0x11006;
    static const unsigned int GAMEKIT_ERROR_GAME_SAVING_FILE_FAILED_TO_OPEN = 0x11007;
    static const unsigned int GAMEKIT_ERROR_GAME_SAVING_LOCAL_SLOT_IS_NEWER = 0x11008;
    static const unsigned int GAMEKIT_ERROR_GAME_SAVING_SLOT_UNKNOWN_SYNC_STATUS = 0x11009;
    static const unsigned int GAMEKIT_ERROR_GAME_SAVING_MALFORMED_SLOT_NAME = 0x1100A;
    static const unsigned int GAMEKIT_ERROR_GAME_SAVING_MISSING_SHA = 0x1100B;
    static const unsigned int GAMEKIT_ERROR_GAME_SAVING_SLOT_TAMPERED = 0x1100C;
    static const unsigned int GAMEKIT_ERROR_GAME_SAVING_BUFFER_TOO_SMALL = 0x1100D;
    static const unsigned int GAMEKIT_ERROR_GAME_SAVING_MAX_CLOUD_SLOTS_EXCEEDED = 0x1100E;

    // Standard warning status codes (0x11400-0x116FF)
    static const unsigned int GAMEKIT_WARNING_SECRETSMANAGER_SECRET_NOT_FOUND = 0x11400;

    // Error messages
    static const char* ERR_INVALID_GAMEKIT_PROVIDER = "Invalid Provider";
    static const char* ERR_STACK_CURRENT_STATUS_UNDEPLOYED = "UNDEPLOYED";
}
