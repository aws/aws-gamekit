// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// AWS SDK
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/utils/StringUtils.h>

// GameKit
#include <aws/gamekit/achievements/gamekit_admin_achievements.h>
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>
#include <aws/gamekit/core/utils/file_utils.h>

// Boost
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem.hpp>

using namespace Aws::Utils;
using namespace GameKit::Achievements;

#pragma region Constructors/Destructor
AdminAchievements::AdminAchievements(FuncLogCallback logCb, Authentication::GameKitSessionManager* sessionManager, const std::string& cloudResourcesPath, const AccountInfo& accountInfo, const AccountCredentials& accountCredentials) :
    m_sessionManager(sessionManager),
    m_cloudResourcesPath(cloudResourcesPath),
    m_stsUtils(accountCredentials.accessKey, accountCredentials.accessSecret, logCb)
{
    m_logCb = logCb;

    const std::string shortRegionCode = getShortRegionCode(std::string(accountCredentials.region));
    if (shortRegionCode.empty())
    {
        std::string msg = "Could not retrieve short region code for: " + std::string(accountCredentials.region) + " which will forbid you from signing admin requests.";
        Logging::Log(m_logCb, Level::Error, msg.c_str());
    }

    m_accountInfo = CreateAccountInfoCopy(accountInfo);
    m_accountCredentials = CreateAccountCredentialsCopy(accountCredentials, shortRegionCode);

    GameKit::AwsApiInitializer::Initialize(m_logCb, this);

    static const long TIMEOUT = 5000;

    Aws::Client::ClientConfiguration clientConfig;
    GameKit::DefaultClients::SetDefaultClientConfiguration(m_sessionManager->GetClientSettings(), clientConfig);
    clientConfig.region = m_sessionManager->GetClientSettings()[GameKit::ClientSettings::Authentication::SETTINGS_IDENTITY_REGION].c_str();
    clientConfig.connectTimeoutMs = TIMEOUT;
    clientConfig.httpRequestTimeoutMs = TIMEOUT;
    clientConfig.requestTimeoutMs = TIMEOUT;
    m_httpClient = Aws::Http::CreateHttpClient(clientConfig);

    Logging::Log(m_logCb, Level::Info, "Achievements instantiated");
}

AdminAchievements::~AdminAchievements()
{
    GameKit::AwsApiInitializer::Shutdown(m_logCb, this);
}
#pragma endregion


#pragma region Public Methods
unsigned int AdminAchievements::ListAchievements(unsigned int pageSize, bool waitForAllPages, const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback)
{
    std::string startKey = "";
    std::string pagingToken = "";
    unsigned int status = GameKit::GAMEKIT_SUCCESS;

    do
    {
        std::map<std::string, std::string> queryStringParameters;
        if (startKey != "")
        {
            queryStringParameters.insert({"start_key", startKey});
            queryStringParameters.insert({"paging_token", pagingToken});
            startKey = "";
            pagingToken = "";
        }
        queryStringParameters.insert({"limit", std::to_string(pageSize)});
        queryStringParameters.insert({"wait_for_all_pages", std::to_string(waitForAllPages)});

        std::shared_ptr<Aws::Http::HttpResponse> response = std::shared_ptr<Aws::Http::HttpResponse>();
        status = makeAdminRequest(Aws::Http::HttpMethod::HTTP_GET, response, queryStringParameters);
        if (status != GAMEKIT_SUCCESS)
        {
            return status;
        }

        Aws::Utils::Json::JsonValue value;
        status = processResponse(response, "Achievements::ListAchievementsForGame()", dispatchReceiver, responseCallback, value);
        if (status != GameKit::GAMEKIT_SUCCESS)
        {
            return status;
        }

        const Aws::Utils::Json::JsonView view = value.View();
        if (view.KeyExists("paging"))
        {
            const Aws::Utils::Json::JsonView pagingObj = view.GetObject("paging");
            if (pagingObj.KeyExists("next_start_key"))
            {
                startKey = ToStdString(pagingObj.GetObject("next_start_key").WriteCompact());
                if (!pagingObj.KeyExists("paging_token"))
                {
                    Logging::Log(m_logCb, Level::Error, "paging_token missing from response with next_start_key");
                    pagingToken = "";
                }
                else
                {
                    pagingToken = ToStdString(pagingObj.GetString("paging_token"));
                }
            }
        }
    } while (startKey != "");

    return status;
}

unsigned int AdminAchievements::AddAchievements(const GameKit::Achievement* achievements, unsigned int batchSize)
{
    if (batchSize == 0)
    {
        return GameKit::GAMEKIT_SUCCESS;
    }

    // A vector of pairs. Each pair will contain the new locked and unlocked icons
    std::vector<std::pair<std::string, std::string>> updatedIcons;

    // Upload icons to S3, the vector updatedIcons will have the updated locations
    unsigned int uploadResult = uploadIcons(achievements, batchSize, updatedIcons);

    if (uploadResult != GAMEKIT_SUCCESS)
    {
        return uploadResult;
    }

    // Save to Database
    return persistAchievementsData(achievements, updatedIcons, batchSize);
}

unsigned int AdminAchievements::DeleteAchievements(const char* const* achievementIdentifiers, unsigned int batchSize)
{
    if (batchSize == 0)
    {
        return GameKit::GAMEKIT_SUCCESS;
    }

    Aws::Utils::Array<Aws::String> arrayBody(batchSize);
    for (unsigned int i = 0; i < batchSize; i++)
    {
        arrayBody[i] = achievementIdentifiers[i];
    }

    Aws::Utils::Json::JsonValue body;
    body.WithArray("achievement_ids", arrayBody);
    Aws::String body_string = body.View().WriteCompact();

    std::shared_ptr<Aws::Http::HttpResponse> response = std::shared_ptr<Aws::Http::HttpResponse>();
    unsigned int status = makeAdminRequest(Aws::Http::HttpMethod::HTTP_DELETE, response, std::map<std::string, std::string>(), body_string);
    if (status != GAMEKIT_SUCCESS)
    {
        return status;
    }

    Aws::Utils::Json::JsonValue outJson;
    return processResponse(response, "Achievements::DeleteAchievementsForGame()", nullptr, nullptr, outJson);
}

unsigned int GameKit::Achievements::AdminAchievements::ChangeCredentials(const AccountCredentials& accountCredentials, const AccountInfo& accountInfo)
{
    const std::string shortRegionCode = getShortRegionCode(std::string(accountCredentials.region));
    if (shortRegionCode.empty())
    {
        std::string msg = "Could not retrieve short region code for: " + std::string(accountCredentials.region) + " which will forbid you from signing admin requests.";
        Logging::Log(m_logCb, Level::Error, msg.c_str());
        return GAMEKIT_ERROR_REGION_CODE_CONVERSION_FAILED;
    }

    m_accountInfo = CreateAccountInfoCopy(accountInfo);
    m_accountCredentials = CreateAccountCredentialsCopy(accountCredentials, shortRegionCode);
    m_stsUtils = GameKit::Utils::STSUtils(accountCredentials.accessKey, accountCredentials.accessSecret, m_logCb);
    return GAMEKIT_SUCCESS;
}
#pragma endregion

#pragma region Private Methods
unsigned int AdminAchievements::processResponse(const std::shared_ptr<Aws::Http::HttpResponse>& response, const std::string& originMethod,
    const DISPATCH_RECEIVER_HANDLE dispatchReceiver, const CharPtrCallback responseCallback, Aws::Utils::Json::JsonValue& jsonBody) const
{
    if (response->GetResponseCode() == Aws::Http::HttpResponseCode::NO_CONTENT)
    {
        return GAMEKIT_SUCCESS;
    }
    if (response->GetResponseCode() != Aws::Http::HttpResponseCode::OK)
    {
        const std::string errorMessage = "Error: " + originMethod + " returned with http response code : " + std::to_string(static_cast<int>(response->GetResponseCode()));
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_HTTP_REQUEST_FAILED;
    }

    Aws::IOStream& body = response->GetResponseBody();
    jsonBody = Aws::Utils::Json::JsonValue(body);

    if (!jsonBody.WasParseSuccessful())
    {
        const std::string errorMessage = "Error: " + originMethod + " response formatted incorrectly : " + ToStdString(jsonBody.GetErrorMessage());
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_PARSE_JSON_FAILED;
    }

    if (!(dispatchReceiver == nullptr) && !(responseCallback == nullptr))
    {
        const Aws::String output = jsonBody.View().WriteCompact();
        responseCallback(dispatchReceiver, output.c_str());
    }

    return GAMEKIT_SUCCESS;
}

bool AdminAchievements::signRequestWithSessionCredentials(const std::shared_ptr<Aws::Http::HttpRequest> request)
{
    std::shared_ptr<Aws::Auth::AWSCredentialsProvider> credProvider = Aws::MakeShared<Aws::Auth::SimpleAWSCredentialsProvider>("AwsGameKit", m_adminApiSessionCredentials.GetAccessKeyId(), m_adminApiSessionCredentials.GetSecretAccessKey(), m_adminApiSessionCredentials.GetSessionToken());
    Aws::Client::AWSAuthV4Signer signer(credProvider, "execute-api", ToAwsString(m_accountCredentials.region), Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Always, false);
    return signer.SignRequest(*request);
}

unsigned AdminAchievements::persistAchievementsData(const Achievement* achievements, const std::vector<std::pair<std::string, std::string>>& updatedIcons, size_t batchSize)
{
    // formulate request body content
    Aws::Utils::Array<Aws::Utils::Json::JsonValue> arrayBody(batchSize);
    for (unsigned int i = 0; i < batchSize; i++)
    {
        Achievement achievement = achievements[i];
        achievement.lockedIcon = updatedIcons[i].first.c_str();
        achievement.unlockedIcon = updatedIcons[i].second.c_str();
        arrayBody[i] = achievement.ToJson();
    }
    Aws::Utils::Json::JsonValue body;
    body.WithArray("achievements", arrayBody);
    Aws::String body_string = body.View().WriteCompact();

    std::shared_ptr<Aws::Http::HttpResponse> response = std::shared_ptr<Aws::Http::HttpResponse>();
    unsigned int status = makeAdminRequest(Aws::Http::HttpMethod::HTTP_POST, response, std::map<std::string, std::string>(), body_string);
    if (status != GAMEKIT_SUCCESS)
    {
        return status;
    }
    Aws::Utils::Json::JsonValue outJson;
    return processResponse(response, "Achievements::AddAchievementsForGame()", nullptr, nullptr, outJson);
}

std::string AdminAchievements::getAdminSessionPolicy() const
{
    return "{\"Version\":\"2012-10-17\",\"Statement\":[{\"Sid\":\"Stmt1\",\"Effect\":\"Allow\",\"Action\":\"execute-api:Invoke\",\"Resource\": \"arn:aws:execute-api:*:*:*/*/*/achievements/admin\" }]}";
}

std::string AdminAchievements::getAdminApiRoleArn() const
{
    GameKit::ResourceEnvironment env = m_accountInfo.environment;
    std::string roleName = "gamekit_";
    roleName.append(env.GetEnvironmentString())
        .append("_")
        .append(m_accountCredentials.shortRegionCode)
        .append("_")
        .append(m_accountInfo.gameName)
        .append("_AchievementsAdminInvokeRole");

    std::string adminApiRoleArn = "arn:aws:iam::";
    adminApiRoleArn.append(m_accountInfo.accountId)
        .append(":role/")
        .append(roleName);

    return adminApiRoleArn;
}

unsigned int AdminAchievements::getAdminApiSessionCredentials(bool forceCredentialsRefresh)
{
    // to make this function thread safe, lock it behind a mutex
    std::lock_guard<std::mutex> guard(m_adminCredentialsMutex);

    // Update credentials in memory if not available or if they expire in the next ADMIN_SESSION_EXPIRATION_BUFFER_MILLIS milliseconds
    if (forceCredentialsRefresh || !m_adminApiSessionCredentials.ExpirationHasBeenSet()
        || m_adminApiSessionCredentials.GetExpiration().Millis() < Aws::Utils::DateTime::CurrentTimeMillis() + ADMIN_SESSION_EXPIRATION_BUFFER_MILLIS)
    {
        std::string sessionPolicy = getAdminSessionPolicy();
        std::string adminApiRoleArn = getAdminApiRoleArn();
        std::string adminApiRoleName = "AchievementsAdminSession_" + std::string(m_accountCredentials.accessKey) + "_" + m_accountInfo.accountId;

        if (!m_stsUtils.TryGetAssumeRoleCredentials(adminApiRoleArn, adminApiRoleName, sessionPolicy, m_adminApiSessionCredentials))
        {
            return GAMEKIT_ERROR_SIGN_REQUEST_FAILED;
        }
    }
    return GAMEKIT_SUCCESS;
}

std::string AdminAchievements::getAchievementsBucketName() const
{
    std::string achievementsBucketName = "gamekit-";

    GameKit::ResourceEnvironment env = m_accountInfo.environment;
    achievementsBucketName.append(env.GetEnvironmentString())
        .append("-")
        .append(std::string(m_accountCredentials.shortRegionCode))
        .append("-")
        .append(GameKit::Utils::EncodingUtils::DecimalToBase(m_accountInfo.accountId, GameKit::Utils::BASE_36))
        .append("-")
        .append(m_accountInfo.gameName)
        .append("-")
        .append(GameKit::GetFeatureTypeString(FeatureType::Achievements));

    return achievementsBucketName;
}

std::string AdminAchievements::generateIconObjectKeySuffix(const std::string& achievementId, const std::string& iconType, const std::string& fileExtension) const
{
    const std::string uuid = ToStdString(Aws::String(Aws::Utils::UUID::RandomUUID()));
    return std::string(achievementId)
        .append("_")
        .append(iconType)
        .append("_")
        .append(boost::algorithm::to_lower_copy(uuid))
        .append(boost::algorithm::to_lower_copy(fileExtension));
}

Aws::S3::Model::PutObjectOutcome AdminAchievements::uploadToS3(const Aws::S3::S3Client* s3Client,
    const std::string& objectKeySuffix,
    const boost::filesystem::path& filePath) const
{
    // Upload the icon to the staging bucket, where it will automatically be resized
    const std::string objectKey = std::string(ACHIEVEMENT_ICONS_UPLOAD_OBJECT_PATH).append(objectKeySuffix);

    Aws::S3::Model::PutObjectRequest putObjRequest;
    putObjRequest.SetExpectedBucketOwner(ToAwsString(m_accountCredentials.accountId));
    putObjRequest.SetBucket(ToAwsString(getAchievementsBucketName()));
    putObjRequest.SetKey(ToAwsString(objectKey));

    std::shared_ptr<Aws::IOStream> inputData = Aws::MakeShared<Aws::FStream>(
        objectKey.c_str(),
        filePath.native(),
        std::ios_base::in | std::ios_base::binary);
    putObjRequest.SetBody(inputData);

    return s3Client->PutObject(putObjRequest);
}

unsigned int AdminAchievements::uploadIcon(const Aws::S3::S3Client* s3Client,
    const Achievement& achievementCopy,
    const char* iconType,
    const std::string& iconSource,
    std::string& outObjectKey) const
{
    if (iconSource.empty())
    {
        outObjectKey = "";
        return GameKit::GAMEKIT_SUCCESS;
    }

    if (boost::filesystem::exists(iconSource))
    {
        const boost::filesystem::path sourcePath = boost::filesystem::path(iconSource);
        const std::string fileExtension = sourcePath.extension().string();

        // Generate a unique identifier for the icon, including a UUID
        const std::string objectKeySuffix = generateIconObjectKeySuffix(achievementCopy.achievementId, iconType, fileExtension);

        const Aws::S3::Model::PutObjectOutcome outcome = uploadToS3(s3Client, objectKeySuffix, sourcePath);
        if (!outcome.IsSuccess())
        {
            return GameKit::GAMEKIT_ERROR_ACHIEVEMENTS_ICON_UPLOAD_FAILED;
        }

        // Provide a link to the resized achievement icon
        outObjectKey = std::string(ACHIEVEMENT_ICONS_RESIZED_OBJECT_PATH)
            .append(objectKeySuffix);
    }
    else
    {
        // This is a cloudfront suffix path, leave as is.
        outObjectKey = iconSource;
    }

    return GameKit::GAMEKIT_SUCCESS;
}

unsigned int AdminAchievements::uploadIcons(const Achievement* achievements, unsigned batchSize, std::vector<std::pair<std::string, std::string>>& updatedIcons) const
{
    const Aws::S3::S3Client* s3Client = GameKit::DefaultClients::GetDefaultS3Client(m_accountCredentials);

    // updated keys
    std::string newLockedKey;
    std::string newUnlockedKey;

    for (unsigned int i = 0; i < batchSize; i++)
    {
        Achievement achievement = achievements[i];

        unsigned int uploadResult = uploadIcon(s3Client, achievement, "locked", achievement.lockedIcon, newLockedKey);
        if (uploadResult != GameKit::GAMEKIT_SUCCESS)
        {
            std::string errorMsg = "Achievements::AddAchievementsForGame() Failed to upload locked icon for " + std::string(achievement.achievementId);
            Logging::Log(m_logCb, Level::Error, errorMsg.c_str());
            return uploadResult;
        }

        uploadResult = uploadIcon(s3Client, achievement, "unlocked", achievement.unlockedIcon, newUnlockedKey);
        if (uploadResult != GameKit::GAMEKIT_SUCCESS)
        {
            std::string errorMsg = "Achievements::AddAchievementsForGame() Failed to upload unlocked icon for " + std::string(achievement.achievementId);
            Logging::Log(m_logCb, Level::Error, errorMsg.c_str());
            return uploadResult;
        }

        // Set the updated icon locations as a pair of {newLockedKey, newUnlockedKey}
        updatedIcons.push_back({newLockedKey, newUnlockedKey});
    }

    return GameKit::GAMEKIT_SUCCESS;
}

std::string AdminAchievements::getShortRegionCode(const std::string& region) const
{
    if (m_cloudResourcesPath.empty() || region.empty())
    {
        return "";
    }
    AwsRegionMappings& regionMappings = AwsRegionMappings::getInstance(m_cloudResourcesPath, m_logCb);
    return regionMappings.getFiveLetterRegionCode(region);
}

unsigned int AdminAchievements::makeAdminRequest(const Aws::Http::HttpMethod method, std::shared_ptr<Aws::Http::HttpResponse>& response, const std::map<std::string, std::string>& queryStringParams, const Aws::String& body)
{
    const std::string uri = m_sessionManager->GetClientSettings()[GameKit::ClientSettings::Achievements::SETTINGS_ACHIEVEMENTS_API_GATEWAY_BASE_URL] + "/admin";

    Aws::String startKey = "";
    Aws::String pagingToken = "";
    unsigned int status = GameKit::GAMEKIT_SUCCESS;

    auto assembleAndExecuteRequest = [&](bool forceCredentialRefresh=false) mutable
    {
        std::shared_ptr<Aws::Http::HttpRequest> request = Aws::Http::CreateHttpRequest(ToAwsString(uri), method, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

        for (std::pair<std::string, std::string> pair : queryStringParams)
        {
            request->AddQueryStringParameter(pair.first.c_str(), ToAwsString(pair.second));
        }

        if (!body.empty())
        {
            std::shared_ptr<Aws::IOStream> bodyStream = Aws::MakeShared<Aws::StringStream>("RequestBody", std::ios_base::in | std::ios_base::out);
            bodyStream->write(body.c_str(), body.length());

            request->SetContentType("application/json");
            request->AddContentBody(bodyStream);
            request->SetContentLength(StringUtils::to_string(body.length()));
        }

        status = getAdminApiSessionCredentials(forceCredentialRefresh);
        if (status != GAMEKIT_SUCCESS)
        {
            return;
        }

        if(!this->signRequestWithSessionCredentials(request))
        {
            status = GameKit::GAMEKIT_ERROR_SIGN_REQUEST_FAILED;
            return;
        }

        response = m_httpClient->MakeRequest(request);
    };

    assembleAndExecuteRequest();
    if (status == GAMEKIT_SUCCESS && response->GetResponseCode() == Aws::Http::HttpResponseCode::FORBIDDEN)
    {
        // Retry once forcing it to reassume the role with permissions.
        assembleAndExecuteRequest(true);
    }
    return status;
}
#pragma endregion
