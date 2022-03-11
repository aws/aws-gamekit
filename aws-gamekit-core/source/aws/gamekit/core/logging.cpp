// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include <aws/gamekit/core/logging.h>

using namespace GameKit::Logger;

#define CONTEXT_MARK_START "["
#define CONTEXT_MARK_END "]~ "

#pragma region Public Methods
void Logging::Log(FuncLogCallback cb, Level level, const char* message)
{
    if (cb != nullptr)
    {
        std::thread::id threadId = std::this_thread::get_id();
        std::stringstream buffer;
        buffer << CONTEXT_MARK_START << "@" << threadId << CONTEXT_MARK_END << message;
        cb((unsigned int) level, buffer.str().c_str(), buffer.str().size());
    }
}

void Logging::Log(FuncLogCallback cb, Level level, const char* message, const void* context)
{
    if (cb != nullptr)
    {
        std::thread::id threadId = std::this_thread::get_id();
        std::stringstream buffer;
        buffer << CONTEXT_MARK_START << context << "@" << threadId << CONTEXT_MARK_END << message;
        cb((unsigned int) level, buffer.str().c_str(), buffer.str().size());
    }
}
#pragma endregion
