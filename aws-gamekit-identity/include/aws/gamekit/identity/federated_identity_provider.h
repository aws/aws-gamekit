// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <string>

// AWS SDK
#include <aws/core/utils/memory/stl/AWSMap.h>

// GameKit
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/logging.h>

namespace GameKit
{
    namespace Identity
    {
        struct LoginUrlResponseInternal
        {
            std::string requestId;
            std::string loginUrl;
        };

        class IFederatedIdentityProvider
        {
        public:
            IFederatedIdentityProvider() {}
            IFederatedIdentityProvider(std::map<std::string, std::string> clientSettings, FuncLogCallback logCb) {};
            virtual ~IFederatedIdentityProvider() {};
            virtual LoginUrlResponseInternal GetLoginUrl() = 0;
            virtual unsigned int PollForCompletion(const std::string& requestId, int timeout, std::string& encryptedLocation) = 0;
            virtual unsigned int RetrieveTokens(const std::string& location, std::string& tokens) = 0;
        };

        template <class T>
        class FederatedIdentityProviderFactory
        {
        public:
            inline static T CreateProvider(std::map<std::string, std::string> clientSettings, FuncLogCallback logCb)
            {
                return T(clientSettings, logCb);
            }

            inline static T CreateProviderWithHttpClient(std::map<std::string, std::string> clientSettings, const std::shared_ptr<Aws::Http::HttpClient> httpClient, FuncLogCallback logCb)
            {
                return T(clientSettings, httpClient, logCb);
            }
        };
    }
}
