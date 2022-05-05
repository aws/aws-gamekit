// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// AWS SDK
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/utils/StringUtils.h>

// GameKit
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/core/utils/validation_utils.h>
#include <aws/gamekit/user-gameplay-data/gamekit_user_gameplay_data.h>

using namespace Aws::Http;
using namespace Aws::Utils;
using namespace Aws::Utils::Json;
using namespace GameKit::ClientSettings::UserGameplayData;
using namespace GameKit::UserGameplayData;
using namespace GameKit::Utils::HttpClient;

#define DEFAULT_CLIENT_TIMEOUT_SECONDS  3
#define DEFAULT_RETRY_INTERVAL_SECONDS  5
#define DEFAULT_MAX_QUEUE_SIZE  256
#define DEFAULT_MAX_RETRIES 32
#define DEFAULT_RETRY_STRATEGY  0
#define DEFAULT_MAX_EXPONENTIAL_BACKOFF_THRESHOLD   32
#define DEFAULT_PAGINATION_SIZE 100

#pragma region Constructors/Deconstructor
UserGameplayData::UserGameplayData(Authentication::GameKitSessionManager* sessionManager, FuncLogCallback logCb)
{
    m_sessionManager = sessionManager;
    GameKit::AwsApiInitializer::Initialize(logCb, this);

    // Set default client settings
    m_clientSettings.ClientTimeoutSeconds = DEFAULT_CLIENT_TIMEOUT_SECONDS;
    m_clientSettings.RetryIntervalSeconds = DEFAULT_RETRY_INTERVAL_SECONDS;
    m_clientSettings.MaxRetryQueueSize = DEFAULT_MAX_QUEUE_SIZE;
    m_clientSettings.MaxRetries = DEFAULT_MAX_RETRIES;
    m_clientSettings.RetryStrategy = DEFAULT_RETRY_STRATEGY;
    m_clientSettings.MaxExponentialRetryThreshold = DEFAULT_MAX_EXPONENTIAL_BACKOFF_THRESHOLD;
    m_clientSettings.PaginationSize = DEFAULT_PAGINATION_SIZE;

    m_logCb = logCb;

    this->initializeClient();

    Logging::Log(logCb, Level::Info, "User Gameplay Data instantiated");
}

void UserGameplayData::SetClientSettings(const UserGameplayDataClientSettings& settings)
{
    m_clientSettings = settings;
    this->initializeClient();

    Logging::Log(m_logCb, Level::Info, "User Gameplay Data Client settings updated.");
}

UserGameplayData::~UserGameplayData()
{
    m_customHttpClient->StopRetryBackgroundThread();
    AwsApiInitializer::Shutdown(m_logCb, this);
    m_logCb = nullptr;
}
#pragma endregion

#pragma region Public Methods
unsigned int UserGameplayData::AddUserGameplayData(UserGameplayDataBundle userGameplayDataBundle, DISPATCH_RECEIVER_HANDLE unprocessedItemsReceiver, FuncBundleResponseCallback unprocessedItemsCallback)
{
    if (!m_sessionManager->AreSettingsLoaded(FeatureType::UserGameplayData))
    {
        return GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED;
    }

    if (!Utils::ValidationUtils::IsValidPrimaryIdentifier(userGameplayDataBundle.bundleName))
    {
        const std::string errorMessage = "Error: UserGameplayData::AddUserGameplayData() malformed bundle name: " + std::string(userGameplayDataBundle.bundleName) + ". Bundle name" + GameKit::Utils::PRIMARY_IDENTIFIER_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_MALFORMED_BUNDLE_NAME;
    }

    std::stringstream invalidKeys;
    if (!validateBundleItemKeys(userGameplayDataBundle.bundleItemKeys, userGameplayDataBundle.numKeys, invalidKeys))
    {
        const std::string errorMessage = "Error: UserGameplayData::AddUserGameplayData() malformed item key(s): " + invalidKeys.str() + ". Item key(s)" + GameKit::Utils::PRIMARY_IDENTIFIER_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_MALFORMED_BUNDLE_ITEM_KEY;
    }

    const std::string uri = m_sessionManager->GetClientSettings()[SETTINGS_USER_GAMEPLAY_DATA_API_GATEWAY_BASE_URL] +
        BUNDLES_PATH_PART + userGameplayDataBundle.bundleName;
    const std::string& idToken = m_sessionManager->GetToken(GameKit::TokenType::IdToken);

    if (idToken.empty())
    {
        Logging::Log(m_logCb, Level::Info, "UserGameplayData::AddUserGameplayData() No user is currently logged in.");
        return GAMEKIT_ERROR_NO_ID_TOKEN;
    }

    auto request = CreateHttpRequest(ToAwsString(uri), HttpMethod::HTTP_POST, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

    setAuthorizationHeader(request);

    JsonValue payload;
    userGameplayDataBundle.ToJson(payload);

    std::shared_ptr<Aws::IOStream> payloadStream = Aws::MakeShared<Aws::StringStream>("AddUserGameplayDataBody");
    Aws::String serialized = payload.View().WriteCompact();
    *payloadStream << serialized;

    request->AddContentBody(payloadStream);
    request->SetContentType("application/json");
    request->SetContentLength(StringUtils::to_string(serialized.size()));

    RequestResult result = m_customHttpClient->MakeRequest(UserGameplayDataOperationType::Write, false, userGameplayDataBundle.bundleName, "", request, HttpResponseCode::CREATED, m_clientSettings.MaxRetries);

    if (result.ResultType != RequestResultType::RequestMadeSuccess)
    {
        const std::string errorMessage = "Error: UserGameplayData::AddUserGameplayData() returned with " + result.ToString();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return result.ToErrorCode();
    }

    // Parse through JSON and call callback function for the item the request failed on and all unprocessed items
    Aws::IOStream& bodyStream = result.Response->GetResponseBody();
    const JsonValue bodyJson(bodyStream);

    if (!bodyJson.WasParseSuccessful())
    {
        const Aws::String errorMessage = "Error: UserGameplayData::AddUserGameplayData() error response formatted incorrectly : " + bodyJson.GetErrorMessage();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_PARSE_JSON_FAILED;
    }

    const JsonView bodyView = bodyJson.View();
    const JsonView data = bodyView.GetObject(ENVELOPE_KEY_DATA);

    if (data.KeyExists(UNPROCESSED_ITEMS) && data.GetArray(UNPROCESSED_ITEMS).GetLength() > 0)
    {
        auto unprocessedItems = data.GetArray(UNPROCESSED_ITEMS);

        for (size_t i = 0; i < unprocessedItems.GetLength(); ++i)
        {
            JsonView& item = unprocessedItems[i];
            Aws::String bundleItemKey = item.GetString(BUNDLE_ITEM_KEY);
            Aws::String bundleItemValue = item.GetString(BUNDLE_ITEM_VALUE);

            unprocessedItemsCallback(unprocessedItemsReceiver, bundleItemKey.c_str(), bundleItemValue.c_str());
        }

        return GAMEKIT_ERROR_USER_GAMEPLAY_DATA_UNPROCESSED_ITEMS;
    }

    return result.ToErrorCode();
}

unsigned int UserGameplayData::ListUserGameplayDataBundles(DISPATCH_RECEIVER_HANDLE receiver, FuncListGameplayDataBundlesResponseCallback responseCallback)
{
    if (!m_sessionManager->AreSettingsLoaded(FeatureType::UserGameplayData))
    {
        return GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED;
    }

    const std::string uri = m_sessionManager->GetClientSettings()[SETTINGS_USER_GAMEPLAY_DATA_API_GATEWAY_BASE_URL] + LIST_BUNDLES_PATH;
    const std::string& idToken = m_sessionManager->GetToken(GameKit::TokenType::IdToken);

    if (idToken.empty())
    {
        Logging::Log(m_logCb, Level::Info, "UserGameplayData::ListUserGameplayDataBundles() No user is currently logged in.");
        return GAMEKIT_ERROR_NO_ID_TOKEN;
    }

    Aws::String startKey = "";
    Aws::String pagingToken = "";

    do
    {
        auto request = CreateHttpRequest(ToAwsString(uri), HttpMethod::HTTP_GET, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

        setAuthorizationHeader(request);
        setPaginationLimit(request, m_clientSettings.PaginationSize);

        if (startKey.length() > 0)
        {
            std::string message = "UserGameplayData::ListUserGameplayDataBundles() Sending request with pagination keys: (" + ToStdString(startKey) + ")";
            Logging::Log(m_logCb, Level::Verbose, message.c_str());

            request->AddQueryStringParameter(BUNDLE_PAGINATION_KEY.c_str(), startKey);
            startKey = "";

            request->AddQueryStringParameter(BUNDLE_PAGINATION_TOKEN.c_str(), pagingToken);
            pagingToken = "";
        }

        RequestResult result = m_customHttpClient->MakeRequest(UserGameplayDataOperationType::Get, false, "", "", request, HttpResponseCode::OK, m_clientSettings.MaxRetries);

        if (result.ResultType != RequestResultType::RequestMadeSuccess)
        {
            const std::string errorMessage = "Error: UserGameplayData::ListUserGameplayDataBundles() returned with " + result.ToString();
            Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
            return result.ToErrorCode();
        }

        // Parse through JSON and call callback function and then set start keys
        Aws::IOStream& bodyStream = result.Response->GetResponseBody();
        const JsonValue bodyJson(bodyStream);

        if (!bodyJson.WasParseSuccessful())
        {
            const Aws::String errorMessage = "Error: UserGameplayData::ListUserGameplayDataBundles() response formatted incorrectly : " + bodyJson.GetErrorMessage();
            Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
            return GAMEKIT_ERROR_PARSE_JSON_FAILED;
        }

        const JsonView bodyView = bodyJson.View();
        const JsonView data = bodyView.GetObject(ENVELOPE_KEY_DATA);
        auto allBundles = data.GetArray(BUNDLE_NAMES);

        std::string message = "UserGameplayData::ListUserGameplayDataBundles() received " + std::to_string(allBundles.GetLength()) + " bundles.";
        Logging::Log(m_logCb, Level::Verbose, message.c_str());

        for (size_t i = 0; i < allBundles.GetLength(); ++i)
        {
            JsonView& item = allBundles[i];
            Aws::String bundleName = item.GetString(BUNDLE_NAME);

            responseCallback(receiver, bundleName.c_str());
        }

        if (bodyView.KeyExists(ENVELOPE_KEY_PAGING))
        {
            const JsonView paging = bodyView.GetObject(ENVELOPE_KEY_PAGING);
            startKey = paging.KeyExists(BUNDLE_PAGINATION_KEY) ? paging.GetString(BUNDLE_PAGINATION_KEY) : "";
            if (!paging.KeyExists(BUNDLE_PAGINATION_TOKEN))
            {
                Logging::Log(m_logCb, Level::Error, "paging_token missing from response with next_start_key");
                pagingToken = "";
            }
            else
            {
                pagingToken = paging.GetString(BUNDLE_PAGINATION_TOKEN);
            }
        }
    } while (startKey.length() > 0);

    return GAMEKIT_SUCCESS;
}

unsigned int UserGameplayData::GetUserGameplayDataBundle(char* bundleName, DISPATCH_RECEIVER_HANDLE receiver, FuncBundleResponseCallback responseCallback)
{
    if (!m_sessionManager->AreSettingsLoaded(FeatureType::UserGameplayData))
    {
        return GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED;
    }

    if (!Utils::ValidationUtils::IsValidPrimaryIdentifier(bundleName))
    {
        const std::string errorMessage = "Error: UserGameplayData::GetUserGameplayDataBundle() malformed bundle name: " + std::string(bundleName) + ". Bundle name" + GameKit::Utils::PRIMARY_IDENTIFIER_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_MALFORMED_BUNDLE_NAME;
    }

    const std::string uri = m_sessionManager->GetClientSettings()[SETTINGS_USER_GAMEPLAY_DATA_API_GATEWAY_BASE_URL] +
        BUNDLES_PATH_PART + bundleName;

    const std::string& idToken = m_sessionManager->GetToken(GameKit::TokenType::IdToken);

    if (idToken.empty())
    {
        Logging::Log(m_logCb, Level::Info, "UserGameplayData::GetUserGameplayDataBundle() No user is currently logged in.");
        return GAMEKIT_ERROR_NO_ID_TOKEN;
    }

    Aws::String startKey = "";
    Aws::String pagingToken = "";

    do
    {
        auto request = CreateHttpRequest(ToAwsString(uri), Aws::Http::HttpMethod::HTTP_GET, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

        setAuthorizationHeader(request);
        setPaginationLimit(request, m_clientSettings.PaginationSize);

        if (startKey.length() > 0)
        {
            request->AddQueryStringParameter(BUNDLE_PAGINATION_KEY.c_str(), startKey);
            startKey = "";
            request->AddQueryStringParameter(BUNDLE_PAGINATION_TOKEN.c_str(), pagingToken);
            pagingToken = "";
        }

        auto result = m_customHttpClient->MakeRequest(UserGameplayDataOperationType::Get, false, bundleName, "", request, HttpResponseCode::OK, m_clientSettings.MaxRetries);

        if (result.ResultType != RequestResultType::RequestMadeSuccess)
        {
            const std::string errorMessage = "Error: UserGameplayData::GetUserGameplayDataBundle() returned with " + result.ToString();
            Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
            return result.ToErrorCode();
        }

        // Parse through JSON and call callback function and then set startKey
        Aws::IOStream& bodyStream = result.Response->GetResponseBody();
        const JsonValue bodyJson(bodyStream);

        if (!bodyJson.WasParseSuccessful())
        {
            const Aws::String errorMessage = "Error: UserGameplayData::GetUserGameplayDataBundle() response formatted incorrectly : " + bodyJson.GetErrorMessage();
            Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
            return GAMEKIT_ERROR_PARSE_JSON_FAILED;
        }

        const JsonView bodyView = bodyJson.View();
        const JsonView data = bodyView.GetObject(ENVELOPE_KEY_DATA);
        auto items = data.GetArray(BUNDLE_ITEMS);

        for (size_t i = 0; i < items.GetLength(); ++i)
        {
            auto& item = items[i];
            Aws::String itemKey = item.GetString(BUNDLE_ITEM_KEY);
            Aws::String itemValue = item.GetString(BUNDLE_ITEM_VALUE);

            responseCallback(receiver, itemKey.c_str(), itemValue.c_str());
        }

        if (bodyView.KeyExists(ENVELOPE_KEY_PAGING))
        {
            const JsonView paging = bodyView.GetObject(ENVELOPE_KEY_PAGING);
            startKey = paging.KeyExists(BUNDLE_PAGINATION_KEY) ? paging.GetString(BUNDLE_PAGINATION_KEY) : "";
            if (!paging.KeyExists(BUNDLE_PAGINATION_TOKEN))
            {
                Logging::Log(m_logCb, Level::Error, "paging_token missing from response with next_start_key");
                pagingToken = "";
            }
            else
            {
                pagingToken = paging.GetString(BUNDLE_PAGINATION_TOKEN);
            }
        }
    } while (startKey.length() > 0);

    return GAMEKIT_SUCCESS;
}

unsigned int UserGameplayData::GetUserGameplayDataBundleItem(UserGameplayDataBundleItem userGameplayDataBundleItem, DISPATCH_RECEIVER_HANDLE receiver, FuncBundleItemResponseCallback responseCallback)
{
    if (!m_sessionManager->AreSettingsLoaded(FeatureType::UserGameplayData))
    {
        return GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED;
    }

    if (!Utils::ValidationUtils::IsValidPrimaryIdentifier(userGameplayDataBundleItem.bundleName))
    {
        const std::string errorMessage = "Error: UserGameplayData::GetUserGameplayDataBundleItem() malformed bundle name: " + std::string(userGameplayDataBundleItem.bundleName) + ". Bundle name" + GameKit::Utils::PRIMARY_IDENTIFIER_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_MALFORMED_BUNDLE_NAME;
    }

    if (!Utils::ValidationUtils::IsValidPrimaryIdentifier(userGameplayDataBundleItem.bundleItemKey))
    {
        const std::string errorMessage = "Error: UserGameplayData::GetUserGameplayDataBundleItem() malformed item key: " + std::string(userGameplayDataBundleItem.bundleItemKey) + ". Item key" + GameKit::Utils::PRIMARY_IDENTIFIER_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_MALFORMED_BUNDLE_ITEM_KEY;
    }

    const std::string uri = m_sessionManager->GetClientSettings()[SETTINGS_USER_GAMEPLAY_DATA_API_GATEWAY_BASE_URL] +
        BUNDLES_PATH_PART + userGameplayDataBundleItem.bundleName +
        BUNDLE_ITEMS_PATH_PART + userGameplayDataBundleItem.bundleItemKey;

    const std::string& idToken = m_sessionManager->GetToken(GameKit::TokenType::IdToken);

    if (idToken.empty())
    {
        Logging::Log(m_logCb, Level::Info, "UserGameplayData::GetUserGameplayDataBundleItem() No user is currently logged in.");
        return GAMEKIT_ERROR_NO_ID_TOKEN;
    }

    auto const request = CreateHttpRequest(ToAwsString(uri), Aws::Http::HttpMethod::HTTP_GET, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

    setAuthorizationHeader(request);

    RequestResult const result = m_customHttpClient->MakeRequest(UserGameplayDataOperationType::Get, false, userGameplayDataBundleItem.bundleName, userGameplayDataBundleItem.bundleItemKey, request, HttpResponseCode::OK, m_clientSettings.MaxRetries);

    if (result.ResultType != RequestResultType::RequestMadeSuccess)
    {
        const std::string errorMessage = "Error: UserGameplayData::GetUserGameplayDataBundleItem() returned with " + result.ToString();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return result.ToErrorCode();
    }

    // Parse through JSON and call callback function
    Aws::IOStream& bodyStream = result.Response->GetResponseBody();
    const JsonValue bodyJson(bodyStream);

    if (!bodyJson.WasParseSuccessful())
    {
        const Aws::String errorMessage = "Error: UserGameplayData::GetUserGameplayDataBundleItem() response formatted incorrectly: " + bodyJson.GetErrorMessage();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_PARSE_JSON_FAILED;
    }

    const JsonView bodyView = bodyJson.View();
    const JsonView data = bodyView.GetObject(ENVELOPE_KEY_DATA);

    Aws::String bundleItemValue = data.GetString(BUNDLE_ITEM_VALUE);
    responseCallback(receiver, bundleItemValue.c_str());

    return GAMEKIT_SUCCESS;
}

unsigned int UserGameplayData::UpdateUserGameplayDataBundleItem(UserGameplayDataBundleItemValue userGameplayDataBundleItemValue)
{
    if (!m_sessionManager->AreSettingsLoaded(FeatureType::UserGameplayData))
    {
        return GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED;
    }

    if (!Utils::ValidationUtils::IsValidPrimaryIdentifier(userGameplayDataBundleItemValue.bundleName))
    {
        const std::string errorMessage = "Error: UserGameplayData::UpdateUserGameplayDataBundleItem() malformed bundle name: " + std::string(userGameplayDataBundleItemValue.bundleName) + ". Bundle name" + GameKit::Utils::PRIMARY_IDENTIFIER_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_MALFORMED_BUNDLE_NAME;
    }

    if (!Utils::ValidationUtils::IsValidPrimaryIdentifier(userGameplayDataBundleItemValue.bundleItemKey))
    {
        const std::string errorMessage = "Error: UserGameplayData::UpdateUserGameplayDataBundleItem() malformed item key: " + std::string(userGameplayDataBundleItemValue.bundleItemKey) + ". Item key" + GameKit::Utils::PRIMARY_IDENTIFIER_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_MALFORMED_BUNDLE_ITEM_KEY;
    }

    const std::string uri = m_sessionManager->GetClientSettings()[SETTINGS_USER_GAMEPLAY_DATA_API_GATEWAY_BASE_URL] +
        BUNDLES_PATH_PART + userGameplayDataBundleItemValue.bundleName +
        BUNDLE_ITEMS_PATH_PART + userGameplayDataBundleItemValue.bundleItemKey;
    const std::string& idToken = m_sessionManager->GetToken(GameKit::TokenType::IdToken);

    if (idToken.empty())
    {
        Logging::Log(m_logCb, Level::Info, "UserGameplayData::UpdateUserGameplayDataBundleItem() No user is currently logged in.");
        return GAMEKIT_ERROR_NO_ID_TOKEN;
    }

    auto request = CreateHttpRequest(ToAwsString(uri), Aws::Http::HttpMethod::HTTP_PUT, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

    setAuthorizationHeader(request);

    JsonValue payload;
    userGameplayDataBundleItemValue.ToJson(payload);

    const std::shared_ptr<Aws::IOStream> payloadStream = Aws::MakeShared<Aws::StringStream>("UpdateUserGameplayDataBundleItemsBody");
    const Aws::String serialized = payload.View().WriteCompact();
    *payloadStream << serialized;

    request->AddContentBody(payloadStream);
    request->SetContentType("application/json");
    request->SetContentLength(StringUtils::to_string(serialized.size()));

    const RequestResult result = m_customHttpClient->MakeRequest(UserGameplayDataOperationType::Write, false, userGameplayDataBundleItemValue.bundleName, userGameplayDataBundleItemValue.bundleItemKey, request, HttpResponseCode::NO_CONTENT, m_clientSettings.MaxRetries);

    if (result.ResultType != RequestResultType::RequestMadeSuccess)
    {
        const std::string errorMessage = "Error: UserGameplayData::UpdateUserGameplayDataBundleItem() returned with " + result.ToString();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
    }

    return result.ToErrorCode();
}

unsigned int UserGameplayData::DeleteAllUserGameplayData()
{
    if (!m_sessionManager->AreSettingsLoaded(FeatureType::UserGameplayData))
    {
        return GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED;
    }

    const std::string uri = m_sessionManager->GetClientSettings()[SETTINGS_USER_GAMEPLAY_DATA_API_GATEWAY_BASE_URL];
    const std::string& idToken = m_sessionManager->GetToken(GameKit::TokenType::IdToken);

    if (idToken.empty())
    {
        Logging::Log(m_logCb, Level::Info, "UserGameplayData::DeleteAllUserGameplayData() No user is currently logged in.");
        return GAMEKIT_ERROR_NO_ID_TOKEN;
    }

    const auto request = CreateHttpRequest(ToAwsString(uri), Aws::Http::HttpMethod::HTTP_DELETE, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

    setAuthorizationHeader(request);

    const RequestResult result = m_customHttpClient->MakeRequest(UserGameplayDataOperationType::Delete, false, "", "", request, HttpResponseCode::NO_CONTENT, m_clientSettings.MaxRetries);

    if (result.ResultType != RequestResultType::RequestMadeSuccess)
    {
        const std::string errorMessage = "Error: UserGameplayData::DeleteAllUserGameplayData() returned with " + result.ToString();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
    }

    return result.ToErrorCode();
}

unsigned int UserGameplayData::DeleteUserGameplayDataBundle(char* bundleName)
{
    if (!m_sessionManager->AreSettingsLoaded(FeatureType::UserGameplayData))
    {
        return GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED;
    }

    if (!Utils::ValidationUtils::IsValidPrimaryIdentifier(bundleName))
    {
        const std::string errorMessage = "Error: UserGameplayData::DeleteUserGameplayDataBundle() malformed bundle name: " + std::string(bundleName) + ". Bundle name" + GameKit::Utils::PRIMARY_IDENTIFIER_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_MALFORMED_BUNDLE_NAME;
    }

    const std::string uri = m_sessionManager->GetClientSettings()[SETTINGS_USER_GAMEPLAY_DATA_API_GATEWAY_BASE_URL] +
        BUNDLES_PATH_PART + bundleName;
    const auto& idToken = m_sessionManager->GetToken(GameKit::TokenType::IdToken);

    if (idToken.empty())
    {
        Logging::Log(m_logCb, Level::Info, "UserGameplayData::DeleteUserGameplayDataBundle() No user is currently logged in.");
        return GAMEKIT_ERROR_NO_ID_TOKEN;
    }

    const auto request = CreateHttpRequest(ToAwsString(uri), Aws::Http::HttpMethod::HTTP_DELETE, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

    setAuthorizationHeader(request);

    const RequestResult result = m_customHttpClient->MakeRequest(UserGameplayDataOperationType::Delete, false, bundleName, "", request, HttpResponseCode::NO_CONTENT, m_clientSettings.MaxRetries);

    if (result.ResultType != RequestResultType::RequestMadeSuccess)
    {
        const std::string errorMessage = "Error: UserGameplayData::DeleteUserGameplayDataBundle() returned with " + result.ToString();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
    }

    return result.ToErrorCode();
}

unsigned int UserGameplayData::DeleteUserGameplayDataBundleItems(UserGameplayDataDeleteItemsRequest deleteItemsRequest)
{
    if (!m_sessionManager->AreSettingsLoaded(FeatureType::UserGameplayData))
    {
        return GAMEKIT_ERROR_SETTINGS_FILE_READ_FAILED;
    }

    if (!Utils::ValidationUtils::IsValidPrimaryIdentifier(deleteItemsRequest.bundleName))
    {
        const std::string errorMessage = "Error: UserGameplayData::DeleteUserGameplayDataBundleItems() malformed bundle name: " + std::string(deleteItemsRequest.bundleName) + ". Bundle name" + GameKit::Utils::PRIMARY_IDENTIFIER_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_MALFORMED_BUNDLE_NAME;
    }

    std::stringstream invalidKeys;
    if (!validateBundleItemKeys(deleteItemsRequest.bundleItemKeys, deleteItemsRequest.numKeys, invalidKeys))
    {
        const std::string errorMessage = "Error: UserGameplayData::DeleteUserGameplayDataBundleItem() malformed item key(s): " + invalidKeys.str() + ". Item key(s)" + GameKit::Utils::PRIMARY_IDENTIFIER_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_MALFORMED_BUNDLE_ITEM_KEY;
    }

    std::string uri = m_sessionManager->GetClientSettings()[SETTINGS_USER_GAMEPLAY_DATA_API_GATEWAY_BASE_URL] +
        BUNDLES_PATH_PART + deleteItemsRequest.bundleName;
    const std::string& idToken = m_sessionManager->GetToken(GameKit::TokenType::IdToken);

    if (idToken.empty())
    {
        Logging::Log(m_logCb, Level::Info, "UserGameplayData::DeleteUserGameplayDataBundleItem() No user is currently logged in.");
        return GAMEKIT_ERROR_NO_ID_TOKEN;
    }

    auto request = CreateHttpRequest(ToAwsString(uri), Aws::Http::HttpMethod::HTTP_DELETE, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

    setAuthorizationHeader(request);

    JsonValue payload;
    deleteItemsRequest.ToJson(payload);

    Aws::String serialized = payload.View().WriteCompact();

    // Some HTTP clients don't append the body to the DELETE request, causing the whole bundle to
    // be deleted so we pass the keys in the query string.
    Aws::String urlEncodedPayload = StringUtils::URLEncode(serialized.c_str());
    if (urlEncodedPayload.length() > GameKit::Utils::MAX_URL_PARAM_CHARS)
    {
        const std::string errorMessage = "Error: UserGameplayData::DeleteUserGameplayDataBundleItem() payload is above " + std::to_string(GameKit::Utils::MAX_URL_PARAM_CHARS) + " maximum length, reduce the number of items to delete.";
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_USER_GAMEPLAY_DATA_PAYLOAD_TOO_LARGE;
    }
    request->AddQueryStringParameter("payload", urlEncodedPayload);

    // Since the request operates on several items, passing empty string as item name.
    // The filtering logic does not handle operations on multiple items in the same request.
    RequestResult result = m_customHttpClient->MakeRequest(UserGameplayDataOperationType::Delete, false, deleteItemsRequest.bundleName, "", request, HttpResponseCode::NO_CONTENT, m_clientSettings.MaxRetries);

    if (result.ResultType != RequestResultType::RequestMadeSuccess)
    {
        const std::string errorMessage = "Error: UserGameplayData::DeleteUserGameplayDataBundleItem() returned with " + result.ToString();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
    }

    return result.ToErrorCode();
}

void UserGameplayData::StartRetryBackgroundThread()
{
    m_customHttpClient->StartRetryBackgroundThread();
}

void UserGameplayData::StopRetryBackgroundThread()
{
    m_customHttpClient->StopRetryBackgroundThread();
}

void UserGameplayData::SetNetworkChangeCallback(NETWORK_STATE_RECEIVER_HANDLE receiverHandle, NetworkStatusChangeCallback statusChangeCallback)
{
    m_customHttpClient->SetNetworkChangeCallback(receiverHandle, statusChangeCallback);
}

void UserGameplayData::SetCacheProcessedCallback(CACHE_PROCESSED_RECEIVER_HANDLE receiverHandle, CacheProcessedCallback cacheProcessedCallback)
{
    m_customHttpClient->SetCacheProcessedCallback(receiverHandle, cacheProcessedCallback);
}

void UserGameplayData::DropAllCachedEvents()
{
    m_customHttpClient->DropAllCachedEvents();
}

unsigned int UserGameplayData::PersistApiCallsToCache(const std::string& offlineCacheFile)
{
    const auto serializer = static_cast<bool(*)(std::ostream& os, const std::shared_ptr<IOperation>, FuncLogCallback)>(&UserGameplayDataOperation::TrySerializeBinary);
    return m_customHttpClient->PersistQueue(offlineCacheFile, serializer, true) ?
        GAMEKIT_SUCCESS : GAMEKIT_ERROR_USER_GAMEPLAY_DATA_CACHE_WRITE_FAILED;
}

unsigned int UserGameplayData::LoadApiCallsFromCache(const std::string& offlineCacheFile)
{
    const auto deserializer = static_cast<bool(*)(std::istream& is, std::shared_ptr<IOperation>&, FuncLogCallback)>(&UserGameplayDataOperation::TryDeserializeBinary);
    return m_customHttpClient->LoadQueue(offlineCacheFile, deserializer, true) ?
        GAMEKIT_SUCCESS : GAMEKIT_ERROR_USER_GAMEPLAY_DATA_CACHE_READ_FAILED;
}
#pragma endregion

#pragma region Private Methods
void UserGameplayData::initializeClient()
{
    if (m_clientSettings.ClientTimeoutSeconds == 0)
    {
        m_clientSettings.ClientTimeoutSeconds = DEFAULT_CLIENT_TIMEOUT_SECONDS;
    }

    if (m_clientSettings.RetryIntervalSeconds == 0)
    {
        m_clientSettings.RetryIntervalSeconds = DEFAULT_RETRY_INTERVAL_SECONDS;
    }

    if (m_clientSettings.MaxRetryQueueSize == 0)
    {
        m_clientSettings.MaxRetryQueueSize = DEFAULT_MAX_QUEUE_SIZE;
    }

    if (m_clientSettings.MaxRetries == 0)
    {
        m_clientSettings.MaxRetries = DEFAULT_MAX_RETRIES;
    }

    if (m_clientSettings.RetryStrategy > 1)
    {
        // invalid value, set to default
        m_clientSettings.RetryStrategy = DEFAULT_RETRY_STRATEGY;
    }

    if (m_clientSettings.MaxExponentialRetryThreshold == 0)
    {
        m_clientSettings.MaxExponentialRetryThreshold = DEFAULT_MAX_EXPONENTIAL_BACKOFF_THRESHOLD;
    }

    if (m_clientSettings.PaginationSize == 0)
    {
        m_clientSettings.PaginationSize = DEFAULT_PAGINATION_SIZE;
    }

    // Low level client settings
    Aws::Client::ClientConfiguration clientConfig;
    GameKit::DefaultClients::SetDefaultClientConfiguration(m_sessionManager->GetClientSettings(), clientConfig);
    clientConfig.connectTimeoutMs = m_clientSettings.ClientTimeoutSeconds * 1000;
    clientConfig.httpRequestTimeoutMs = m_clientSettings.ClientTimeoutSeconds * 1000;
    clientConfig.requestTimeoutMs = m_clientSettings.ClientTimeoutSeconds * 1000;
    clientConfig.region = m_sessionManager->GetClientSettings()[ClientSettings::Authentication::SETTINGS_IDENTITY_REGION].c_str();

    auto lowLevelHttpClient = Aws::Http::CreateHttpClient(clientConfig);

    // High level settings for custom client
    auto strategyBuilder = [&]()
    {
        StrategyType strategyType = (StrategyType)m_clientSettings.RetryStrategy;
        std::shared_ptr<IRetryStrategy> retryLogic;
        switch (strategyType)
        {
        case StrategyType::ExponentialBackoff:
            retryLogic = std::make_shared<ExponentialBackoffStrategy>(m_clientSettings.MaxExponentialRetryThreshold, m_logCb);
            break;
        case StrategyType::ConstantInterval:
            retryLogic = std::make_shared<ConstantIntervalStrategy>();
            break;
        }

        return retryLogic;
    };

    // Auth token setter
    std::function<void(std::shared_ptr<HttpRequest>)> authSetter =
        std::bind(&UserGameplayData::setAuthorizationHeader, this, std::placeholders::_1);

    // Build custom client with retry logic
    auto retryStrategy = strategyBuilder();
    m_customHttpClient = std::make_shared<UserGameplayDataHttpClient>(
        lowLevelHttpClient, authSetter, m_clientSettings.RetryIntervalSeconds, retryStrategy, m_clientSettings.MaxRetryQueueSize, m_logCb);
}

void UserGameplayData::setAuthorizationHeader(std::shared_ptr<HttpRequest> request)
{
    std::string value = "Bearer " + m_sessionManager->GetToken(GameKit::TokenType::IdToken);
    request->SetHeaderValue(HEADER_AUTHORIZATION, ToAwsString(value));
}

void UserGameplayData::setPaginationLimit(std::shared_ptr<HttpRequest> request, unsigned int paginationLimit)
{
    request->AddQueryStringParameter(LIMIT_KEY.c_str(), StringUtils::to_string(paginationLimit));
}

bool UserGameplayData::validateBundleItemKeys(const char* const* bundleItemKeys, int numKeys, std::stringstream& tempBuffer)
{
    bool valid = true;
    for (int i = 0; i < numKeys; i++)
    {
        if (!Utils::ValidationUtils::IsValidPrimaryIdentifier(bundleItemKeys[i]))
        {
            valid = false;

            if (tempBuffer.tellp() == std::streampos(0))
            {
                tempBuffer << bundleItemKeys[i];
            }
            else
            {
                tempBuffer << ", " << bundleItemKeys[i];
            }
        }
    }

    return valid;
}

void UserGameplayData::setHttpClient(std::shared_ptr<Aws::Http::HttpClient> httpClient)
{
    m_customHttpClient->SetLowLevelHttpClient(httpClient);
}
#pragma endregion
