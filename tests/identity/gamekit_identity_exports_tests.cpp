// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <iostream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "gamekit_identity_exports_tests.h"

#include "aws/gamekit/authentication/exports.h"
#include "aws/gamekit/core/logging.h"
#include "aws/gamekit/core/awsclients/api_initializer.h"
#include "aws/gamekit/identity/exports.h"

#define TEST_APP_ID	"MyTestAppId"
#define TEST_REGION "us-west-2"
#define TEST_USERNAME "Someone"
#define TEST_PASSWORD "********"
#define TEST_EMAIL "user@host.tld"
#define TEST_CONFIRMATION_CODE "123"
#define INVALID_USERNAME "a"
#define INVALID_PASSWORD "*******"

using namespace Aws::CognitoIdentityProvider::Model;
using namespace GameKit::Tests::IdentityExports;
using namespace testing;

void responseCallback(DISPATCH_RECEIVER_HANDLE receiver, const GameKit::GetUserResponse* responsePayload)
{
    ((GameKit::Tests::IdentityExports::Dispatcher*) receiver)->CallbackHandler(responsePayload);
}

void GameKit::Tests::IdentityExports::Dispatcher::CallbackHandler(const GameKit::GetUserResponse* res)
{
    this->email = res->email;
    this->userName = res->userName;
    this->userId = res->userId;
}

GameKitIdentityExportsTestFixture::GameKitIdentityExportsTestFixture()
{}

GameKitIdentityExportsTestFixture::~GameKitIdentityExportsTestFixture()
{}

void GameKitIdentityExportsTestFixture::SetUp()
{
    ::testing::internal::CaptureStdout();

    testStack.Initialize();
}

void GameKitIdentityExportsTestFixture::TearDown()
{
    std::string capturedStdout = ::testing::internal::GetCapturedStdout().c_str();

    testStack.Cleanup();
}

void* GameKitIdentityExportsTestFixture::createIdentityInstance()
{
    void* sessMgr = GameKitSessionManagerInstanceCreate("../core/test_data/sampleplugin/instance/testgame/dev/awsGameKitClientConfig.yml", TestLogger::Log);
    (static_cast<GameKit::Authentication::GameKitSessionManager*>(sessMgr))->SetToken(TokenType::AccessToken, "test_token");
    (static_cast<GameKit::Authentication::GameKitSessionManager*>(sessMgr))->SetToken(TokenType::IdToken, "test_token");

    return GameKitIdentityInstanceCreateWithSessionManager(sessMgr, TestLogger::Log);
}

void* GameKitIdentityExportsTestFixture::createIdentityInstanceWithNoSessionManagerTokens()
{
    void* sessMgr = GameKitSessionManagerInstanceCreate("../core/test_data/sampleplugin/instance/testgame/dev/awsGameKitClientConfig.yml", TestLogger::Log);

    return GameKitIdentityInstanceCreateWithSessionManager(sessMgr, TestLogger::Log);
}

void GameKitIdentityExportsTestFixture::setIdentityMocks(void* instance)
{
    GameKit::Identity::Identity* identityInstance = static_cast<GameKit::Identity::Identity*>(instance);

    cognitoMock = Aws::MakeUnique<GameKit::Mocks::MockCognitoIdentityProviderClient>("cognitoMock");

    mockHttpClient = std::make_shared<MockHttpClient>();
    identityInstance->SetHttpClient(mockHttpClient);

    identityInstance->SetCognitoClient(cognitoMock.get());
}

std::string GameKitIdentityExportsTestFixture::GetCognitoGetUserApiResponse()
{
    return "{\"MFAOptions\":[{\"AttributeName\":\"string\",\"DeliveryMedium\":\"string\"}],\"PreferredMfaSetting\":\"string\",\"UserAttributes\":[{\"Name\":\"email\",\"Value\":\"playerone@test.com\"}],\"UserMFASettingList\":[\"string\"],\"Username\":\"playerone\"}";
}

std::string GameKitIdentityExportsTestFixture::GetIdentityLambdaGetUserApiResponse()
{
    return "{\"data\":{\"updated_at\":\"2021-12-28T01:51:50.647341+00:00\",\"created_at\":\"2021-12-28T01:51:32.165258+00:00\",\"gk_user_id\":\"4f1de70d-c130-444d-af78-000000\",\"facebook_external_id\":\"\",\"facebook_ref_id\":\"\",\"user_name\":\"playerone\"}}";
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityInstanceCreate_Success)
{
    // act
    GameKit::GameKitFeature* identityInstance = static_cast<GameKit::GameKitFeature*>(createIdentityInstance());

    // assert
    ASSERT_NE(identityInstance, nullptr);

    GameKitIdentityInstanceRelease(identityInstance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityRegister_Success)
{
    // arrange
    GameKit::UserRegistration registration = { TEST_USERNAME, TEST_PASSWORD, TEST_EMAIL };

    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), SignUp(_))
        .Times(1)
        .WillOnce(Return(SuccessOutcome<SignUpResult, SignUpOutcome>()));

    // act
    unsigned int result = GameKitIdentityRegister(instance, registration);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityRegisterBadUsername_Failure)
{
    // arrange
    GameKit::UserRegistration registration = { INVALID_USERNAME, TEST_PASSWORD, TEST_EMAIL };

    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), SignUp(_)).Times(0);

    // act
    unsigned int result = GameKitIdentityRegister(instance, registration);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_ERROR_MALFORMED_USERNAME, result);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityRegisterBadPassword_Failure)
{
    // arrange
    GameKit::UserRegistration registration = { TEST_USERNAME, INVALID_PASSWORD, TEST_EMAIL };

    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), SignUp(_)).Times(0);

    // act
    unsigned int result = GameKitIdentityRegister(instance, registration);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_ERROR_MALFORMED_PASSWORD, result);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityConfirmRegistration_Success)
{
    // arrange
    GameKit::ConfirmRegistrationRequest request = { TEST_USERNAME, TEST_CONFIRMATION_CODE };

    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), ConfirmSignUp(_))
        .Times(1)
        .WillOnce(Return(SuccessOutcome<ConfirmSignUpResult, ConfirmSignUpOutcome>()));

    // act
    unsigned int result = GameKitIdentityConfirmRegistration(instance, request);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityConfirmRegistrationBadUsername_Failure)
{
    // arrange
    GameKit::ConfirmRegistrationRequest request = { INVALID_USERNAME, TEST_CONFIRMATION_CODE };

    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), ConfirmSignUp(_)).Times(0);

    // act
    unsigned int result = GameKitIdentityConfirmRegistration(instance, request);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_ERROR_MALFORMED_USERNAME, result);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityResendConfirmationCode_Success)
{
    // arrange
    GameKit::ResendConfirmationCodeRequest request = { TEST_USERNAME };

    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), ResendConfirmationCode(_))
        .Times(1)
        .WillOnce(Return(SuccessOutcome<ResendConfirmationCodeResult, ResendConfirmationCodeOutcome>()));

    // act
    unsigned int result = GameKitIdentityResendConfirmationCode(instance, request);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityResendConfirmationCodeBadUsername_Failure)
{
    // arrange
    GameKit::ResendConfirmationCodeRequest request = { INVALID_USERNAME };

    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), ResendConfirmationCode(_)).Times(0);

    // act
    unsigned int result = GameKitIdentityResendConfirmationCode(instance, request);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_ERROR_MALFORMED_USERNAME, result);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityLogin_Success)
{
    // arrange
    GameKit::UserLogin login = { TEST_USERNAME, TEST_PASSWORD };

    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), InitiateAuth(_))
        .Times(1)
        .WillOnce(Return(SuccessOutcome<InitiateAuthResult, InitiateAuthOutcome>()));

    // act
    unsigned int result = GameKitIdentityLogin(instance, login);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityLoginBadUsername_Failure)
{
    // arrange
    GameKit::UserLogin login = { INVALID_USERNAME, TEST_PASSWORD };

    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), InitiateAuth(_)).Times(0);

    // act
    unsigned int result = GameKitIdentityLogin(instance, login);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_ERROR_MALFORMED_USERNAME, result);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityLoginBadPassword_Failure)
{
    // arrange
    GameKit::UserLogin login = { TEST_USERNAME, INVALID_PASSWORD };

    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), InitiateAuth(_)).Times(0);

    // act
    unsigned int result = GameKitIdentityLogin(instance, login);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_ERROR_MALFORMED_PASSWORD, result);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityLoginTwiceRevokeOldToken_Success)
{
    // arrange
    GameKit::UserLogin login = { TEST_USERNAME, TEST_PASSWORD };

    void* instance = createIdentityInstance();
    GameKit::Identity::Identity* identityInstance = static_cast<GameKit::Identity::Identity*>(instance);
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), InitiateAuth(_))
        .Times(2)
        .WillRepeatedly(Return(SuccessOutcome<InitiateAuthResult, InitiateAuthOutcome>()));

    EXPECT_CALL(*cognitoMock.get(), RevokeToken(_))
        .Times(1)
        .WillOnce(Return(SuccessOutcome<RevokeTokenResult, RevokeTokenOutcome>()));

    // act
    unsigned int resultLoginOne = GameKitIdentityLogin(instance, login);

    identityInstance->GetSessionManager()->SetToken(GameKit::TokenType::RefreshToken, "tokenvalue");
    identityInstance->GetSessionManager()->SetToken(GameKit::TokenType::AccessToken, "accesstokenvalue");

    unsigned int resultLoginTwo = GameKitIdentityLogin(instance, login);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, resultLoginOne);
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, resultLoginTwo);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityLogout_Success)
{
    // arrange
    void* instance = createIdentityInstance();
    GameKit::Identity::Identity* identityInstance = static_cast<GameKit::Identity::Identity*>(instance);
    identityInstance->GetSessionManager()->SetToken(GameKit::TokenType::RefreshToken, "tokenvalue");
    identityInstance->GetSessionManager()->SetToken(GameKit::TokenType::AccessToken, "accesstokenvalue");
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), RevokeToken(_))
        .Times(1)
        .WillOnce(Return(SuccessOutcome<RevokeTokenResult, RevokeTokenOutcome>()));

    // act
    unsigned int result = GameKitIdentityLogout(instance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    EXPECT_STREQ("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::AccessToken).c_str());
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityLogout_NotLoggedIn)
{
    // arrange
    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    // act
    unsigned int result = GameKitIdentityLogout(instance);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_ERROR_LOGIN_FAILED, result);

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityLogout_CanLoginAfter)
{
    // arrange
    void* instance = createIdentityInstance();
    GameKit::Identity::Identity* identityInstance = static_cast<GameKit::Identity::Identity*>(instance);
    identityInstance->GetSessionManager()->SetToken(GameKit::TokenType::RefreshToken, "tokenvalue");
    identityInstance->GetSessionManager()->SetToken(GameKit::TokenType::AccessToken, "accesstokenvalue");
    GameKit::UserLogin login = { TEST_USERNAME, TEST_PASSWORD };
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), RevokeToken(_))
        .Times(1)
        .WillOnce(Return(SuccessOutcome<RevokeTokenResult, RevokeTokenOutcome>()));

    EXPECT_CALL(*cognitoMock.get(), InitiateAuth(_))
        .Times(1)
        .WillOnce(Return(SuccessOutcome<InitiateAuthResult, InitiateAuthOutcome>()));

    // act
    unsigned int result = GameKitIdentityLogout(instance);
    unsigned int loginResult = GameKitIdentityLogin(instance, login);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, loginResult);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}
TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityForgotPassword_Success)
{
    // arrange
    GameKit::ForgotPasswordRequest request = { TEST_USERNAME };

    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), ForgotPassword(_))
        .Times(1)
        .WillOnce(Return(SuccessOutcome<ForgotPasswordResult, ForgotPasswordOutcome>()));

    // act
    unsigned int result = GameKitIdentityForgotPassword(instance, request);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityForgotPasswordBadUsername_Failure)
{
    // arrange
    GameKit::ForgotPasswordRequest request = { INVALID_USERNAME };

    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), ForgotPassword(_)).Times(0);

    // act
    unsigned int result = GameKitIdentityForgotPassword(instance, request);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_ERROR_MALFORMED_USERNAME, result);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityConfirmForgotPassword_Success)
{
    // arrange
    GameKit::ConfirmForgotPasswordRequest request = { TEST_USERNAME, TEST_PASSWORD, TEST_CONFIRMATION_CODE };

    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), ConfirmForgotPassword(_))
        .Times(1)
        .WillOnce(Return(SuccessOutcome<ConfirmForgotPasswordResult, ConfirmForgotPasswordOutcome>()));

    // act
    unsigned int result = GameKitIdentityConfirmForgotPassword(instance, request);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_SUCCESS, result);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityConfirmForgotPasswordBadUsername_Failure)
{
    // arrange
    GameKit::ConfirmForgotPasswordRequest request = { INVALID_USERNAME, TEST_PASSWORD, TEST_CONFIRMATION_CODE };

    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), ConfirmForgotPassword(_)).Times(0);

    // act
    unsigned int result = GameKitIdentityConfirmForgotPassword(instance, request);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_ERROR_MALFORMED_USERNAME, result);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityConfirmForgotPasswordBadPassword_Failure)
{
    // arrange
    GameKit::ConfirmForgotPasswordRequest request = { TEST_USERNAME, INVALID_PASSWORD, TEST_CONFIRMATION_CODE };

    void* instance = createIdentityInstance();
    setIdentityMocks(instance);

    EXPECT_CALL(*cognitoMock.get(), ConfirmForgotPassword(_)).Times(0);

    // act
    unsigned int result = GameKitIdentityConfirmForgotPassword(instance, request);

    // assert
    ASSERT_EQ(GameKit::GAMEKIT_ERROR_MALFORMED_PASSWORD, result);
    void* mock = cognitoMock.get();
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(mock));

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityGetUser_Success)
{
    // arrange
    void* instance = createIdentityInstance();
    setIdentityMocks(instance);
    auto identityInstance = static_cast<GameKit::Identity::Identity*>(instance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(200));
    response->SetResponseBody(GetIdentityLambdaGetUserApiResponse());

    std::string cognitoResponseJson = GetCognitoGetUserApiResponse();
    const Aws::String input(cognitoResponseJson.c_str(), cognitoResponseJson.size());
    const Aws::Utils::Json::JsonValue cognitoGetUserJsonValue(input);
    const Aws::Http::HeaderValueCollection headers;
    Aws::AmazonWebServiceResult<Aws::Utils::Json::JsonValue> awsResult(cognitoGetUserJsonValue, headers);

    GetUserResult userResult(awsResult);
    GetUserOutcome outcome = GetUserOutcome(userResult);

    EXPECT_CALL(*this->mockHttpClient.get(), MakeRequest(_, _, _)).WillOnce(Return(response));
    EXPECT_CALL(*this->cognitoMock.get(), GetUser(_)).WillOnce(Return(outcome));

    // act
    GameKit::Tests::IdentityExports::Dispatcher dispatcher = GameKit::Tests::IdentityExports::Dispatcher();
    int result = GameKitIdentityGetUser(instance, dispatcher.get(), responseCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);
    ASSERT_STREQ(dispatcher.email.c_str(), "playerone@test.com");
    ASSERT_STREQ(dispatcher.userName.c_str(), "playerone");
    ASSERT_STREQ(dispatcher.userId.c_str(), "4f1de70d-c130-444d-af78-000000");

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(this->mockHttpClient.get()));
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(this->cognitoMock.get()));
    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityGetUser_API_Fail)
{
    // arrange
    void* instance = createIdentityInstance();
    setIdentityMocks(instance);
    auto identityInstance = static_cast<GameKit::Identity::Identity*>(instance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(500));

    EXPECT_CALL(*this->mockHttpClient.get(), MakeRequest(_, _, _)).WillOnce(Return(response));

    // act
    GameKit::Tests::IdentityExports::Dispatcher dispatcher = GameKit::Tests::IdentityExports::Dispatcher();
    int result = GameKitIdentityGetUser(instance, dispatcher.get(), responseCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_HTTP_REQUEST_FAILED);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(this->mockHttpClient.get()));
    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityGetUser_InvalidJson_Fail)
{
    // arrange
    void* instance = createIdentityInstance();
    setIdentityMocks(instance);
    auto identityInstance = static_cast<GameKit::Identity::Identity*>(instance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(200));
    response->SetResponseBody("{]");

    EXPECT_CALL(*this->mockHttpClient.get(), MakeRequest(_, _, _)).WillOnce(Return(response));

    // act
    GameKit::Tests::IdentityExports::Dispatcher dispatcher = GameKit::Tests::IdentityExports::Dispatcher();
    int result = GameKitIdentityGetUser(instance, dispatcher.get(), responseCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_PARSE_JSON_FAILED);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(this->mockHttpClient.get()));
    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityGetUser_MissingUserId_Fail)
{
    // arrange
    void* instance = createIdentityInstance();
    setIdentityMocks(instance);
    auto identityInstance = static_cast<GameKit::Identity::Identity*>(instance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(200));
    response->SetResponseBody("{\"test\":\"123\"}");

    EXPECT_CALL(*this->mockHttpClient.get(), MakeRequest(_, _, _)).WillOnce(Return(response));

    // act
    GameKit::Tests::IdentityExports::Dispatcher dispatcher = GameKit::Tests::IdentityExports::Dispatcher();
    int result = GameKitIdentityGetUser(instance, dispatcher.get(), responseCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_PARSE_JSON_FAILED);

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(this->mockHttpClient.get()));
    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityGetUser_UserNotLoggedIn_Fail)
{
    // arrange
    void* instance = createIdentityInstanceWithNoSessionManagerTokens();
    setIdentityMocks(instance);
    auto identityInstance = static_cast<GameKit::Identity::Identity*>(instance);

    // act
    GameKit::Tests::IdentityExports::Dispatcher dispatcher = GameKit::Tests::IdentityExports::Dispatcher();
    int result = GameKitIdentityGetUser(instance, dispatcher.get(), responseCallback);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_NO_ID_TOKEN);

    GameKitIdentityInstanceRelease(instance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityInstanceRelease_Success)
{
    // arrange
    void* identityInstance = createIdentityInstance();

    // act
    GameKitIdentityInstanceRelease(identityInstance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityInstanceRelease_SessionManager_Persists)
{
    // arrange
    void* sessMgr = GameKitSessionManagerInstanceCreate("../core/test_data/sampleplugin/instance/testgame/dev/awsGameKitClientConfig.yml", TestLogger::Log);

    void* identityInstance = GameKitIdentityInstanceCreateWithSessionManager(sessMgr, TestLogger::Log);

    // act
    GameKitIdentityInstanceRelease(identityInstance);

    // assert
    ASSERT_NE(nullptr, sessMgr);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityGetFbLoginUrl_Success)
{
    // arrange
    void* sessMgr = GameKitSessionManagerInstanceCreate("../core/test_data/sampleplugin/instance/testgame/dev/awsGameKitClientConfig.yml", TestLogger::Log);
    void* identityInstance = GameKitIdentityInstanceCreateWithSessionManager(sessMgr, TestLogger::Log);

    // TODO:: Mock HttpClient
    //// act
    //auto resp = GameKitGetFederatedLoginUrl(identityInstance, FederatedIdentityProvider::Facebook);

    //// assert
    //ASSERT_NE(nullptr, resp.loginUrl);

    GameKitIdentityInstanceRelease(identityInstance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityGetFbPollAndRetriveTokens_TokensRetrieved_Success)
{
    // arrange
    void* instance = createIdentityInstanceWithNoSessionManagerTokens();
    setIdentityMocks(instance);
    GameKit::Identity::Identity* identityInstance = static_cast<GameKit::Identity::Identity*>(instance);

    std::shared_ptr<FakeHttpResponse> pollForCompletionResponse = std::make_shared<FakeHttpResponse>();
    pollForCompletionResponse->SetResponseCode(Aws::Http::HttpResponseCode(200));
    pollForCompletionResponse->SetResponseBody("S3_file_location");

    std::shared_ptr<FakeHttpResponse> retrieveTokensResponse = std::make_shared<FakeHttpResponse>();
    retrieveTokensResponse->SetResponseCode(Aws::Http::HttpResponseCode(200));
    retrieveTokensResponse->SetResponseBody("{\"access_token\":\"fb_access_token\", \"refresh_token\":\"fb_refresh_token\", \"id_token\":\"fb_id_token\",\"expires_in\":3600,\"token_type\":\"Bearer\",\"source_ip\":\"24.22.162.62\"}");

    EXPECT_CALL(*this->mockHttpClient.get(), MakeRequest(_, _, _)).WillOnce(Return(pollForCompletionResponse)).WillOnce(Return(retrieveTokensResponse));

    // act
    unsigned int result = GameKitPollAndRetrieveFederatedTokens(identityInstance, GameKit::FederatedIdentityProvider::Facebook, "41669940-4b22-49b5-8a59-84c596455058", 60);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);
    ASSERT_NE("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::AccessToken));
    ASSERT_NE("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::RefreshToken));
    ASSERT_NE("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::IdToken));
    
    ASSERT_TRUE(Mock::VerifyAndClearExpectations(this->mockHttpClient.get()));
    GameKitIdentityInstanceRelease(identityInstance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityGetFbPollAndRetriveTokens_HttpResponseBodyOfFbtokensContainsRetrieved_Success)
{
    // arrange
    void* instance = createIdentityInstance();
    setIdentityMocks(instance);
    GameKit::Identity::Identity* identityInstance = static_cast<GameKit::Identity::Identity*>(instance);
    identityInstance->GetSessionManager()->SetToken(GameKit::TokenType::RefreshToken, "refresh_token");

    std::shared_ptr<FakeHttpResponse> pollForCompletionResponse = std::make_shared<FakeHttpResponse>();
    pollForCompletionResponse->SetResponseCode(Aws::Http::HttpResponseCode(200));
    pollForCompletionResponse->SetResponseBody("S3_file_location");

    std::shared_ptr<FakeHttpResponse> retrieveTokensResponse = std::make_shared<FakeHttpResponse>();
    retrieveTokensResponse->SetResponseCode(Aws::Http::HttpResponseCode(200));
    retrieveTokensResponse->SetResponseBody("Retrieved");

    EXPECT_CALL(*this->mockHttpClient.get(), MakeRequest(_, _, _)).WillOnce(Return(pollForCompletionResponse)).WillOnce(Return(retrieveTokensResponse));

    // act
    unsigned int result = GameKitPollAndRetrieveFederatedTokens(identityInstance, GameKit::FederatedIdentityProvider::Facebook, "41669940-4b22-49b5-8a59-84c596455058", 60);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);
    ASSERT_NE("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::AccessToken));
    ASSERT_NE("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::RefreshToken));
    ASSERT_NE("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::IdToken));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(this->mockHttpClient.get()));
    GameKitIdentityInstanceRelease(identityInstance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityGetFbPollAndRetriveTokens_HttpResponseBodyOfFblogincheckContainsRetrieved_Success)
{
    // arrange
    void* instance = createIdentityInstance();
    setIdentityMocks(instance);
    GameKit::Identity::Identity* identityInstance = static_cast<GameKit::Identity::Identity*>(instance);
    identityInstance->GetSessionManager()->SetToken(GameKit::TokenType::RefreshToken, "refresh_token");

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode(200));
    response->SetResponseBody("Retrieved");

    EXPECT_CALL(*this->mockHttpClient.get(), MakeRequest(_, _, _)).WillOnce(Return(response));

    // act
    unsigned int result = GameKitPollAndRetrieveFederatedTokens(identityInstance, GameKit::FederatedIdentityProvider::Facebook, "41669940-4b22-49b5-8a59-84c596455058", 60);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_SUCCESS);
    ASSERT_NE("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::AccessToken));
    ASSERT_NE("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::RefreshToken));
    ASSERT_NE("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::IdToken));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(this->mockHttpClient.get()));
    GameKitIdentityInstanceRelease(identityInstance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityGetFbPollAndRetriveTokens_RequestTimedOut_Fail)
{
    // arrange
    void* instance = createIdentityInstanceWithNoSessionManagerTokens();
    setIdentityMocks(instance);
    GameKit::Identity::Identity* identityInstance = static_cast<GameKit::Identity::Identity*>(instance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode::NOT_FOUND);

    EXPECT_CALL(*this->mockHttpClient.get(), MakeRequest(_, _, _)).WillRepeatedly(Return(response));

    // act
    unsigned int result = GameKitPollAndRetrieveFederatedTokens(identityInstance, GameKit::FederatedIdentityProvider::Facebook, "41669940-4b22-49b5-8a59-84c596455058", 6);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_REQUEST_TIMED_OUT);
    ASSERT_EQ("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::AccessToken));
    ASSERT_EQ("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::RefreshToken));
    ASSERT_EQ("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::IdToken));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(this->mockHttpClient.get()));
    GameKitIdentityInstanceRelease(identityInstance);
}

TEST_F(GameKitIdentityExportsTestFixture, TestGameKitIdentityGetFbPollAndRetriveTokens_FacebookIsNotDeployed_Fail)
{
    // arrange
    void* instance = createIdentityInstanceWithNoSessionManagerTokens();
    setIdentityMocks(instance);
    GameKit::Identity::Identity* identityInstance = static_cast<GameKit::Identity::Identity*>(instance);

    std::shared_ptr<FakeHttpResponse> response = std::make_shared<FakeHttpResponse>();
    response->SetResponseCode(Aws::Http::HttpResponseCode::FORBIDDEN);

    EXPECT_CALL(*this->mockHttpClient.get(), MakeRequest(_, _, _)).WillOnce(Return(response));

    // act
    unsigned int result = GameKitPollAndRetrieveFederatedTokens(identityInstance, GameKit::FederatedIdentityProvider::Facebook, "41669940-4b22-49b5-8a59-84c596455058", 6);

    // assert
    ASSERT_EQ(result, GameKit::GAMEKIT_ERROR_HTTP_REQUEST_FAILED);
    ASSERT_EQ("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::AccessToken));
    ASSERT_EQ("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::RefreshToken));
    ASSERT_EQ("", identityInstance->GetSessionManager()->GetToken(GameKit::TokenType::IdToken));

    ASSERT_TRUE(Mock::VerifyAndClearExpectations(this->mockHttpClient.get()));
    GameKitIdentityInstanceRelease(identityInstance);
}
