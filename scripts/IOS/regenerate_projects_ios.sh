#!/bin/bash
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

if [[ $# -eq 0 ]] ; then
    echo 'USAGE: ./regenerate_projects_ios.sh <AWSSDK_DIR> <BOOST_DIR> <yaml-cpp_DIR> <curl_DIR> <SOURCE_PATH> <Debug|Release>'
    echo 'Example: ./regenerate_projects_ios.sh ~/development/AWSSDK_ios ~/development/boost_1_76_0 ~/development/yaml-cpp ~/development/Build-OpenSSL-cURL . Debug'
    exit 1
fi

cmake -G "Xcode" -DCMAKE_BUILD_TYPE=$6 -DBUILD_SHARED_LIBS=OFF \
-Daws-c-http_DIR=$1/install/arm64/ios/$6/lib/aws-c-http/cmake \
-Daws-c-io_DIR=$1/install/arm64/ios/$6/lib/aws-c-io/cmake \
-Daws-c-common_DIR=$1/install/arm64/ios/$6/lib/aws-c-common/cmake \
-Daws-c-cal_DIR=$1/install/arm64/ios/$6/lib/aws-c-cal/cmake \
-Daws-c-compression_DIR=$1/install/arm64/ios/$6/lib/aws-c-compression/cmake \
-Daws-c-mqtt_DIR=$1/install/arm64/ios/$6/lib/aws-c-mqtt/cmake \
-Daws-c-auth_DIR=$1/install/arm64/ios/$6/lib/aws-c-auth/cmake \
-Daws-c-event-stream_DIR=$1/install/arm64/ios/$6/lib/aws-c-event-stream/cmake \
-Daws-checksums_DIR=$1/install/arm64/ios/$6/lib/aws-checksums/cmake \
-Daws-c-s3_DIR=$1/install/arm64/ios/$6/lib/aws-c-s3/cmake \
-DAWSSDK_DIR=$1/install/arm64/ios/$6/lib/cmake/AWSSDK \
-Daws-cpp-sdk-core_DIR=$1/install/arm64/ios/$6/lib/cmake/aws-cpp-sdk-core \
-Daws-crt-cpp_DIR=$1/install/arm64/ios/$6/lib/aws-crt-cpp/cmake \
-Daws-cpp-sdk-cloudformation_DIR=$1/install/arm64/ios/$6/lib/cmake/aws-cpp-sdk-cloudformation \
-Daws-cpp-sdk-cognito-idp_DIR=$1/install/arm64/ios/$6/lib/cmake/aws-cpp-sdk-cognito-idp \
-Daws-cpp-sdk-sts_DIR=$1/install/arm64/ios/$6/lib/cmake/aws-cpp-sdk-sts \
-Daws-cpp-sdk-secretsmanager_DIR=$1/install/arm64/ios/$6/lib/cmake/aws-cpp-sdk-secretsmanager \
-Daws-cpp-sdk-ssm_DIR=$1/install/arm64/ios/$6/lib/cmake/aws-cpp-sdk-ssm \
-Daws-cpp-sdk-s3_DIR=$1/install/arm64/ios/$6/lib/cmake/aws-cpp-sdk-s3 \
-Daws-cpp-sdk-lambda_DIR=$1/install/arm64/ios/$6/lib/cmake/aws-cpp-sdk-lambda \
-Daws-cpp-sdk-apigateway_DIR=$1/install/arm64/ios/$6/lib/cmake/aws-cpp-sdk-apigateway \
-Daws-c-sdkutils_DIR=$1/install/arm64/ios/$6/lib/aws-c-sdkutils/cmake -DBOOST_ROOT=$2 \
-DBOOST_LIBRARYDIR=$2/stage/lib -Dyaml-cpp_DIR=$3/build_ios/install/arm64/$6/share/cmake/yaml-cpp \
-DCURL_INCLUDE_DIR=$4/curl/include -DOPENSSL_INCLUDE_DIR=$4/openssl/IOS/include \
-DNGHTTP2_INCLUDE_DIR=$4/nghttp2/iOS/arm64/include -DCMAKE_XCODE_ATTRIBUTE_ARCHS="arm64" \
-DCMAKE_INSTALL_PREFIX=$5/build_ios/install/arm64/$6 -DCMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET=14.0 \
-DCMAKE_XCODE_ATTRIBUTE_SDK_NAME=iOS -DCMAKE_XCODE_ATTRIBUTE_SDKROOT=iphoneos14.0 \
-DENABLE_CUSTOM_HTTP_CLIENT_FACTORY=ON -DENABLE_CURL_CLIENT=ON -DENABLE_CURL_LOGGING=OFF \
$5
