REM Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
REM SPDX-License-Identifier: Apache-2.0

@ECHO OFF

if DEFINED GAMEKIT_AWSSDK_PATH (goto set_boost_path)
set /P GAMEKIT_AWSSDK_PATH="Path to AWSSDK (ex: D:\development\AWSSDK): "
setx GAMEKIT_AWSSDK_PATH %GAMEKIT_AWSSDK_PATH%

:set_boost_path
if DEFINED GAMEKIT_BOOST_PATH (goto set_yamlcpp_path)
set /P GAMEKIT_BOOST_PATH="Path to Boost (ex: D:\development\boost_1_76_0): "
setx GAMEKIT_BOOST_PATH %GAMEKIT_BOOST_PATH%

:set_yamlcpp_path
if DEFINED GAMEKIT_YAMLCPP_PATH (goto set_gtest_path)
set /P GAMEKIT_YAMLCPP_PATH="Path to yaml-cpp (ex: D:\development\yaml-cpp): "
setx GAMEKIT_YAMLCPP_PATH %GAMEKIT_YAMLCPP_PATH%

:set_gtest_path
if DEFINED GAMEKIT_GTEST_PATH (goto set_pybind_path)
set /P GAMEKIT_GTEST_PATH="Path to googletest (ex: D:\development\googletest): "
setx GAMEKIT_GTEST_PATH %GAMEKIT_GTEST_PATH%

:set_pybind_path
if DEFINED GAMEKIT_PYBIND_PATH (goto ask_build_type)
set /P GAMEKIT_PYBIND_PATH="Path to pybind (ex: D:\development\pybind11): "
setx GAMEKIT_PYBIND_PATH %GAMEKIT_PYBIND_PATH%

:ask_build_type
if NOT [%1]==[] (goto set_build_type)
set /P build_type="Build Type (Debug|Release): "
goto default_gamekit_sdk_source_path

:set_build_type
set build_type=%1

:default_gamekit_sdk_source_path
if NOT [%2]==[] (goto set_gamekit_sdk_source_path)
set GAMEKIT_SOURCE_PATH=%~dp0..\..
goto start_build

:set_gamekit_sdk_source_path
set GAMEKIT_SOURCE_PATH=%2

:start_build
@ECHO ON
set CMAKE_PREFIX_PATH=%GAMEKIT_AWSSDK_PATH%\install\%build_type%
cmake -G "Visual Studio 16 2019" -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=%build_type% -Daws-c-http_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\aws-c-http\cmake -Daws-c-io_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\aws-c-io\cmake -Daws-c-common_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\aws-c-common\cmake -Daws-c-cal_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\aws-c-cal\cmake -Daws-c-compression_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\aws-c-compression\cmake -Daws-c-mqtt_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\aws-c-mqtt\cmake -Daws-c-auth_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\aws-c-auth\cmake -Daws-c-event-stream_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\aws-c-event-stream\cmake -Daws-checksums_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\aws-checksums\cmake -Daws-c-s3_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\aws-c-s3\cmake -DAWSSDK_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\cmake\AWSSDK -Daws-cpp-sdk-core_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\cmake\aws-cpp-sdk-core -Daws-crt-cpp_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\aws-crt-cpp\cmake -Daws-cpp-sdk-cloudformation_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\cmake\aws-cpp-sdk-cloudformation -Daws-cpp-sdk-cognito-idp_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\cmake\aws-cpp-sdk-cognito-idp -Daws-cpp-sdk-sts_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\cmake\aws-cpp-sdk-sts -Daws-cpp-sdk-secretsmanager_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\cmake\aws-cpp-sdk-secretsmanager -Daws-cpp-sdk-ssm_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\cmake\aws-cpp-sdk-ssm -Daws-cpp-sdk-s3_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\cmake\aws-cpp-sdk-s3 -Daws-cpp-sdk-lambda_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\cmake\aws-cpp-sdk-lambda -Daws-cpp-sdk-apigateway_DIR=%GAMEKIT_AWSSDK_PATH%\install\x86_64\windows\%build_type%\lib\cmake\aws-cpp-sdk-apigateway -DBOOST_ROOT=%GAMEKIT_BOOST_PATH% -DBOOST_LIBRARYDIR=%GAMEKIT_BOOST_PATH%\stage\lib -Dyaml-cpp_DIR=%GAMEKIT_YAMLCPP_PATH%\install\%build_type%\share\cmake\yaml-cpp -DGTEST_LINKED_AS_SHARED_LIBRARY=1 -DGTest_DIR=%GAMEKIT_GTEST_PATH%\build\install\lib\cmake\GTest -DGTEST_SRC=%GAMEKIT_GTEST_PATH%\build\install\include -Dpybind11_DIR=%GAMEKIT_PYBIND_PATH%\install\%build_type%\share\cmake\pybind11 -DCMAKE_INSTALL_PREFIX=%GAMEKIT_SOURCE_PATH%\install\%build_type% --log-level=Verbose %GAMEKIT_SOURCE_PATH%
@ECHO OFF
goto end

:usage
ECHO Run from the root of the repository
ECHO Usage: scripts\Win64\regenerate_projects.bat ^[Debug^|Release^] ^[^<Path To GameKit source^>^]
ECHO ---
ECHO Example: (with default GameKit source path: <script_path>\..\..)
ECHO scripts\Win64\regenerate_projects.bat Debug
:end
