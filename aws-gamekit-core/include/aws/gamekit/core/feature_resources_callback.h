// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

extern "C"
{
    typedef void(*FuncResourceInfoCallback)(const char* logicalResourceId, const char* resourceType, const char* resourceStatus);
    typedef void(*DeployedParametersCallback)(const char* parameterKey, const char* parameterValue);
}
