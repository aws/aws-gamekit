cmake_minimum_required(VERSION 3.20)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

project(aws_gamekit_py)
if (BUILD_SHARED_LIBS AND ${ENABLE_GAMEKIT_PYBIND})

  find_package(pybind11)
  find_package(Boost 1.76.0 COMPONENTS filesystem iostreams)
  find_package(AWSSDK REQUIRED COMPONENTS cloudformation cognito-idp lambda s3 ssm secretsmanager apigateway sts)

  # header and source files to compile
  include_directories(${Boost_INCLUDE_DIRS})

  # pybind
  pybind11_add_module(aws_gamekit_py SHARED "main.cpp")

  target_link_libraries(${PROJECT_NAME} PRIVATE
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
  )

  # copy target to install directory
  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

  add_custom_command(TARGET ${PROJECT_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-gamekit-core> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-gamekit-identity> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-gamekit-achievements> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-gamekit-authentication> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-gamekit-user-gameplay-data> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-gamekit-game-saving> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-core> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-cloudformation> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-cognito-idp> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-lambda> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-s3> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-ssm> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-secretsmanager> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-apigateway> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:aws-cpp-sdk-sts> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-crt-cpp> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-common> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-mqtt> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-auth> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-s3> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-http> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-io> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-cal> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-event-stream> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-c-compression> $<TARGET_FILE_DIR:aws_gamekit_py>
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AWS::aws-checksums> $<TARGET_FILE_DIR:aws_gamekit_py>
  )
else()
    message(STATUS "Skipping ${PROJECT_NAME} project generation")
endif()
