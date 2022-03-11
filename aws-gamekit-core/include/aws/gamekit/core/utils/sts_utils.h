// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// AWS SDK
#include <aws/sts/model/GetAccessKeyInfoRequest.h>
#include <aws/sts/model/GetCallerIdentityRequest.h>

// GameKit
#include <aws/gamekit/core/api.h>
#include <aws/gamekit/core/awsclients/api_initializer.h>
#include <aws/gamekit/core/awsclients/default_clients.h>
#include <aws/gamekit/core/logging.h>

namespace GameKit
{
    namespace Utils
    {
        class GAMEKIT_API STSUtils
        {
        private:
            bool m_deleteClients;
            std::shared_ptr<Aws::STS::STSClient> m_stsClient;
            FuncLogCallback m_logCb;

        public:
            STSUtils(const std::string& accessKey, const std::string& secretKey, FuncLogCallback logCallback);
            virtual ~STSUtils();
            void SetSTSClient(std::shared_ptr<Aws::STS::STSClient> stsClient);

            std::string GetAwsAccountId() const;
            bool TryGetAssumeRoleCredentials(const std::string& roleArn, const std::string& roleSessionName, const std::string& sesionPolicy, Aws::STS::Model::Credentials& sessionCredentials) const;
        };
    }
}
