// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// AWS SDK
#if ENABLE_CURL_CLIENT
#include <aws/core/http/curl/CurlHttpClient.h>
#include <aws/core/utils/logging/LogMacros.h>
#endif
#include <aws/core/http/standard/StandardHttpRequest.h>

// GameKit
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/core/awsclients/http_client_factory.h>

using namespace GameKit;
using namespace GameKit::Logger;

GameKitHttpClientFactory::GameKitHttpClientFactory(FuncLogCallback log)
    :m_logCb(log)
{
    Logging::Log(m_logCb, Level::Info, "Using GameKit::GameKitHttpClientFactory as the HttpClientFactory");
}

std::shared_ptr<Aws::Http::HttpClient> GameKitHttpClientFactory::CreateHttpClient(const Aws::Client::ClientConfiguration& clientConfig) const
{
#if ENABLE_CURL_CLIENT
    Logging::Log(m_logCb, Level::Info, std::string("GameKitHttpClientFactory::CreateHttpClient(): Using Aws::Http::CurlHttpClient; clientConfig.httpLibOverride=" + std::to_string(static_cast<int>(clientConfig.httpLibOverride))).c_str());
    return Aws::MakeShared<Aws::Http::CurlHttpClient>(GAMEKIT_HTTP_CLIENT_FACTORY_ALLOCATION_TAG, clientConfig);
#else
    Logging::Log(m_logCb, Level::Error, "GameKitHttpClientFactory::CreateHttpClient(): This currently only supports creating a CurlHttpClient. Enable it by setting ENABLE_CURL_CLIENT=1.");
    return nullptr;
#endif
}

void GameKitHttpClientFactory::InitStaticState()
{
#if ENABLE_CURL_CLIENT
    Logging::Log(m_logCb, Level::Info, "GameKitHttpClientFactory::InitStaticState() : CurlHttpClient::InitGlobalState()");
    Aws::Http::CurlHttpClient::InitGlobalState();

    // GameKitHttpClientFactory will always install the Sigpipe handler for the curl client regardless of what's set in httpOptions.installSigPipeHandler
    // Copied from DefaultHttpClientFactory's LogAndSwallowHandler
    ::signal(SIGPIPE, [](int signal)
    {
        switch(signal)
        {
            case SIGPIPE:
                AWS_LOGSTREAM_ERROR(GAMEKIT_HTTP_CLIENT_FACTORY_ALLOCATION_TAG, "Received a SIGPIPE error");
                break;
            default:
                AWS_LOGSTREAM_ERROR(GAMEKIT_HTTP_CLIENT_FACTORY_ALLOCATION_TAG, "Unhandled system SIGNAL error"  << signal);
        }
    });
#endif
}

void GameKitHttpClientFactory::CleanupStaticState()
{
#if ENABLE_CURL_CLIENT
    Logging::Log(m_logCb, Level::Info, "GameKitHttpClientFactory::CleanupStaticState() : CurlHttpClient::CleanupGlobalState()");
    Aws::Http::CurlHttpClient::CleanupGlobalState();
#endif
}

std::shared_ptr<Aws::Http::HttpRequest> GameKitHttpClientFactory::CreateHttpRequest(const Aws::String &uri, Aws::Http::HttpMethod method, const Aws::IOStreamFactory &streamFactory) const
{
    return Aws::Http::CreateHttpRequest(Aws::Http::URI(uri), method, streamFactory);
}

std::shared_ptr<Aws::Http::HttpRequest> GameKitHttpClientFactory::CreateHttpRequest(const Aws::Http::URI& uri, Aws::Http::HttpMethod method, const Aws::IOStreamFactory& streamFactory) const
{
    std::shared_ptr<Aws::Http::HttpRequest> request = Aws::MakeShared<Aws::Http::Standard::StandardHttpRequest>(GAMEKIT_HTTP_CLIENT_FACTORY_ALLOCATION_TAG, uri, method);
    request->SetResponseStreamFactory(streamFactory);

    return request;
}
