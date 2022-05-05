REM Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
REM SPDX-License-Identifier: Apache-2.0

@ECHO OFF

REM Android platform variables
set ANDROID_API_LEVEL=24
set ARCH=arm
set PLATFORM=android

if [%1]==[] (goto usage)

set GAMEKIT_ANDROID_AWSSDK_PATH=%1
set GAMEKIT_ANDROID_BOOST_PATH=%2
set GAMEKIT_ANDROID_YAMLCPP_PATH=%3
set GAMEKIT_ANDROID_GTEST_PATH=%4
set GAMEKIT_ANDROID_PATH=%5
set build_type=%6

:start_build
@ECHO ON
cmake -G "Ninja" -DBoost_NO_WARN_NEW_VERSIONS=1 -DTARGET_ARCH=ANDROID -DCMAKE_TOOLCHAIN_FILE=%NDKROOT%\build\cmake\android.toolchain.cmake ^
-DANDROID_ABI=armeabi-v7a -DANDROID_NATIVE_API_LEVEL=%ANDROID_API_LEVEL% -DANDROID_NDK=%NDKROOT% -DANDROID_PLATFORM=android-%ANDROID_API_LEVEL% ^
-DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=%build_type% -DCMAKE_CXX_COMPILER_VERSION=clang ^
-DAWSSDK_ROOT_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type% -DANDROID_STL=c++_static ^
-DCURL_LIB_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\external-install\curl ^
-DCURL_INCLUDE_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\external-install\curl\include  ^
-DCURL_LIBRARY:FILEPATH=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\external-install\curl\lib\libcurl.a ^
-DOPENSSL_ROOT_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\external-install\openssl ^
-DOPENSSL_INCLUDE_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\external-install\openssl\include ^
-DOPENSSL_CRYPTO_LIBRARY:FILEPATH=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\external-install\openssl\lib\libcrypto.a ^
-DOPENSSL_SSL_LIBRARY:FILEPATH=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\external-install\openssl\lib\libssl.a ^
-Ds2n_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\s2n\cmake ^
-Daws-c-http_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\aws-c-http\cmake ^
-Daws-c-io_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\aws-c-io\cmake ^
-Daws-c-common_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\aws-c-common\cmake ^
-Daws-c-cal_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\aws-c-cal\cmake ^
-Daws-c-compression_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\aws-c-compression\cmake ^
-Daws-c-mqtt_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\aws-c-mqtt\cmake ^
-Daws-c-auth_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\aws-c-auth\cmake ^
-Daws-c-event-stream_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\aws-c-event-stream\cmake ^
-Daws-checksums_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\aws-checksums\cmake ^
-Daws-c-s3_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\aws-c-s3\cmake ^
-DAWSSDK_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\cmake\AWSSDK ^
-Daws-cpp-sdk-core_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\cmake\aws-cpp-sdk-core ^
-Daws-crt-cpp_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\aws-crt-cpp\cmake ^
-Daws-cpp-sdk-cloudformation_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\cmake\aws-cpp-sdk-cloudformation ^
-Daws-cpp-sdk-cognito-idp_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\cmake\aws-cpp-sdk-cognito-idp ^
-Daws-cpp-sdk-sts_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\cmake\aws-cpp-sdk-sts ^
-Daws-cpp-sdk-secretsmanager_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\cmake\aws-cpp-sdk-secretsmanager ^
-Daws-cpp-sdk-ssm_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\cmake\aws-cpp-sdk-ssm ^
-Daws-cpp-sdk-s3_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\cmake\aws-cpp-sdk-s3 ^
-Daws-cpp-sdk-lambda_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\cmake\aws-cpp-sdk-lambda ^
-Daws-cpp-sdk-apigateway_DIR=%GAMEKIT_ANDROID_AWSSDK_PATH%\install\%ARCH%\%PLATFORM%\%build_type%\lib\cmake\aws-cpp-sdk-apigateway ^
-DBoost_ARCHITECTURE="-a32" -DBoost_DIR=%GAMEKIT_ANDROID_BOOST_PATH% -DBOOST_ROOT=%GAMEKIT_ANDROID_BOOST_PATH% ^
-DBoost_INCLUDE_DIR:FILEPATH=%GAMEKIT_ANDROID_BOOST_PATH% -DBoost_NO_SYSTEM_PATHS=ON -Dboost_COMPILER="-clang" ^
-Dyaml-cpp_DIR=%GAMEKIT_ANDROID_YAMLCPP_PATH%\install\%build_type%\share\cmake\yaml-cpp -DENABLE_CUSTOM_HTTP_CLIENT_FACTORY=ON ^
-DENABLE_CURL_CLIENT=ON -DGTEST_LINKED_AS_SHARED_LIBRARY=1 -DGTest_DIR=%GAMEKIT_ANDROID_GTEST_PATH%\install\%build_type%\lib\cmake\GTest ^
-DGTEST_SRC=%GAMEKIT_ANDROID_GTEST_PATH%\install\%build_type%\include -DCMAKE_INSTALL_PREFIX=%~dp0\..\..\install\%build_type% ^
-DENABLE_GAMEKIT_PYBIND=False --log-level=Verbose %GAMEKIT_ANDROID_PATH%
@ECHO OFF
goto end

:usage
ECHO Usage: regenerate_android_projects.bat ^<Path to AWS SDK^> ^<Path to Boost^> ^<Path to yaml-cpp^> ^<Path to Gtest^> ^<Path To GameKit source^> ^[Debug^|Release^]
ECHO ---
ECHO Example:
ECHO regenerate_android_projects.bat . Debug

:end
