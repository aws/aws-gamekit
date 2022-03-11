// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <string>

// AWS SDK
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/utils/UUID.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

// GameKit
#include <aws/gamekit/authentication/gamekit_session_manager.h>
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/awsclients/api_initializer.h>
#include <aws/gamekit/identity/federated_identity_provider.h>

// Boost
#include <boost/algorithm/string/case_conv.hpp>

namespace GameKit
{
    namespace Identity
    {
        class FacebookIdentityProvider : public IFederatedIdentityProvider
        {
        private:
            FuncLogCallback m_logCb;
            std::map<std::string, std::string> m_clientSettings;
            std::shared_ptr<Aws::Http::HttpClient> m_httpClient;

            std::shared_ptr<Aws::Http::HttpResponse> makeRequest(const std::string& path, Aws::Http::HttpMethod method, const std::string& payload);
        public:
            FacebookIdentityProvider() : m_logCb(nullptr) {}
            FacebookIdentityProvider(std::map<std::string, std::string>& clientSettings, FuncLogCallback logCb);
            FacebookIdentityProvider(std::map<std::string, std::string>& clientSettings, const std::shared_ptr<Aws::Http::HttpClient> httpClient, FuncLogCallback logCb);
            ~FacebookIdentityProvider();
            LoginUrlResponseInternal GetLoginUrl() override;
            unsigned int PollForCompletion(const std::string& requestId, int timeout, std::string& encryptedLocation) override;
            unsigned int RetrieveTokens(const std::string& location, std::string& tokens) override;
        };
    }
}
