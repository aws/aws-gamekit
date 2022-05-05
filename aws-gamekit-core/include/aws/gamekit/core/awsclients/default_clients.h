// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// AWS SDK
#include <aws/apigateway/APIGatewayClient.h>
#include <aws/apigateway/APIGateway_EXPORTS.h>
#include <aws/cloudformation/CloudFormationClient.h>
#include <aws/cognito-idp/CognitoIdentityProviderClient.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/lambda/LambdaClient.h>
#include <aws/s3/S3Client.h>
#include <aws/secretsmanager/SecretsManagerClient.h>
#include <aws/ssm/SSMClient.h>
#include <aws/sts/STSClient.h>
#include <aws/sts/model/AssumeRoleRequest.h>

// GameKit
#include <aws/gamekit/core/model/account_credentials.h>
#include <aws/gamekit/core/model/account_info.h>

namespace GameKit
{
    namespace ClientSettings
    {
        // These keys can be added by the client on their own instance of awsGameKitClientConfig.yml
        static const std::string SETTINGS_CA_CERT_FILE = "ca_cert_file";
        static const std::string SETTINGS_CA_CERT_PATH = "ca_cert_path";
    }

    class DefaultClients
    {
    private:
        template <class T>
        static inline T* getDefaultClient(const AccountCredentialsCopy& credentials)
        {
            Aws::Client::ClientConfiguration clientConfig;
            Aws::Auth::AWSCredentials creds;

            clientConfig.region = credentials.region.c_str();
            creds.SetAWSAccessKeyId(credentials.accessKey.c_str());
            creds.SetAWSSecretKey(credentials.accessSecret.c_str());

            return new T(creds, clientConfig);
        }

    public:
        static inline Aws::S3::S3Client* GetDefaultS3Client(const AccountCredentialsCopy& credentials)
        {
            return getDefaultClient<Aws::S3::S3Client>(credentials);
        }

        static inline Aws::SSM::SSMClient* GetDefaultSSMClient(const AccountCredentialsCopy& credentials)
        {
            return getDefaultClient<Aws::SSM::SSMClient>(credentials);
        }

        static inline Aws::CloudFormation::CloudFormationClient* GetDefaultCloudFormationClient(const AccountCredentialsCopy& credentials)
        {
            return getDefaultClient<Aws::CloudFormation::CloudFormationClient>(credentials);
        }

        static inline Aws::SecretsManager::SecretsManagerClient* GetDefaultSecretsManagerClient(const AccountCredentialsCopy& credentials)
        {
            return getDefaultClient<Aws::SecretsManager::SecretsManagerClient>(credentials);
        }

        static inline Aws::CognitoIdentityProvider::CognitoIdentityProviderClient* GetDefaultCognitoIdentityProviderClient(const Aws::Client::ClientConfiguration& clientConfig)
        {
            return new Aws::CognitoIdentityProvider::CognitoIdentityProviderClient(Aws::MakeShared<Aws::Auth::AnonymousAWSCredentialsProvider>("anonymous"), clientConfig);
        }

        static inline Aws::APIGateway::APIGatewayClient* GetDefaultApiGatewayClient(const AccountCredentialsCopy& credentials)
        {
            return getDefaultClient<Aws::APIGateway::APIGatewayClient>(credentials);
        }

        static inline Aws::Lambda::LambdaClient* GetDefaultLambdaClient(const AccountCredentialsCopy& credentials)
        {
            return getDefaultClient<Aws::Lambda::LambdaClient>(credentials);
        }

        static inline Aws::Client::ClientConfiguration GetDefaultClientConfigurationWithRegion(const std::map<std::string, std::string>& clientSettings, const std::string& regionKey)
        {
            Aws::Client::ClientConfiguration clientConfig;
            const auto region = clientSettings.find(regionKey);
            if (region != clientSettings.end() && !region->second.empty())
            {
                clientConfig.region = region->second.c_str();
            }
            SetDefaultClientConfiguration(clientSettings, clientConfig);

            return clientConfig;
        }

        static inline void SetDefaultClientConfiguration(const std::map<std::string, std::string>& clientSettings, Aws::Client::ClientConfiguration& clientConfig)
        {
#if ENABLE_CURL_CLIENT
            // Timeout overrides for mobile platforms
            const long defaultMinCurlTimeout = 5000; // 5 seconds
            if (clientConfig.httpRequestTimeoutMs < defaultMinCurlTimeout)
            {
                clientConfig.httpRequestTimeoutMs = defaultMinCurlTimeout;
            }

            if (clientConfig.requestTimeoutMs < defaultMinCurlTimeout)
            {
                clientConfig.requestTimeoutMs = defaultMinCurlTimeout;
            }

            if (clientConfig.connectTimeoutMs < defaultMinCurlTimeout)
            {
                clientConfig.connectTimeoutMs = defaultMinCurlTimeout;
            }

            const auto certPath = clientSettings.find(ClientSettings::SETTINGS_CA_CERT_PATH);
            if (certPath != clientSettings.end() && !certPath->second.empty())
            {
                clientConfig.caPath = certPath->second.c_str();
            }

            const auto certFile = clientSettings.find(ClientSettings::SETTINGS_CA_CERT_FILE);
            if (certFile != clientSettings.end() && !certFile->second.empty())
            {
                clientConfig.caFile = certFile->second.c_str();
            }

            clientConfig.httpLibOverride = Aws::Http::TransferLibType::CURL_CLIENT;
#endif
        }
    };
}
