# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

message("Build Configuration=${CMAKE_BUILD_TYPE}")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DGAMEKIT_DEBUG")

if(MSVC)
  # Enable multi-processor compilation
  add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
  add_compile_definitions(_CRT_NONSTDC_NO_DEPRECATE)
endif()

project(aws-gamekit-cpp)

string(TOLOWER "${TARGET_ARCH}" TARGET_ARCH_LOWER)
if ("${TARGET_ARCH_LOWER}" STREQUAL "android")
    # Android settings
    list(APPEND CMAKE_FIND_ROOT_PATH "${AWSSDK_ROOT_DIR}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${BOOST_ROOT}")
    list(APPEND CMAKE_PREFIX_PATH "${CURL_LIB_DIR}")
    list(APPEND CMAKE_PREFIX_PATH "${OPENSSL_ROOT_DIR}")
    
    message(WARNING "Disabling GameKit pybind integration for Android")
    set(ENABLE_GAMEKIT_PYBIND False)
    set(GAMEKIT_ANDROID True)

    # Curl and SSL libraries are auto included by AWS SDK, we want to set them explicitly from the
    # command line
    find_package(OpenSSL)
    find_package(CURL)

    set(ANDROID_LIBS_TO_REMOVE curl ssl crypto)
    set(ANDROID_CURL_SSL_LIBS OpenSSL::SSL OpenSSL::Crypto CURL::libcurl)

    string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
    if ("${CMAKE_BUILD_TYPE_LOWER}" STREQUAL "release")
        set_target_properties(CURL::libcurl PROPERTIES IMPORTED_LOCATION_RELEASE "${CURL_LIBRARY}")
    else()
        set_target_properties(CURL::libcurl PROPERTIES IMPORTED_LOCATION_DEBUG "${CURL_LIBRARY}")
    endif()
else()
    set(ENABLE_GAMEKIT_PYBIND True)

    set(GAMEKIT_ANDROID False)
    set(ANDROID_CURL_SSL_LIBS "")
endif()

# AWS SDK and CRT dependencies
find_package(AWSSDK REQUIRED COMPONENTS apigateway cloudformation cognito-idp lambda s3 ssm secretsmanager sts)

find_package(yaml-cpp)

add_subdirectory(aws_gamekit_py)
add_subdirectory(aws-gamekit-core)
add_subdirectory(aws-gamekit-identity)
add_subdirectory(aws-gamekit-achievements)
add_subdirectory(aws-gamekit-authentication)
add_subdirectory(aws-gamekit-user-gameplay-data)
add_subdirectory(aws-gamekit-game-saving)
add_subdirectory(tests)
add_subdirectory(publish)
