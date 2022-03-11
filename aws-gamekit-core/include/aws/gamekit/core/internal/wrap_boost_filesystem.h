// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

//
// In GameKit source code, we never include any boost::filesystem headers directly.
// The default behavior of boost::filesystem::path is to treat std::string filenames
// as ANSI, even though GameKit itself has standardized on "UTF-8 everywhere", and
// this mismatch will cause file operations to fail when handed UTF-8 filenames.
//
// Instead of including boost::filesystem headers directly, GameKit source files
// should include this wrapper, which registers a module-level startup function that
// reconfigures boost::filesystem to always treat std::string filenames as UTF-8.
//
// Including this wrapper from a public GameKit header would be bad, though, as it
// would cause any project which includes this wrapper to reconfigure boost's global
// behavior. So don't! Only include this wrapper from GameKit C++ source files.
//
// GameKit headers should use std::string for paths, not boost::filesystem::path.
// If you absolutely must refer to boost::filesystem::path from a GameKit header,
// forward-declare it like so, and always pass it via const reference:
//
//   namespace boost { namespace filesystem { class path; } }
//

#include <aws/gamekit/core/api.h>

#if defined(_WIN32) || defined(ANDROID) || defined(__ANDROID__)
#include <boost/filesystem.hpp>
#endif

namespace GameKitInternal
{
#ifdef _WIN32
    class BoostFilesystemUtf8Initializer
    {
    private:
        static GAMEKIT_API std::locale GetLocale();
    public:
        BoostFilesystemUtf8Initializer()
        {
            // This call must remain in the header to ensure that it references
            // global boost state in the current module, not always aws-gamekit-core
            boost::filesystem::path::imbue(GetLocale());
        }
    };

    // This weak declaration can be safely included in every file within a project
    // and the linker will quietly discard duplicates instead of generating an error.
    __declspec(selectany) BoostFilesystemUtf8Initializer BoostFilesystemUtf8Init;
#endif
}
