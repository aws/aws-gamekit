// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "test_common.h"
#include <gtest/gtest.h>
#include <aws/gamekit/core/exports.h>

namespace GameKit
{
    namespace Tests
    {
        namespace CoreExports
        {
            static const char* DUMMY_INSTANCE_PATH = "tests/core/test_data/sampleplugin/dummyinstance";
            std::unique_ptr<GameKit::Mocks::MockS3Client> coreS3Mock;
            std::unique_ptr<GameKit::Mocks::MockSSMClient> coreSsmMock;
            std::unique_ptr<GameKit::Mocks::MockCloudFormationClient> coreCfnMock;
            std::unique_ptr<GameKit::Mocks::MockSecretsManagerClient> coreSecretsMock;
            std::unique_ptr<GameKit::Mocks::MockAPIGatewayClient> coreApigwMock;
            
            using namespace testing;
            namespace S3Model = Aws::S3::Model;
            namespace SSMModel = Aws::SSM::Model;
            namespace CfnModel = Aws::CloudFormation::Model;
            namespace SecretsModel = Aws::SecretsManager::Model;
            namespace ApigwModel = Aws::APIGateway::Model;

            class GameKitCoreExportsTestFixture;
            void* createAccountInstance();
            void* createFeatureResourceInstance(GameKit::FeatureType featureType);
            void setAccountMocks(void*);
            void setResourceMocks(void*);

            // Templated structs that forward a callback to an object
            template <typename Target, Target> struct CallbackHandler;
            template <typename Target, typename RetType, typename ... Args, RetType (Target::*CbFunc)(Args...)>
            struct CallbackHandler<RetType(Target::*)(Args...), CbFunc>
            {
                static RetType OnCallback(void* obj, Args... args)
                {
                    Target* instance = static_cast<Target*>(obj);
                    return (instance->*CbFunc)(std::forward<Args>(args)...);
                }

                static RetType OnCallback(void* obj, Args&&... args)
                {
                    Target* instance = static_cast<Target*>(obj);
                    return (instance->*CbFunc)(std::forward<Args>(args)...);
                }
            };

            // Used as callback to handle c-strings
            class SimpleCaller
            {
            public:
                virtual void OnHandleResult(const char* result) = 0;
            };
        }
    }
}
