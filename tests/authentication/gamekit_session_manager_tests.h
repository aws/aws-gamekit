#pragma once
#pragma once
#include <aws/core/utils/memory/AWSMemory.h>
#include <gtest/gtest.h>
#include <aws/gamekit/authentication/gamekit_session_manager.h>

#include <gtest/gtest.h>

namespace GameKit
{
    namespace Tests
    {
        namespace GameKitSessionManager
        {
            Aws::UniquePtr<GameKit::Authentication::GameKitSessionManager> gamekitSessionManagerInstance = nullptr;

            class GameKitSessionManagerTestFixture;
        }
    }
}
