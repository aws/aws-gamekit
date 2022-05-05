# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 11)

project(aws-gamekit-game-saving)

find_package(AWSSDK REQUIRED COMPONENTS cognito-idp)
find_package(Boost 1.76.0 COMPONENTS filesystem iostreams)

if (ENABLE_CUSTOM_HTTP_CLIENT_FACTORY)
    add_compile_definitions(ENABLE_CUSTOM_HTTP_CLIENT_FACTORY=1)
endif()

if (ENABLE_CURL_CLIENT)
    add_compile_definitions(ENABLE_CURL_CLIENT=1)

    #curl
    include_directories(${CURL_INCLUDE_DIR})

    #openssl
    include_directories(${OPENSSL_INCLUDE_DIR})

    #nghttp2
    include_directories(${NGHTTP2_LIBRARY_DIR})
endif()

# header and source files to compile
include_directories(${Boost_INCLUDE_DIRS})
file(GLOB_RECURSE GAMEKIT_GAME_SAVING_INCLUDES CONFIGURE_DEPENDS "include/aws/gamekit/game-saving" *.h)
file(GLOB_RECURSE GAMEKIT_GAME_SAVING_SOURCE CONFIGURE_DEPENDS "source/aws/gamekit/game-saving" *.cpp)

# build aws-gamekit-game-saving as shared library
add_library(${PROJECT_NAME} ${GAMEKIT_GAME_SAVING_INCLUDES} ${GAMEKIT_GAME_SAVING_SOURCE})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# Enable Unity build
set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ON)

# expose include directories
target_include_directories(${PROJECT_NAME} PUBLIC include)

# link libraries
if (GAMEKIT_ANDROID)
    # Remove conflicting libraries from dependencies
    list(REMOVE_ITEM AWSSDK_PLATFORM_DEPS ${ANDROID_LIBS_TO_REMOVE})
    target_link_libraries(${PROJECT_NAME} aws-gamekit-core aws-gamekit-authentication ${Boost_LIBRARIES} aws-cpp-sdk-core aws-cpp-sdk-cognito-idp ${AWSSDK_PLATFORM_DEPS} ${ANDROID_CURL_SSL_LIBS})
else()
    target_link_libraries(${PROJECT_NAME} aws-gamekit-core aws-gamekit-authentication ${Boost_LIBRARIES} aws-cpp-sdk-core aws-cpp-sdk-cognito-idp ${AWSSDK_PLATFORM_DEPS})
endif()

# copy target to install directory
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION bin)
endif()

# copy common dependencies
install(FILES
    $<TARGET_FILE:aws-cpp-sdk-core>
    $<TARGET_FILE:aws-cpp-sdk-cognito-idp>
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
        TYPE BIN)
endif()
