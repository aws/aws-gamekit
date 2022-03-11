// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// GameKit
#include <aws/gamekit/core/awsclients/api_initializer.h>

// Aws
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/config/AWSProfileConfigLoader.h>
#include <aws/core/platform/FileSystem.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/utils/logging/AWSLogging.h>
#if __ANDROID__
#include <android/log.h>
#include <jni.h>
#include <aws/core/platform/Android.h>
#include <aws/core/utils/logging/android/LogcatLogSystem.h>
#endif

using namespace GameKit::Logger;

#pragma region Static Members
std::mutex GameKit::AwsApiInitializer::m_mutex;
bool GameKit::AwsApiInitializer::m_isAwsSdkInitialized;
std::unique_ptr<Aws::SDKOptions> GameKit::AwsApiInitializer::m_awsSdkOptions;
int GameKit::AwsApiInitializer::m_count = 0;
#pragma endregion

#pragma region Public Methods
void GameKit::AwsApiInitializer::Initialize(FuncLogCallback log, const void* caller)
{
    std::lock_guard<std::mutex> lock(AwsApiInitializer::m_mutex);

    std::string message;

    if (m_count <= 0)
    {
        m_count = 0;
        message = "AwsApiInitializer::Initialize(): Initializing (count: " + std::to_string(m_count) + ")";
        m_awsSdkOptions.reset(new Aws::SDKOptions());

#if ENABLE_CUSTOM_HTTP_CLIENT_FACTORY
        message += "; Using custom HttpClientFactory: GameKitHttpClientFactory";
        m_awsSdkOptions->httpOptions.httpClientFactory_create_fn = [=](){
            return Aws::MakeShared<GameKit::GameKitHttpClientFactory>(GAMEKIT_HTTP_CLIENT_FACTORY_ALLOCATION_TAG, log);
        };
#endif

#if ENABLE_CURL_CLIENT
        // Disable EC2 metadata lookup
        putenv("AWS_EC2_METADATA_DISABLED=true");
        m_awsSdkOptions->httpOptions.installSigPipeHandler = true;
#endif
#if defined(__ANDROID__) && (defined(_DEBUG) || defined(GAMEKIT_DEBUG))
        // Pipe AWS logging to Android's Logcat in Debug builds
        Aws::Utils::Logging::InitializeAWSLogging(Aws::MakeShared<Aws::Utils::Logging::LogcatLogSystem>(AWS_LOGGING_ALLOCATION_TAG, Aws::Utils::Logging::LogLevel::Debug));
#else
        m_awsSdkOptions->loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Debug;
#endif

        message += "; initAndCleanupCurl: " + (m_awsSdkOptions->httpOptions.initAndCleanupCurl? std::string("true") : std::string("false"));
        message += "; installSigPipeHandler: " + (m_awsSdkOptions->httpOptions.installSigPipeHandler? std::string("true") : std::string("false"));
        message += "; initAndCleanupOpenSSL: " + (m_awsSdkOptions->cryptoOptions.initAndCleanupOpenSSL? std::string("true") : std::string("false"));
        Aws::InitAPI(*m_awsSdkOptions);

        m_isAwsSdkInitialized = true;
    }
    else
    {
        message = "AwsApiInitializer::Initialize(): Already initialized (count: " + std::to_string(m_count) + ")";
    }
    m_count++;

    Logging::Log(log, Level::Info, message.c_str(), caller);
}

void GameKit::AwsApiInitializer::Shutdown(FuncLogCallback log, const void* caller)
{
    std::lock_guard<std::mutex> lock(AwsApiInitializer::m_mutex);
    std::string message;

    if (m_count <= 0)
    {
        message = "AwsApiInitializer::Shutdown(): Already shut down (count: " + std::to_string(m_count) + ")";
        m_count = 0;
    }
    else if (m_count == 1)
    {
        message = "AwsApiInitializer::Shutdown(): Shutting down (count: " + std::to_string(m_count) + ")";
        Aws::ShutdownAPI(*m_awsSdkOptions);
        m_awsSdkOptions.reset();
        m_isAwsSdkInitialized = false;
    }
    else
    {
        message = "AwsApiInitializer::Shutdown(): Not shutting down (count: " + std::to_string(m_count) + ")";
    }
    m_count--;

    Logging::Log(log, Level::Verbose, message.c_str(), caller);
}

bool GameKit::AwsApiInitializer::IsInitialized()
{
    std::lock_guard<std::mutex> lock(AwsApiInitializer::m_mutex);

    return m_isAwsSdkInitialized;
}
#pragma endregion
