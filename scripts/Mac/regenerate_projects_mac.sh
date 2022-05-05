#!/bin/bash
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

if [[ $# -eq 0 ]] ; then
    echo 'USAGE: ./regenerate_projects_mac.sh <AWSSDK_DIR> <BOOST_DIR> <yaml-cpp_DIR> <GTEST_DIR> <SOURCE_PATH> <$6|Release>'
    echo 'Example: ./regenerate_projects_mac.sh ~/development/AWSSDK_mac ~/development/boost_1_76_0 ~/development/yaml-cpp ~/development/googletest . $6'
    exit 1
fi

cmake -G "Xcode" -DCMAKE_BUILD_TYPE=$6 -DBUILD_SHARED_LIBS=ON \
-Daws-c-sdkutils_DIR=$1/install/x86_64/macos/$6/lib/aws-c-sdkutils/cmake \
-Daws-c-http_DIR=$1/install/x86_64/macos/$6/lib/aws-c-http/cmake \
-Daws-c-io_DIR=$1/install/x86_64/macos/$6/lib/aws-c-io/cmake \
-Daws-c-common_DIR=$1/install/x86_64/macos/$6/lib/aws-c-common/cmake \
-Daws-c-cal_DIR=$1/install/x86_64/macos/$6/lib/aws-c-cal/cmake \
-Daws-c-compression_DIR=$1/install/x86_64/macos/$6/lib/aws-c-compression/cmake \
-Daws-c-mqtt_DIR=$1/install/x86_64/macos/$6/lib/aws-c-mqtt/cmake \
-Daws-c-auth_DIR=$1/install/x86_64/macos/$6/lib/aws-c-auth/cmake \
-Daws-c-event-stream_DIR=$1/install/x86_64/macos/$6/lib/aws-c-event-stream/cmake \
-Daws-checksums_DIR=$1/install/x86_64/macos/$6/lib/aws-checksums/cmake \
-Daws-c-s3_DIR=$1/install/x86_64/macos/$6/lib/aws-c-s3/cmake \
-DAWSSDK_DIR=$1/install/x86_64/macos/$6/lib/cmake/AWSSDK \
-Daws-cpp-sdk-core_DIR=$1/install/x86_64/macos/$6/lib/cmake/aws-cpp-sdk-core \
-Daws-crt-cpp_DIR=$1/install/x86_64/macos/$6/lib/aws-crt-cpp/cmake \
-Daws-cpp-sdk-cloudformation_DIR=$1/install/x86_64/macos/$6/lib/cmake/aws-cpp-sdk-cloudformation \
-Daws-cpp-sdk-cognito-idp_DIR=$1/install/x86_64/macos/$6/lib/cmake/aws-cpp-sdk-cognito-idp \
-Daws-cpp-sdk-sts_DIR=$1/install/x86_64/macos/$6/lib/cmake/aws-cpp-sdk-sts \
-Daws-cpp-sdk-secretsmanager_DIR=$1/install/x86_64/macos/$6/lib/cmake/aws-cpp-sdk-secretsmanager \
-Daws-cpp-sdk-ssm_DIR=$1/install/x86_64/macos/$6/lib/cmake/aws-cpp-sdk-ssm \
-Daws-cpp-sdk-s3_DIR=$1/install/x86_64/macos/$6/lib/cmake/aws-cpp-sdk-s3 \
-Daws-cpp-sdk-lambda_DIR=$1/install/x86_64/macos/$6/lib/cmake/aws-cpp-sdk-lambda \
-Daws-cpp-sdk-apigateway_DIR=$1/install/x86_64/macos/$6/lib/cmake/aws-cpp-sdk-apigateway \
-Daws-c-sdkutils_DIR=$1/install/x86_64/macos/$6/lib/aws-c-sdkutils/cmake \
-DBOOST_ROOT=$2 -DBOOST_LIBRARYDIR=$2/stage/lib \
-Dyaml-cpp_DIR=$3/build_mac/install/x86_64/$6/share/cmake/yaml-cpp \
-DGTEST_LINKED_AS_SHARED_LIBRARY=1 -DGTest_DIR=$4/build_mac/install/x86_64/lib/cmake/GTest \
-DGTEST_SRC=$4/build_mac/install/x86_64/include \
-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 \
-DCMAKE_INSTALL_PREFIX=./install/x86_64/$6 \
$5
