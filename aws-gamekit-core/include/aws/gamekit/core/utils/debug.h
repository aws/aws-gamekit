// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/**
* Define GameKitInternalAssert helper macro. This is a wrapper for C++ assert() that is only compiled in
* debug builds. No-op in Release builds.
*/

#if defined(_DEBUG) || defined(GAMEKIT_DEBUG)
#include <assert.h>

#define GameKitInternalAssert(value)    assert(value)

#else

#define GameKitInternalAssert(value)    (void)(value)

#endif