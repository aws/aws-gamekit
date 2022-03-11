// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <string>
#include <sstream>
#include <thread>

// GameKit
#include "api.h"

extern "C"
{
    typedef void(*FuncLogCallback)(unsigned int level, const char* message, int size);
}

namespace GameKit
{
    namespace Logger
    {
        enum class Level
        {
            None = 0,
            Verbose = 1,
            Info = 2,
            Warning = 3,
            Error = 4
        };

        class GAMEKIT_API Logging
        {
        public:
            static void Log(FuncLogCallback cb, Level level, const char* message);
            static void Log(FuncLogCallback cb, Level level, const char* message, const void* context);
        };
    }
}
