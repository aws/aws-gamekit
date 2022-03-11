// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/identity/gamekit_identity.h>

namespace Cognito = Aws::CognitoIdentityProvider;
namespace CognitoModel = Aws::CognitoIdentityProvider::Model;

#pragma region Constructors/Destructor
GameKit::Identity::Identity::Identity(FuncLogCallback logCb, Authentication::GameKitSessionManager* sessionManager)
{
    Logging::Log(logCb, Level::Info, "Identity::Identity()");

    m_logCb = logCb;
    m_sessionManager = sessionManager;

    GameKit::AwsApiInitializer::Initialize(m_logCb, this);

    Aws::Client::ClientConfiguration clientConfig;
    GameKit::DefaultClients::SetDefaultClientConfiguration(m_sessionManager->GetClientSettings(), clientConfig);
    clientConfig.region = m_sessionManager->GetClientSettings()[ClientSettings::Authentication::SETTINGS_IDENTITY_REGION].c_str();

    m_httpClient = Aws::Http::CreateHttpClient(clientConfig);
    Logging::Log(m_logCb, Level::Info, "Identity::Identity() >> Identity instantiated");
}

GameKit::Identity::Identity::~Identity()
{
    if (m_awsClientsInitializedInternally)
    {
        delete(m_cognitoClient);
    }

    GameKit::AwsApiInitializer::Shutdown(m_logCb, this);
}
#pragma endregion

#pragma region Public Methods
unsigned int GameKit::Identity::Identity::Register(UserRegistration userRegistration)
{
    auto sha256 = Aws::Utils::Crypto::Sha256();
    std::string gamekitUserId;
    std::string gamekitUserHashKey;

    if (userRegistration.userId == nullptr || strlen(userRegistration.userId) == 0) // This is a new registration by email/password
    {
        gamekitUserId = Aws::String(Aws::Utils::UUID::RandomUUID()).c_str();
        std::string keyUuid = Aws::String(Aws::Utils::UUID::RandomUUID()).c_str();

        boost::algorithm::to_lower(gamekitUserId);
        boost::algorithm::to_lower(keyUuid);

        auto hashResult = sha256.Calculate(ToAwsString(keyUuid));
        auto base64 = Aws::Utils::Base64::Base64();
        gamekitUserHashKey = ToStdString(base64.Encode(hashResult.GetResult()));
    }
    else // This is a guest user being fully registered
    {
        Logging::Log(m_logCb, Level::Error, "Guest Registration not yet implemented");
        return GAMEKIT_ERROR_METHOD_NOT_IMPLEMENTED;
    }

    if (!GameKit::Utils::CredentialsUtils::IsValidUsername(userRegistration.userName))
    {
        std::string errorMessage = "Error: Identity::Register: Malformed Username. " + GameKit::Utils::USERNAME_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GameKit::GAMEKIT_ERROR_MALFORMED_USERNAME;
    }

    if (!GameKit::Utils::CredentialsUtils::IsValidPassword(userRegistration.password))
    {
        std::string errorMessage = "Error: Identity::Register: Malformed Password. " + GameKit::Utils::PASSWORD_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GameKit::GAMEKIT_ERROR_MALFORMED_PASSWORD;
    }

    auto request = CognitoModel::SignUpRequest()
        .WithClientId(m_sessionManager->GetClientSettings()[ClientSettings::Authentication::SETTINGS_USER_POOL_CLIENT_ID].c_str())
        .WithUsername(userRegistration.userName)
        .WithPassword(userRegistration.password)
        .WithUserAttributes(Aws::Vector<CognitoModel::AttributeType>{
        CognitoModel::AttributeType().WithName(ATTR_EMAIL).WithValue(userRegistration.email),
            CognitoModel::AttributeType().WithName(ATTR_CUSTOM_GAMEKIT_USER_ID).WithValue(gamekitUserId.c_str()),
            CognitoModel::AttributeType().WithName(ATTR_CUSTOM_GAMEKIT_USER_HASH_KEY).WithValue(gamekitUserHashKey.c_str()),
    });

    auto outcome = m_cognitoClient->SignUp(request);
    if (!outcome.IsSuccess())
    {
        auto error = outcome.GetError();
        Aws::String errorMessage = "Error: Identity::Register: " + error.GetExceptionName() + ": " + error.GetMessage();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());

        return GameKit::GAMEKIT_ERROR_REGISTER_USER_FAILED;
    }

    return GameKit::GAMEKIT_SUCCESS;
}

unsigned int GameKit::Identity::Identity::ConfirmRegistration(ConfirmRegistrationRequest confirmationRequest)
{
    if (!GameKit::Utils::CredentialsUtils::IsValidUsername(confirmationRequest.userName))
    {
        std::string errorMessage = "Error: Identity::ConfirmRegister: Malformed Username. " + GameKit::Utils::USERNAME_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GameKit::GAMEKIT_ERROR_MALFORMED_USERNAME;
    }

    auto request = CognitoModel::ConfirmSignUpRequest()
        .WithClientId(m_sessionManager->GetClientSettings()[ClientSettings::Authentication::SETTINGS_USER_POOL_CLIENT_ID].c_str())
        .WithUsername(confirmationRequest.userName)
        .WithConfirmationCode(confirmationRequest.confirmationCode);

    auto outcome = m_cognitoClient->ConfirmSignUp(request);
    if (!outcome.IsSuccess())
    {
        auto error = outcome.GetError();
        auto errorMessage = "Error: Identity::ConfirmRegister: " + error.GetExceptionName() + ": " + error.GetMessage();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());

        return GameKit::GAMEKIT_ERROR_CONFIRM_REGISTRATION_FAILED;
    }

    return GameKit::GAMEKIT_SUCCESS;
}

unsigned int GameKit::Identity::Identity::ResendConfirmationCode(ResendConfirmationCodeRequest resendConfirmationRequest)
{
    if (!GameKit::Utils::CredentialsUtils::IsValidUsername(resendConfirmationRequest.userName))
    {
        std::string errorMessage = "Error: Identity::ResendConfirmationCode: Malformed Username. " + GameKit::Utils::USERNAME_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GameKit::GAMEKIT_ERROR_MALFORMED_USERNAME;
    }

    auto request = CognitoModel::ResendConfirmationCodeRequest()
        .WithClientId(m_sessionManager->GetClientSettings()[ClientSettings::Authentication::SETTINGS_USER_POOL_CLIENT_ID].c_str())
        .WithUsername(resendConfirmationRequest.userName);

    auto outcome = m_cognitoClient->ResendConfirmationCode(request);
    if (!outcome.IsSuccess())
    {
        auto error = outcome.GetError();
        auto errorMessage = "Error: Identity::ResendConfirmationCode: " + error.GetExceptionName() + ": " + error.GetMessage();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());

        return GameKit::GAMEKIT_ERROR_RESEND_CONFIRMATION_CODE_FAILED;
    }

    return GameKit::GAMEKIT_SUCCESS;
}

unsigned int GameKit::Identity::Identity::Login(UserLogin userLogin)
{
    if (!GameKit::Utils::CredentialsUtils::IsValidUsername(userLogin.userName))
    {
        std::string errorMessage = "Error: Identity::Login: Malformed Username. " + GameKit::Utils::USERNAME_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GameKit::GAMEKIT_ERROR_MALFORMED_USERNAME;
    }

    if (!GameKit::Utils::CredentialsUtils::IsValidPassword(userLogin.password))
    {
        std::string errorMessage = "Error: Identity::Login: Malformed Password. " + GameKit::Utils::PASSWORD_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GameKit::GAMEKIT_ERROR_MALFORMED_PASSWORD;
    }

    auto request = CognitoModel::InitiateAuthRequest()
        .WithClientId(m_sessionManager->GetClientSettings()[ClientSettings::Authentication::SETTINGS_USER_POOL_CLIENT_ID].c_str())
        .WithAuthFlow(CognitoModel::AuthFlowType::USER_PASSWORD_AUTH)
        .AddAuthParameters("USERNAME", userLogin.userName)
        .AddAuthParameters("PASSWORD", userLogin.password);

    auto outcome = m_cognitoClient->InitiateAuth(request);
    if (!outcome.IsSuccess())
    {
        auto error = outcome.GetError();
        Aws::String errorMessage = "Error: Identity::Login: " + error.GetExceptionName() + ": " + error.GetMessage();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());

        return GameKit::GAMEKIT_ERROR_LOGIN_FAILED;
    }

    // Store tokens in SessionManager. This will also start the background thread for refreshing the tokens
    if (m_sessionManager != nullptr)
    {
        std::string accessToken = ToStdString(outcome.GetResult().GetAuthenticationResult().GetAccessToken());
        std::string refreshToken = ToStdString(outcome.GetResult().GetAuthenticationResult().GetRefreshToken());
        std::string idToken = ToStdString(outcome.GetResult().GetAuthenticationResult().GetIdToken());
        int expiresIn = outcome.GetResult().GetAuthenticationResult().GetExpiresIn();

        m_sessionManager->SetToken(GameKit::TokenType::AccessToken, accessToken);
        m_sessionManager->SetToken(GameKit::TokenType::RefreshToken, refreshToken);
        m_sessionManager->SetToken(GameKit::TokenType::IdToken, idToken);
        m_sessionManager->SetSessionExpiration(expiresIn);
    }
    else
    {
        Logging::Log(m_logCb, Level::Error, "A SessionManager was not initialized for this Identity instance. No tokens will be persisted.");
    }

    return GameKit::GAMEKIT_SUCCESS;
}

unsigned int GameKit::Identity::Identity::Logout()
{
    std::string refreshToken = m_sessionManager->GetToken(GameKit::TokenType::RefreshToken);
    if (refreshToken.empty())
    {
        Logging::Log(m_logCb, Level::Info, "Identity::Logout() No user is currently logged in.");
        return GAMEKIT_ERROR_LOGIN_FAILED;
    }

    std::string client_id = m_sessionManager->GetClientSettings()[ClientSettings::Authentication::SETTINGS_USER_POOL_CLIENT_ID];

    auto revokeRequest = CognitoModel::RevokeTokenRequest()
        .WithToken(refreshToken.c_str())
        .WithClientId(client_id.c_str());

    auto outcome = m_cognitoClient->RevokeToken(revokeRequest);
    if (!outcome.IsSuccess())
    {
        auto error = outcome.GetError();
        auto errorMessage = "Error: Identity::Logout: " + error.GetExceptionName() + ": " + error.GetMessage();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GameKit::GAMEKIT_ERROR_LOGOUT_FAILED;
    }

    m_sessionManager->DeleteToken(GameKit::TokenType::AccessToken);
    m_sessionManager->DeleteToken(GameKit::TokenType::IdToken);
    m_sessionManager->DeleteToken(GameKit::TokenType::RefreshToken);
    return GAMEKIT_SUCCESS;
}

unsigned int GameKit::Identity::Identity::ForgotPassword(ForgotPasswordRequest forgotPasswordRequest)
{
    if (!GameKit::Utils::CredentialsUtils::IsValidUsername(forgotPasswordRequest.userName))
    {
        std::string errorMessage = "Error: Identity::ForgotPassword: Malformed Username. " + GameKit::Utils::USERNAME_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GameKit::GAMEKIT_ERROR_MALFORMED_USERNAME;
    }

    auto request = CognitoModel::ForgotPasswordRequest()
        .WithClientId(m_sessionManager->GetClientSettings()[ClientSettings::Authentication::SETTINGS_USER_POOL_CLIENT_ID].c_str())
        .WithUsername(forgotPasswordRequest.userName);

    auto outcome = m_cognitoClient->ForgotPassword(request);
    if (!outcome.IsSuccess())
    {
        auto error = outcome.GetError();
        Aws::String errorMessage = Aws::String("Error: Identity::ForgotPassword: ").append(error.GetExceptionName()).append(": ").append(error.GetMessage());
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());

        return GameKit::GAMEKIT_ERROR_FORGOT_PASSWORD_FAILED;
    }

    return GameKit::GAMEKIT_SUCCESS;
}

unsigned int GameKit::Identity::Identity::ConfirmForgotPassword(ConfirmForgotPasswordRequest confirmForgotPasswordRequest)
{
    if (!GameKit::Utils::CredentialsUtils::IsValidUsername(confirmForgotPasswordRequest.userName))
    {
        std::string errorMessage = "Error: Identity::ConfirmForgotPassword: Malformed Username. " + GameKit::Utils::USERNAME_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GameKit::GAMEKIT_ERROR_MALFORMED_USERNAME;
    }

    if (!GameKit::Utils::CredentialsUtils::IsValidPassword(confirmForgotPasswordRequest.newPassword))
    {
        std::string errorMessage = "Error: Identity::ConfirmForgotPassword: Malformed Password. " + GameKit::Utils::PASSWORD_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GameKit::GAMEKIT_ERROR_MALFORMED_PASSWORD;
    }

    auto request = CognitoModel::ConfirmForgotPasswordRequest()
        .WithClientId(m_sessionManager->GetClientSettings()[ClientSettings::Authentication::SETTINGS_USER_POOL_CLIENT_ID].c_str())
        .WithUsername(confirmForgotPasswordRequest.userName)
        .WithPassword(confirmForgotPasswordRequest.newPassword)
        .WithConfirmationCode(confirmForgotPasswordRequest.confirmationCode);

    auto outcome = m_cognitoClient->ConfirmForgotPassword(request);
    if (!outcome.IsSuccess())
    {
        auto error = outcome.GetError();
        Aws::String errorMessage = "Error: Identity::ConfirmForgotPassword: " + error.GetExceptionName() + ": " + error.GetMessage();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());

        return GameKit::GAMEKIT_ERROR_CONFIRM_FORGOT_PASSWORD_FAILED;
    }

    return GameKit::GAMEKIT_SUCCESS;
}

unsigned int GameKit::Identity::Identity::GetUser(const DISPATCH_RECEIVER_HANDLE receiver, const GameKit::FuncIdentityGetUserResponseCallback responseCallback)
{
    std::string idToken = m_sessionManager->GetToken(GameKit::TokenType::IdToken);
    if (idToken.empty())
    {
        Logging::Log(m_logCb, Level::Info, "Identity::GetUser() No user is currently logged in.");
        return GAMEKIT_ERROR_NO_ID_TOKEN;
    }

    std::string fullUri = m_sessionManager->GetClientSettings()[ClientSettings::Authentication::SETTINGS_IDENTITY_API_GATEWAY_BASE_URL] + "/getuser";
    Aws::Http::URI uri((ToAwsString(fullUri)));
    Logging::Log(m_logCb, Level::Info, std::string("Identity::GetUser() >> Url: '" + fullUri + "'").c_str());
    
    std::shared_ptr<Aws::Http::HttpRequest> request = Aws::Http::CreateHttpRequest(uri, Aws::Http::HttpMethod::HTTP_GET, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
    request->SetAuthorization(ToAwsString(idToken));

    std::shared_ptr<Aws::Http::HttpResponse> response = m_httpClient->MakeRequest(request);
    if (response->GetResponseCode() != Aws::Http::HttpResponseCode::OK)
    {
        auto errorMessage = "Error: Identity::GetUser() returned with http response code: " + std::to_string(static_cast<int>(response->GetResponseCode()));
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_HTTP_REQUEST_FAILED;
    }

    Aws::IOStream& body = response->GetResponseBody();
    Aws::Utils::Json::JsonValue value(body);

    if (!value.WasParseSuccessful())
    {
        Aws::String errorMessage = "Error: Identity:GetUser() response formatted incorrectly: " + value.GetErrorMessage();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_PARSE_JSON_FAILED;
    }

    Aws::String responseUserId;
    Aws::String responseCreatedAt;
    Aws::String responseUpdatedAt;
    Aws::String responseFbExternalId;
    Aws::String responseFbRefId;
    Aws::String responseUserName;
    Aws::String responseUserEmail;

    const JsonView view = value.View().GetObject("data");

    if (!view.KeyExists(GameKit::Identity::USER_ID))
    {
        Aws::String errorMessage = "Error: Identity:GetUser() response formatted incorrectly: " + value.GetErrorMessage();
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_PARSE_JSON_FAILED;
    }

    responseUserId = view.GetString(GameKit::Identity::USER_ID);
    responseCreatedAt = view.GetString(GameKit::Identity::USER_CREATED_AT);
    responseUpdatedAt = view.GetString(GameKit::Identity::USER_UPDATED_AT);
    responseFbExternalId = view.GetString(GameKit::Identity::USER_FB_EXTERNAL_ID);
    responseFbRefId = view.GetString(GameKit::Identity::USER_FB_REF_ID);
    responseUserName = view.GetString(GameKit::Identity::USER_NAME);

    // Get email address from cognito
    CognitoModel::GetUserRequest getUserRequest;
    std::string accessToken = m_sessionManager->GetToken(GameKit::TokenType::AccessToken);
    getUserRequest.SetAccessToken(ToAwsString(accessToken));
    CognitoModel::GetUserOutcome getUserOutcome{ m_cognitoClient->GetUser(getUserRequest) };

    if (getUserOutcome.IsSuccess())
    {
        CognitoModel::GetUserResult getUserResult{ getUserOutcome.GetResult() };
        for (const auto& attribute : getUserResult.GetUserAttributes())
        {
            if (attribute.GetName() == GameKit::Identity::USER_EMAIL)
            {
                responseUserEmail = attribute.GetValue();
            }
        }
    }
    else
    {
        Aws::String errorMessage = Aws::String("Warning: Identity:GetUser() Failed to retrive user email address: ").append(value.GetErrorMessage());
        Logging::Log(m_logCb, Level::Warning, errorMessage.c_str());
    }

    GetUserResponse getUserResponse = { responseUserId.c_str(),
                                        responseCreatedAt.c_str(),
                                        responseUpdatedAt.c_str(),
                                        responseFbExternalId.c_str(),
                                        responseFbRefId.c_str(),
                                        responseUserName.c_str(),
                                        responseUserEmail.c_str() };

    if ( nullptr != receiver && nullptr != responseCallback)
    {
        responseCallback(receiver, &getUserResponse);
    }
    return GAMEKIT_SUCCESS;
}

unsigned int GameKit::Identity::Identity::GetFacebookLoginUrl(DISPATCH_RECEIVER_HANDLE dispatchReceiver, KeyValueCharPtrCallbackDispatcher responseCallback)
{
    auto provider = FederatedIdentityProviderFactory<FacebookIdentityProvider>::CreateProviderWithHttpClient(m_sessionManager->GetClientSettings(), m_httpClient, m_logCb);
    const auto loginUrl = provider.GetLoginUrl();
    if (!(dispatchReceiver == nullptr) && !(responseCallback == nullptr))
    {
        responseCallback(dispatchReceiver, KEY_FEDERATED_LOGIN_URL_REQUEST_ID.c_str(), loginUrl.requestId.c_str());
        responseCallback(dispatchReceiver, KEY_FEDERATED_LOGIN_URL.c_str(), loginUrl.loginUrl.c_str());
    }

    return GameKit::GAMEKIT_SUCCESS;
}

unsigned int GameKit::Identity::Identity::PollFacebookLoginCompletion(const std::string& requestId, int timeout, std::string& encryptedLocation)
{
    auto provider = FederatedIdentityProviderFactory<FacebookIdentityProvider>::CreateProviderWithHttpClient(m_sessionManager->GetClientSettings(), m_httpClient, m_logCb);
    return provider.PollForCompletion(requestId, timeout, encryptedLocation);
}

unsigned int GameKit::Identity::Identity::RetrieveFacebookTokens(const std::string& location)
{
    auto provider = FederatedIdentityProviderFactory<FacebookIdentityProvider>::CreateProviderWithHttpClient(m_sessionManager->GetClientSettings(), m_httpClient, m_logCb);
    std::string tokenString;
    unsigned int result = provider.RetrieveTokens(location, tokenString);
    if (tokenString.empty() || result != GameKit::GAMEKIT_SUCCESS )
    {
        return result;
    }

    auto json = Aws::Utils::Json::JsonValue(ToAwsString(tokenString));
    m_sessionManager->SetToken(GameKit::TokenType::AccessToken, ToStdString(json.View().GetString("access_token")));
    m_sessionManager->SetToken(GameKit::TokenType::RefreshToken, ToStdString(json.View().GetString("refresh_token")));
    m_sessionManager->SetToken(GameKit::TokenType::IdToken, ToStdString(json.View().GetString("id_token")));

    return result;
}

void GameKit::Identity::Identity::InitializeDefaultAwsClients()
{
    m_awsClientsInitializedInternally = true;
    Aws::Client::ClientConfiguration clientConfig;
    GameKit::DefaultClients::SetDefaultClientConfiguration(m_sessionManager->GetClientSettings(), clientConfig);
    clientConfig.region = ToAwsString(m_sessionManager->GetClientSettings()[ClientSettings::Authentication::SETTINGS_IDENTITY_REGION]);
    m_cognitoClient = DefaultClients::GetDefaultCognitoIdentityProviderClient(clientConfig);
}
#pragma endregion
