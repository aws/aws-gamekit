# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 11)

project(aws-gamekit-core)

# AWS SDK and CRT dependencies
find_package(AWSSDK REQUIRED COMPONENTS apigateway cloudformation lambda s3 ssm secretsmanager sts)

# Boost
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.76.0 COMPONENTS filesystem iostreams)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARIES})

message("BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}")

if (ENABLE_CUSTOM_HTTP_CLIENT_FACTORY)
    add_compile_definitions(ENABLE_CUSTOM_HTTP_CLIENT_FACTORY=1)
endif()

if (ENABLE_CURL_CLIENT)
    add_compile_definitions(ENABLE_CURL_CLIENT=1)

    message("CURL_INCLUDE_DIR=${CURL_INCLUDE_DIR}")
    message("OPENSSL_INCLUDE_DIR=${OPENSSL_INCLUDE_DIR}")
    message("NGHTTP2_INCLUDE_DIR=${NGHTTP2_INCLUDE_DIR}")

    #curl
    include_directories(${CURL_INCLUDE_DIR})

    #openssl
    include_directories(${OPENSSL_INCLUDE_DIR})

    #nghttp2
    include_directories(${NGHTTP2_LIBRARY_DIR})
endif()

# yaml-cpp
find_package(yaml-cpp REQUIRED CONFIG)
include_directories(${yaml-cpp_INCLUDE_DIRS})
link_directories(${yaml-cpp_LIBRARIES})

# header and source files to compile
file(GLOB_RECURSE GAMEKIT_CORE_INCLUDES CONFIGURE_DEPENDS "include/aws/gamekit/core" *.h)
file(GLOB_RECURSE GAMEKIT_CORE_SOURCE CONFIGURE_DEPENDS "source/aws/gamekit/core" *.cpp)

# build aws-gamekit-core library
add_library(${PROJECT_NAME} ${GAMEKIT_CORE_INCLUDES} ${GAMEKIT_CORE_SOURCE})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/NODEFAULTLIB:msvcrt")
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/NODEFAULTLIB:msvcrt")
  set_target_properties(${PROJECT_NAME} PROPERTIES STATIC_LIBRARY_FLAGS  "/NODEFAULTLIB:msvcrt")
endif()

# Enable Unity build
set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ON)

# Exclude api_initializer.cpp from unity builds because it is known to include windows.h
# which will pollute other files in the same compilation unit with macros like GetMessage
if (WIN32)
  set_property(SOURCE "source/aws/gamekit/core/awsclients/api_initializer.cpp" PROPERTY SKIP_UNITY_BUILD_INCLUSION ON)
endif()

# expose include directories
target_include_directories(${PROJECT_NAME} PUBLIC include)

# Organize files in VS Filters
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${GAMEKIT_CORE_INCLUDES} ${GAMEKIT_CORE_SOURCE})

# link libraries
if (GAMEKIT_ANDROID)
    # Remove conflicting libraries from dependencies
    list(REMOVE_ITEM AWSSDK_PLATFORM_DEPS ${ANDROID_LIBS_TO_REMOVE})
    target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES} yaml-cpp aws-cpp-sdk-core aws-cpp-sdk-apigateway aws-cpp-sdk-cloudformation aws-cpp-sdk-lambda aws-cpp-sdk-s3 aws-cpp-sdk-ssm aws-cpp-sdk-secretsmanager aws-cpp-sdk-sts ${AWSSDK_PLATFORM_DEPS} ${ANDROID_CURL_SSL_LIBS})
else()
    target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES} yaml-cpp aws-cpp-sdk-core aws-cpp-sdk-apigateway aws-cpp-sdk-cloudformation aws-cpp-sdk-lambda aws-cpp-sdk-s3 aws-cpp-sdk-ssm aws-cpp-sdk-secretsmanager aws-cpp-sdk-sts ${AWSSDK_PLATFORM_DEPS})
endif()

# copy target to install directory
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION bin)
endif()

# copy common dependencies
install(FILES
  $<TARGET_FILE:aws-cpp-sdk-core>
  $<TARGET_FILE:aws-cpp-sdk-apigateway>
  $<TARGET_FILE:aws-cpp-sdk-cloudformation>
  $<TARGET_FILE:aws-cpp-sdk-lambda>
  $<TARGET_FILE:aws-cpp-sdk-s3>
  $<TARGET_FILE:aws-cpp-sdk-ssm>
  $<TARGET_FILE:aws-cpp-sdk-secretsmanager>
  $<TARGET_FILE:aws-cpp-sdk-sts>
  $<TARGET_FILE:AWS::aws-crt-cpp>
  $<TARGET_FILE:AWS::aws-c-common>
  $<TARGET_FILE:AWS::aws-c-mqtt>
  $<TARGET_FILE:AWS::aws-c-auth>
  $<TARGET_FILE:AWS::aws-c-s3>
  $<TARGET_FILE:AWS::aws-c-http>
  $<TARGET_FILE:AWS::aws-c-io>
  $<TARGET_FILE:AWS::aws-c-cal>
  $<TARGET_FILE:AWS::aws-c-event-stream>
  $<TARGET_FILE:AWS::aws-c-compression>
  $<TARGET_FILE:AWS::aws-checksums>
  TYPE BIN)

if (GAMEKIT_ANDROID)
    install(FILES
        $<TARGET_FILE:AWS::s2n>
        $<TARGET_FILE:OpenSSL::SSL>
        $<TARGET_FILE:OpenSSL::Crypto>
        $<TARGET_FILE:CURL::libcurl>
        $<TARGET_FILE:Boost::filesystem>
        $<TARGET_FILE:Boost::iostreams>
        $<TARGET_FILE:Boost::regex>
        $<TARGET_FILE:yaml-cpp>
        TYPE BIN)

    # For Android Shared Lib builds we need to copy the libc++_shared library as well when installing.
    string(TOLOWER "${ANDROID_STL}" ANDROID_STL_LOWER)
    if("${ANDROID_STL_LOWER}" STREQUAL "c++_shared")
        cmake_path(APPEND CMAKE_ANDROID_NDK "sources/cxx-stl/llvm-libc++/libs/${ANDROID_ABI}/libc++_shared.so" OUTPUT_VARIABLE STL_SHARED_LIB)
        INSTALL(FILES ${STL_SHARED_LIB} TYPE BIN)
    endif()
endif()

if (APPLE)
  install(FILES $<TARGET_FILE:AWS::aws-c-sdkutils> TYPE BIN)
endif()
