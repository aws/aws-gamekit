#pragma once

#include "../core/test_common.h"
#include "../core/mocks/fake_http_client.h"
#include "aws/gamekit/achievements/gamekit_admin_achievements.h"
#include "aws/gamekit/achievements/exports_admin.h"
#include "../core/test_stack.h"
#include "../core/test_log.h"

namespace GameKit
{
    namespace Tests
    {
        namespace AdminAchievementsExports
        {
            class Dispatcher
            {
            public:
                Dispatcher() = default;
                DISPATCH_RECEIVER_HANDLE get()
                {
                    return this;
                }
                std::string message;
                void CallbackHandler(const char* message);
            };

            class GameKitAdminAchievementsExportsTestFixture : public ::testing::Test
            {
            public:
                GameKitAdminAchievementsExportsTestFixture();
                ~GameKitAdminAchievementsExportsTestFixture();
                virtual void SetUp() override;
                virtual void TearDown() override;

            protected:
                static const std::string MOCK_ACCESS_ID;
                static const std::string MOCK_ACCESS_SECRET;
                static const std::string MOCK_SESSION_TOKEN;

                void* createAdminAchievementsInstance(bool setToken);
                GAMEKIT_SESSIONMANAGER_INSTANCE_HANDLE testSessionManager = nullptr;
                void setAchievementsMocks(void* instance);
                void setAchievementsAdminCredentials(GameKit::Achievements::AdminAchievements* achievementsInstance);
                std::shared_ptr<MockHttpClient> mockHttpClient;
                AccountCredentials mockAccountCredentials;
                AccountInfo mockAccountInfo;
                Aws::STS::Model::Credentials mockSessionCredentials;

                template <typename Result, typename Outcome>
                Outcome SuccessOutcome()
                {
                    Result result;
                    Outcome outcome(result);
                    return outcome;
                }

                TestStackInitializer testStackInitializer;
                typedef TestLog<GameKitAdminAchievementsExportsTestFixture> TestLogger;
            };
        }
    }
}
