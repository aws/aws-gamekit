// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// AWS SDK
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpClientFactory.h>

// GameKit
#include <aws/gamekit/core/api.h>
namespace GameKit
{
    /**
     * @brief This factory is used as the HttpClient factory for both AWS SDK service clients
     * and GameKit API Gateway clients.
     * This currently only supports creating CurlHttpClient. It will be expanded later to support platform-specicic HTTP clients.
     * Do not override SDKOptions.httpOptions.httpClientFactory_create_fn to use the DefaultHttpClientFactory provided by AWS SDK.
     */
    class GAMEKIT_API GameKitHttpClientFactory : public Aws::Http::HttpClientFactory
    {
    private:
        FuncLogCallback m_logCb = nullptr;
    public:
        GameKitHttpClientFactory(FuncLogCallback log = nullptr);
        virtual ~GameKitHttpClientFactory() {}

        void InitStaticState() override;
        void CleanupStaticState() override;
        std::shared_ptr<Aws::Http::HttpClient> CreateHttpClient(const Aws::Client::ClientConfiguration& clientConfiguration) const override;
        std::shared_ptr<Aws::Http::HttpRequest> CreateHttpRequest(const Aws::String &uri, Aws::Http::HttpMethod method, const Aws::IOStreamFactory &streamFactory) const override;
        std::shared_ptr<Aws::Http::HttpRequest> CreateHttpRequest(const Aws::Http::URI& uri, Aws::Http::HttpMethod method, const Aws::IOStreamFactory& streamFactory) const override;
    };
}
