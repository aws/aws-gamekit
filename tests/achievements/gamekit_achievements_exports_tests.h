#pragma once

#include "../core/test_common.h"
#include "../core/mocks/fake_http_client.h"
#include "aws/gamekit/achievements/gamekit_achievements.h"
#include "aws/gamekit/achievements/exports.h"
#include "aws/gamekit/authentication/exports.h"
#include "../core/test_stack.h"
#include "../core/test_log.h"

namespace GameKit
{
    namespace Tests
    {
        namespace AchievementsExports
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

            class GameKitAchievementsExportsTestFixture : public ::testing::Test
            {
            public:
                GameKitAchievementsExportsTestFixture();
                ~GameKitAchievementsExportsTestFixture();
                virtual void SetUp() override;
                virtual void TearDown() override;

            protected:
                void* createAdminAchievementsInstance(bool setToken);
                GAMEKIT_SESSIONMANAGER_INSTANCE_HANDLE testSessionManager = nullptr;
                void setAchievementsMocks(void* instance);
                std::shared_ptr<MockHttpClient> mockHttpClient;

                template <typename Result, typename Outcome>
                Outcome SuccessOutcome()
                {
                    Result result;
                    Outcome outcome(result);
                    return outcome;
                }

                TestStackInitializer testStackInitializer;
                typedef TestLog<GameKitAchievementsExportsTestFixture> TestLogger;
            };
        }
    }
}
