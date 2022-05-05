// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

/** @file
 */

#pragma once

#ifdef _WIN32
#define GAMEKIT_API __declspec(dllexport)
#else
#define GAMEKIT_API __attribute__ ((visibility ("default")))
#endif

#define GAMEKIT_SDK_OPTIONS_ALLOCATION_TAG "gamekit_sdk_options"
#define GAMEKIT_HTTP_CLIENT_FACTORY_ALLOCATION_TAG "gamekit_http_client"
#define AWS_LOGGING_ALLOCATION_TAG "aws_logging"
