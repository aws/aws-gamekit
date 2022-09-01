// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// GTest
#include <gtest/gtest.h>

// GameKit
#include <aws/gamekit/core/deployment_orchestrator.h>
#include <aws/gamekit/core/feature_resources.h>
#include <aws/gamekit/core/gamekit_account.h>

// Mocks
#include "mocks/mock_apigateway_client.h"
#include "mocks/mock_cloudformation_client.h"
#include "mocks/mock_cognito_client.h"
#include "mocks/mock_feature_resources.h"
#include "mocks/mock_gamekit_account.h"
#include "mocks/fake_http_client.h"
#include "mocks/mock_secretsmanager_client.h"
#include "mocks/mock_ssm_client.h"
#include "mocks/mock_sts_client.h"
#include "mocks/mock_s3_client.h"