// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/core/utils/sts_utils.h>

namespace STSModel = Aws::STS::Model;
using STSClient = Aws::STS::STSClient;
using AWSCredentials = Aws::Auth::AWSCredentials;

using namespace GameKit::Utils;
using namespace GameKit::Logger;

#pragma region Constructors/Destructor
STSUtils::STSUtils(const std::string& accessKey, const std::string& secretKey, FuncLogCallback logCallback) :
    m_deleteClients(true), m_logCb(logCallback)
{
    GameKit::AwsApiInitializer::Initialize(m_logCb, this);
    AWSCredentials credentials(ToAwsString(accessKey), ToAwsString(secretKey));

    m_stsClient = std::make_shared<Aws::STS::STSClient>(credentials);
}

void STSUtils::SetSTSClient(std::shared_ptr<STSClient> client)
{
    m_stsClient = client;
}

STSUtils::~STSUtils()
{
    GameKit::AwsApiInitializer::Shutdown(m_logCb, this);
}
#pragma endregion

#pragma region Public Methods
std::string STSUtils::GetAwsAccountId() const
{
    STSModel::GetCallerIdentityRequest request;
    STSModel::GetCallerIdentityOutcome outcome = m_stsClient->GetCallerIdentity(request);

    if (outcome.IsSuccess())
    {
        Aws::STS::Model::GetCallerIdentityResult result = outcome.GetResult();
        Aws::String account = result.GetAccount();

        return ToStdString(account);
    }
    else
    {   
        Logging::Log(m_logCb, Level::Error, outcome.GetError().GetMessage().c_str());
    }

    return "";
}

bool STSUtils::TryGetAssumeRoleCredentials(const std::string& roleArn, const std::string& roleSessionName, const std::string& sesionPolicy,
    Aws::STS::Model::Credentials& sessionCredentials) const
{
    STSModel::AssumeRoleRequest request;
    request.SetRoleArn(ToAwsString(roleArn));
    request.SetRoleSessionName(ToAwsString(roleSessionName));
    request.SetPolicy(ToAwsString(sesionPolicy));

    STSModel::AssumeRoleOutcome outcome = m_stsClient->AssumeRole(request);

    if (outcome.IsSuccess())
    {
        STSModel::AssumeRoleResult result = outcome.GetResult();
        sessionCredentials = result.GetCredentials();
        return true;
    }
    else
    {
        Logging::Log(m_logCb, Level::Error, outcome.GetError().GetMessage().c_str());
    }
    return false;
}
#pragma endregion
