@ECHO OFF
REM Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
REM SPDX-License-Identifier: Apache-2.0

if [%1]==[] goto usage

if NOT [%2]==[] (goto set_gamekit_sdk_source_path)
set GAMEKIT_SOURCE_PATH=%~dp0..\..
goto start_copy

:set_gamekit_sdk_source_path
set GAMEKIT_SOURCE_PATH=%2

:start_copy
mkdir %1\AwsGameKit\Libraries\include\aws\gamekit\achievements\
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-achievements\include\aws\gamekit\achievements\exports.h %1\AwsGameKit\Libraries\include\aws\gamekit\achievements\exports.h
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-achievements\include\aws\gamekit\achievements\gamekit_achievements_models.h %1\AwsGameKit\Libraries\include\aws\gamekit\achievements\gamekit_achievements_models.h

mkdir %1\AwsGameKit\Libraries\include\aws\gamekit\authentication\
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-authentication\include\aws\gamekit\authentication\exports.h %1\AwsGameKit\Libraries\include\aws\gamekit\authentication\exports.h

mkdir %1\AwsGameKit\Libraries\include\aws\gamekit\core\
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-core\include\aws\gamekit\core\api.h %1\AwsGameKit\Libraries\include\aws\gamekit\core\api.h
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-core\include\aws\gamekit\core\enums.h %1\AwsGameKit\Libraries\include\aws\gamekit\core\enums.h
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-core\include\aws\gamekit\core\errors.h %1\AwsGameKit\Libraries\include\aws\gamekit\core\errors.h
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-core\include\aws\gamekit\core\exports.h %1\AwsGameKit\Libraries\include\aws\gamekit\core\exports.h
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-core\include\aws\gamekit\core\feature_resources_callback.h %1\AwsGameKit\Libraries\include\aws\gamekit\core\feature_resources_callback.h
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-core\include\aws\gamekit\core\logging.h %1\AwsGameKit\Libraries\include\aws\gamekit\core\logging.h

mkdir %1\AwsGameKit\Libraries\include\aws\gamekit\core\model\
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-core\include\aws\gamekit\core\model\account_credentials.h %1\AwsGameKit\Libraries\include\aws\gamekit\core\model\account_credentials.h
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-core\include\aws\gamekit\core\model\account_info.h %1\AwsGameKit\Libraries\include\aws\gamekit\core\model\account_info.h
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-core\include\aws\gamekit\core\model\config_consts.h %1\AwsGameKit\Libraries\include\aws\gamekit\core\model\config_consts.h
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-core\include\aws\gamekit\core\model\resource_environment.h %1\AwsGameKit\Libraries\include\aws\gamekit\core\model\resource_environment.h
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-core\include\aws\gamekit\core\model\template_consts.h %1\AwsGameKit\Libraries\include\aws\gamekit\core\model\template_consts.h

mkdir %1\AwsGameKit\Libraries\include\aws\gamekit\core\utils\
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-core\include\aws\gamekit\core\utils\encoding_utils.h %1\AwsGameKit\Libraries\include\aws\gamekit\core\utils\encoding_utils.h
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-core\include\aws\gamekit\core\utils\gamekit_httpclient_callbacks.h %1\AwsGameKit\Libraries\include\aws\gamekit\core\utils\gamekit_httpclient_callbacks.h
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-core\include\aws\gamekit\core\utils\ticker.h %1\AwsGameKit\Libraries\include\aws\gamekit\core\utils\ticker.h

mkdir %1\AwsGameKit\Libraries\include\aws\gamekit\game-saving\
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-game-saving\include\aws\gamekit\game-saving\exports.h %1\AwsGameKit\Libraries\include\aws\gamekit\game-saving\exports.h
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-game-saving\include\aws\gamekit\game-saving\gamekit_game_saving_models.h %1\AwsGameKit\Libraries\include\aws\gamekit\game-saving\gamekit_game_saving_models.h

mkdir %1\AwsGameKit\Libraries\include\aws\gamekit\identity\
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-identity\include\aws\gamekit\identity\exports.h %1\AwsGameKit\Libraries\include\aws\gamekit\identity\exports.h
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-identity\include\aws\gamekit\identity\gamekit_identity_models.h %1\AwsGameKit\Libraries\include\aws\gamekit\identity\gamekit_identity_models.h

mkdir %1\AwsGameKit\Libraries\include\aws\gamekit\user-gameplay-data\
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-user-gameplay-data\include\aws\gamekit\user-gameplay-data\exports.h %1\AwsGameKit\Libraries\include\aws\gamekit\user-gameplay-data\exports.h
copy %GAMEKIT_SOURCE_PATH%\aws-gamekit-user-gameplay-data\include\aws\gamekit\user-gameplay-data\gamekit_user_gameplay_data_models.h %1\AwsGameKit\Libraries\include\aws\gamekit\user-gameplay-data\gamekit_user_gameplay_data_models.h

python %1\AwsGameKit\generate_error_code_blueprint.py %1

goto end

:usage
ECHO Run in CMD
ECHO copyheaders.bat ^<FULL_PLUGINS_PATH^> ^[^<GAMEKIT_SDK_ROOT_PATH^>^]
ECHO Where: FULL_PLUGINS_PATH is the path to the Plugin directory of either the Unreal Sandbox Game or the Plugin repository
ECHO Where: GAMEKIT_SDK_ROOT_PATH is the path to the GameKit SDK source root (optional)
ECHO ---
ECHO Example: scripts\Win64\copyheaders.bat D:\workspace\GameKit\src\GameKitUnrealPlugin\AwsGameKitUnrealGame\Plugins
goto end

:done
ECHO Done

:end
