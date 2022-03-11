# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

# AWS GameKit Tests
cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 11)
set(BUILD_SHARED_LIBS ON CACHE STRING "Link to shared libraries by default.")

project(aws-gamekit-cpp-tests)
if (GTEST_SRC)

    # pre-processor directives
    add_definitions(-DBOOST_ALL_NO_LIB)

    # Setup GTest
    enable_testing()
    find_package(GTest REQUIRED)

    include_directories(${GTEST_SRC})

    # AWS SDK and CRT dependencies
    find_package(AWSSDK REQUIRED COMPONENTS cloudformation cognito-idp lambda s3 ssm secretsmanager apigateway sts)

    # Boost
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME OFF)
    find_package(Boost 1.76.0 COMPONENTS filesystem iostreams)
    include_directories(${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARIES})

    # header and source files to compile
    # core tests
    file(GLOB_RECURSE GAMEKIT_CORE_TEST_FILES CONFIGURE_DEPENDS "core" *h *.cpp)
    file(GLOB_RECURSE GAMEKIT_CORE_MOCKS CONFIGURE_DEPENDS "core/mocks" *.h *.cpp)
    # identity tests
    file(GLOB_RECURSE GAMEKIT_IDENTITY_TEST_FILES CONFIGURE_DEPENDS "identity" *.cpp *.h)
    # achievements
    file(GLOB_RECURSE GAMEKIT_ACHIEVEMENTS_TEST_FILES CONFIGURE_DEPENDS "achievements" *.cpp *.h)

    # test data
    file(GLOB_RECURSE GAMEKIT_TEST_DATA CONFIGURE_DEPENDS "core/test_data" *.yml)

    # MSVS compile options
    if (WIN32)
      add_compile_options(/bigobj)
    endif()

    set(ALL_TEST_BUILD_SOURCE ${GAMEKIT_CORE_TEST_FILES} ${GAMEKIT_CORE_MOCKS} ${GAMEKIT_IDENTITY_TEST_FILES} ${GAMEKIT_ACHIEVEMENTS_TEST_FILES})
    set(ALL_TEST_PROJECT_SOURCE ${GAMEKIT_CORE_TEST_FILES} ${GAMEKIT_CORE_MOCKS} ${GAMEKIT_IDENTITY_TEST_FILES} ${GAMEKIT_ACHIEVEMENTS_TEST_FILES} ${GAMEKIT_TEST_DATA})

    # Compile tests
    add_executable(${PROJECT_NAME} ${ALL_TEST_BUILD_SOURCE})
    target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

    # Organize files in VS Filters
    source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${ALL_TEST_PROJECT_SOURCE})

    if (WIN32)
      set_property(TARGET ${PROJECT_NAME}
        PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()

    # Enable Unity build
    set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ON)

    target_include_directories(${PROJECT_NAME} PUBLIC include)

    # Remove conflicting libraries from dependencies
    if (GAMEKIT_ANDROID)
        # Remove conflicting libraries from dependencies
        list(REMOVE_ITEM AWSSDK_PLATFORM_DEPS ${ANDROID_LIBS_TO_REMOVE})
        # Link GTest, GameKit and AWS SDK/CRT
        target_link_libraries(${PROJECT_NAME}
            GTest::gtest
            GTest::gmock
            aws-gamekit-core
            aws-gamekit-identity
            aws-gamekit-achievements
            aws-gamekit-authentication
            aws-gamekit-user-gameplay-data
            aws-gamekit-game-saving
            aws-cpp-sdk-core
            aws-cpp-sdk-cloudformation
            aws-cpp-sdk-cognito-idp
            aws-cpp-sdk-lambda
            aws-cpp-sdk-s3
            aws-cpp-sdk-ssm
            aws-cpp-sdk-secretsmanager
            aws-cpp-sdk-apigateway
            aws-cpp-sdk-sts
            AWS::aws-crt-cpp
            AWS::aws-c-common
            AWS::aws-c-mqtt
            AWS::aws-c-auth
            AWS::aws-c-s3
            AWS::aws-c-http
            AWS::aws-c-io
            AWS::aws-c-cal
            AWS::aws-c-event-stream
            AWS::aws-c-compression
            AWS::aws-checksums
            ${AWSSDK_PLATFORM_DEPS} 
            ${ANDROID_CURL_SSL_LIBS}
        )
    else()
        # Link GTest, GameKit and AWS SDK/CRT
        target_link_libraries(${PROJECT_NAME}
            GTest::gtest
            GTest::gmock
            aws-gamekit-core
            aws-gamekit-identity
            aws-gamekit-achievements
            aws-gamekit-authentication
            aws-gamekit-user-gameplay-data
            aws-gamekit-game-saving
            aws-cpp-sdk-core
            aws-cpp-sdk-cloudformation
            aws-cpp-sdk-cognito-idp
            aws-cpp-sdk-lambda
            aws-cpp-sdk-s3
            aws-cpp-sdk-ssm
            aws-cpp-sdk-secretsmanager
            aws-cpp-sdk-apigateway
            aws-cpp-sdk-sts
            AWS::aws-crt-cpp
            AWS::aws-c-common
            AWS::aws-c-mqtt
            AWS::aws-c-auth
            AWS::aws-c-s3
            AWS::aws-c-http
            AWS::aws-c-io
            AWS::aws-c-cal
            AWS::aws-c-event-stream
            AWS::aws-c-compression
            AWS::aws-checksums
            ${AWSSDK_PLATFORM_DEPS} 
        )
    endif()
    
    # Copy required DLLs to output directory
    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:GTest::gtest> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:GTest::gmock> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-gamekit-core> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-gamekit-identity> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-gamekit-achievements> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-gamekit-authentication> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-gamekit-user-gameplay-data> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-gamekit-game-saving> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-core> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-cloudformation> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-cognito-idp> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-lambda> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-s3> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-ssm> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-secretsmanager> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-apigateway> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-sts> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-crt-cpp> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-common> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-mqtt> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-auth> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-s3> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-http> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-io> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-cal> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-event-stream> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-compression> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-checksums> $<TARGET_FILE_DIR:aws-gamekit-cpp-tests>
    )

    # Add a CTest test
    add_test(AllTestsSuite ${PROJECT_NAME})
else()
    message(STATUS "Skipping ${PROJECT_NAME} project generation")
endif()
