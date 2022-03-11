// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// GameKit
#include <aws/gamekit/core/api.h>

namespace GameKit
{
    // Wrapper for public-domain miniz ZIP writer - see miniz.inc
    // NOTE: filenames and paths use UTF-8 encoding on all platforms
    class GAMEKIT_API Zipper
    {
    private:
        void* m_zipFile;
        std::string m_sourcePath;

    public:
        Zipper(const std::string& sourcePath, const std::string& zipFileName);
        ~Zipper();

        bool AddDirectoryToZipFile(const std::string& directoryPath);
        bool AddFileToZipFile(const std::string& fileName);
        bool CloseZipFile();
        static void NormalizePathInZip(std::string& inOutPathInZip, const std::string& relativeSoucePath);
    };
}
