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
#else
#include <aws/core/utils/logging/DefaultLogSystem.h>
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
        // Disable EC2 metadata lookup
        putenv("AWS_EC2_METADATA_DISABLED=true");

        m_count = 0;
        message = "AwsApiInitializer::Initialize(): Initializing (count: " + std::to_string(m_count) + ")";
        m_awsSdkOptions.reset(Aws::New<Aws::SDKOptions>(GAMEKIT_SDK_OPTIONS_ALLOCATION_TAG));

#if ENABLE_CUSTOM_HTTP_CLIENT_FACTORY
        message += "; Using custom HttpClientFactory: GameKitHttpClientFactory";
        m_awsSdkOptions->httpOptions.httpClientFactory_create_fn = [=](){
            return Aws::MakeShared<GameKit::GameKitHttpClientFactory>(GAMEKIT_HTTP_CLIENT_FACTORY_ALLOCATION_TAG, log);
        };
#endif

#if ENABLE_CURL_CLIENT

        m_awsSdkOptions->httpOptions.installSigPipeHandler = true;
        m_awsSdkOptions->cryptoOptions.initAndCleanupOpenSSL = true;
        Aws::Http::SetInitCleanupCurlFlag(m_awsSdkOptions->httpOptions.initAndCleanupCurl);
        Aws::Http::SetInstallSigPipeHandlerFlag(m_awsSdkOptions->httpOptions.installSigPipeHandler);
        Aws::Utils::Crypto::SetInitCleanupOpenSSLFlag(m_awsSdkOptions->cryptoOptions.initAndCleanupOpenSSL);
        message += "; initAndCleanupCurl: " + (m_awsSdkOptions->httpOptions.initAndCleanupCurl? std::string("true") : std::string("false"));
        message += "; installSigPipeHandler: " + (m_awsSdkOptions->httpOptions.installSigPipeHandler? std::string("true") : std::string("false"));
        message += "; initAndCleanupOpenSSL: " + (m_awsSdkOptions->cryptoOptions.initAndCleanupOpenSSL? std::string("true") : std::string("false"));
#endif
#if (defined(_DEBUG) || defined(GAMEKIT_DEBUG))
#if defined(__ANDROID__)
        // Pipe AWS logging to Android's Logcat in Debug builds
        Aws::Utils::Logging::InitializeAWSLogging(Aws::MakeShared<Aws::Utils::Logging::LogcatLogSystem>(AWS_LOGGING_ALLOCATION_TAG, Aws::Utils::Logging::LogLevel::Debug));
#else // defined(__ANDROID__)
        Aws::Utils::Logging::InitializeAWSLogging(Aws::MakeShared<Aws::Utils::Logging::DefaultLogSystem>(AWS_LOGGING_ALLOCATION_TAG, Aws::Utils::Logging::LogLevel::Debug, "aws_gamekit_"));
#endif
#else // (defined(_DEBUG) || defined(GAMEKIT_DEBUG))
#if defined(__ANDROID__)
        // Pipe AWS logging to Android's Logcat
        Aws::Utils::Logging::InitializeAWSLogging(Aws::MakeShared<Aws::Utils::Logging::LogcatLogSystem>(AWS_LOGGING_ALLOCATION_TAG, Aws::Utils::Logging::LogLevel::Fatal));
#else // defined(__ANDROID__)
        Aws::Utils::Logging::InitializeAWSLogging(Aws::MakeShared<Aws::Utils::Logging::DefaultLogSystem>(AWS_LOGGING_ALLOCATION_TAG, Aws::Utils::Logging::LogLevel::Fatal, "aws_gamekit_"));
#endif // defined(__ANDROID__)
#endif // (defined(_DEBUG) || defined(GAMEKIT_DEBUG))

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

void GameKit::AwsApiInitializer::Shutdown(FuncLogCallback log, const void* caller, bool force)
{
    std::lock_guard<std::mutex> lock(AwsApiInitializer::m_mutex);
    std::string message;

    if (m_count == 1 || (m_count > 1 && force))
    {
        message = "AwsApiInitializer::Shutdown(): Shutting down (count: " + std::to_string(m_count) + ", force: " + std::to_string(force) + ")";
        Aws::ShutdownAPI(*m_awsSdkOptions);

        m_awsSdkOptions = nullptr;
        m_isAwsSdkInitialized = false;
        m_count = 0;
    }
    else if (m_count <= 0)
    {
        message = "AwsApiInitializer::Shutdown(): Already shut down (count: " + std::to_string(m_count) + ")";
        m_count = 0;
    }
    else
    {
        message = "AwsApiInitializer::Shutdown(): Not shutting down (count: " + std::to_string(m_count) + ")";
        m_count--;
    }

    Logging::Log(log, Level::Info, message.c_str(), caller);
}

bool GameKit::AwsApiInitializer::IsInitialized()
{
    std::lock_guard<std::mutex> lock(AwsApiInitializer::m_mutex);

    return m_isAwsSdkInitialized;
}
#pragma endregion
