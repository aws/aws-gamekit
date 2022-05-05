// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
// Standard Library
#include <memory>
#include <mutex>

// GameKit
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/logging.h>
#if ENABLE_CUSTOM_HTTP_CLIENT_FACTORY
#include <aws/gamekit/core/awsclients/http_client_factory.h>
#endif

// AWS SDK Forward declarations
namespace Aws {
    struct SDKOptions;
    namespace Http {
        class CreateHttpClient;
    }
}

namespace GameKit
{
    // This singleton class helps ensure that the AWS SDK InitAPI and ShutdownAPI methods are only called once
    class GAMEKIT_API AwsApiInitializer
    {
    private:
        static std::mutex m_mutex;
        static bool m_isAwsSdkInitialized;
        static std::unique_ptr<Aws::SDKOptions> m_awsSdkOptions;
        static int m_count;

    public:
        // Calls Aws::InitAPI()
        static void Initialize(FuncLogCallback log = nullptr, const void* caller = nullptr);

        // Calls Aws::ShutdownAPI()
        static void Shutdown(FuncLogCallback log = nullptr, const void* caller = nullptr, bool force = false);

        static bool IsInitialized();
    };
}
