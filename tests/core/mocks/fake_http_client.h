// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "gmock/gmock.h"

#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/http/standard/StandardHttpRequest.h>
#include <aws/core/http/standard/StandardHttpResponse.h>
#include <aws/core/utils/stream/SimpleStreamBuf.h>
#include <boost/algorithm/string.hpp>

class FakeHttpRequest : public Aws::Http::HttpRequest
{
private:
    Aws::Http::HeaderValueCollection headers;
    std::shared_ptr<Aws::IOStream> bodyStream;

public:
    FakeHttpRequest(const Aws::Http::URI& uri, Aws::Http::HttpMethod method) : HttpRequest(uri, method) 
    {
        headers["host"] = uri.GetAuthority();
    }
    virtual ~FakeHttpRequest() {}
    
    virtual Aws::Http::HeaderValueCollection GetHeaders() const override
    {
        return headers;
    }
    
    virtual const Aws::String& GetHeaderValue(const char* headerName) const override
    {
        return headers.at(headerName);
    }
    
    virtual void SetHeaderValue(const char* headerName, const Aws::String& headerValue) override
    {
        headers[headerName] = headerValue;
    }
    
    virtual void SetHeaderValue(const Aws::String& headerName, const Aws::String& headerValue) override
    {
        headers[headerName] = headerValue;
    }
    
    virtual void DeleteHeader(const char* headerName) override 
    {
        auto iterator = headers.find(headerName);
        
        if (iterator != headers.end())
        {
            headers.erase(iterator);
        }
    }
    
    virtual void AddContentBody(const std::shared_ptr<Aws::IOStream>& strContent) override 
    {
        bodyStream = strContent;
    }
    
    virtual const std::shared_ptr<Aws::IOStream>& GetContentBody() const override
    {
        return bodyStream;
    }
    
    virtual bool HasHeader(const char* name) const override 
    {
        auto iterator = headers.find(name);

        return (iterator != headers.end());
    }
    
    virtual int64_t GetSize() const override { return 0; }
    
    virtual const Aws::IOStreamFactory& GetResponseStreamFactory() const override
    {
        static Aws::IOStreamFactory foo; // might need to be a member variable
        return foo;
    }
    
    virtual void SetResponseStreamFactory(const Aws::IOStreamFactory& factory) override
    {}
};

class FakeHttpResponse : public Aws::Http::HttpResponse
{
private:
    Aws::Http::HeaderValueCollection headers;
    std::string responseBody;
    std::shared_ptr<Aws::IOStream> bodyStream;
    Aws::Utils::Stream::ResponseStream dummyResponseStream;

public:
    FakeHttpResponse(const std::shared_ptr<const Aws::Http::HttpRequest>& originatingRequest) : 
        HttpResponse(originatingRequest), dummyResponseStream([]()->Aws::IOStream* { return nullptr; })
    {}
    
    virtual ~FakeHttpResponse() override {};
    
    std::shared_ptr<Aws::Http::HttpRequest> originalRequest;
    
    virtual Aws::Http::HeaderValueCollection GetHeaders() const override { return headers; }
    
    virtual bool HasHeader(const char* headerName) const override
    {
        return headers.find(headerName) != headers.end();
    }
    
    virtual const Aws::String& GetHeader(const Aws::String& headerName) const override 
    { 
        return headers.at(headerName);
    }
    
    virtual Aws::Utils::Stream::ResponseStream&& SwapResponseStreamOwnership() override 
    { 
        // Return a ResponseStream with a dummy IOStreamFactory.
        return std::move(dummyResponseStream);
    }
    
    virtual void AddHeader(const Aws::String& headerName, const Aws::String& headerValue) override
    {
        headers[headerName] = headerValue;
    }
    
    void SetResponseBody(const std::string& responseBody)
    {
        this->responseBody = responseBody;
        std::stringstream ss(responseBody);
        bodyStream = std::make_shared<Aws::StringStream>(responseBody.c_str());
    }

    virtual Aws::IOStream& GetResponseBody() const override
    {
        return *bodyStream;
    }

    FakeHttpResponse() : Aws::Http::HttpResponse(std::make_shared<FakeHttpRequest>(Aws::Http::URI(), Aws::Http::HttpMethod::HTTP_GET))
    {}
};

class FakeHttpClient : public Aws::Http::HttpClient
{
private:
    std::map<std::shared_ptr<Aws::Http::HttpRequest>, std::shared_ptr<FakeHttpResponse>> requestResponseMap;

public:
    void AddRequestAndResponse(std::shared_ptr<Aws::Http::HttpRequest> request, std::shared_ptr<FakeHttpResponse> response)
    {
        requestResponseMap[request] = response;
    }

    FakeHttpClient() = default;

    std::shared_ptr<Aws::Http::HttpResponse> MakeRequest(const std::shared_ptr<Aws::Http::HttpRequest>& request,
        Aws::Utils::RateLimits::RateLimiterInterface* readLimiter = nullptr,
        Aws::Utils::RateLimits::RateLimiterInterface* writeLimiter = nullptr)
        const override
    {
        return requestResponseMap.at(request);
    }

    void DisableRequestProcessing() {}
    void EnableRequestProcessing() {}
    bool IsRequestProcessingEnabled() const { return true; }
    void RetryRequestSleep(std::chrono::milliseconds sleepTime) {}
    bool ContinueRequest(const Aws::Http::HttpRequest&) const { return true; }
};

class MockHttpClient : public Aws::Http::HttpClient
{
private:
    FakeHttpClient fake;

public:
    void DelegateToFake()
    {
        ON_CALL(*this, MakeRequest).WillByDefault([this](const std::shared_ptr<Aws::Http::HttpRequest>& r, Aws::Utils::RateLimits::RateLimiterInterface* rl, Aws::Utils::RateLimits::RateLimiterInterface* wl) 
            {
                return fake.MakeRequest(r, rl, wl);
            });
    }

    MockHttpClient() {}
    virtual ~MockHttpClient() override {}

    MOCK_METHOD(std::shared_ptr<Aws::Http::HttpResponse>, MakeRequest, (const std::shared_ptr<Aws::Http::HttpRequest>&, Aws::Utils::RateLimits::RateLimiterInterface*, Aws::Utils::RateLimits::RateLimiterInterface*), (const override));
    MOCK_METHOD(void, DisableRequestProcessing, ());
    MOCK_METHOD(void, EnableRequestProcessing, ());
    MOCK_METHOD(bool, IsRequestProcessingEnabled, (), (const));
    MOCK_METHOD(void, RetryRequestSleep, (std::chrono::milliseconds));
    MOCK_METHOD(bool, ContinueRequest, (const Aws::Http::HttpRequest&), (const));
};

class MockHttpClientFactory : public Aws::Http::HttpClientFactory
{
private:
    std::shared_ptr<MockHttpClient> mockClient;

public:
    virtual std::shared_ptr<Aws::Http::HttpClient> CreateHttpClient(const Aws::Client::ClientConfiguration& clientConfiguration) const override
    {
        return mockClient;
    }

    virtual std::shared_ptr<Aws::Http::HttpRequest> CreateHttpRequest(const Aws::String& uri, Aws::Http::HttpMethod method, const Aws::IOStreamFactory& streamFactory) const override
    {
        auto request = std::make_shared<Aws::Http::Standard::StandardHttpRequest>(uri, method);
        request->SetResponseStreamFactory(streamFactory);

        return request;
    }

    virtual std::shared_ptr<Aws::Http::HttpRequest> CreateHttpRequest(const Aws::Http::URI& uri, Aws::Http::HttpMethod method, const Aws::IOStreamFactory& streamFactory) const override
    {
        auto request = std::make_shared<Aws::Http::Standard::StandardHttpRequest>(uri, method);
        request->SetResponseStreamFactory(streamFactory);

        return request;
    }

    inline std::shared_ptr<MockHttpClient> GetClient() const
    { 
        return mockClient; 
    }

    inline void SetClient(const std::shared_ptr<MockHttpClient>& client) 
    { 
        mockClient = client; 
    }
};
