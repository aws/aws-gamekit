// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// Standard library
#include <sys/stat.h>

// GameKit
#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>
#include <aws/gamekit/core/utils/file_utils.h>
#include <aws/gamekit/core/zipper.h>

// Boost
#include <boost/filesystem.hpp>

// Include single-file miniz library as header only
// (implementation is included again at end of file)
#define MINIZ_CUSTOM_FOPEN_FREOPEN_STAT
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#define MINIZ_HEADER_FILE_ONLY
#include "miniz.inc"
#undef MINIZ_HEADER_FILE_ONLY

using namespace GameKit;

#pragma region Constructors/Destructor
Zipper::Zipper(const std::string& sourcePath, const std::string& zipFileName)
{
    m_sourcePath = sourcePath;

    m_zipFile = new mz_zip_archive{};
    if (!mz_zip_writer_init_file((mz_zip_archive*)m_zipFile, zipFileName.c_str(), 0))
    {
        delete (mz_zip_archive*)m_zipFile;
        m_zipFile = nullptr;
    }
}

Zipper::~Zipper()
{
    CloseZipFile();
}
#pragma endregion

#pragma region Public Methods
bool Zipper::AddDirectoryToZipFile(const std::string& directoryPath)
{
    if (m_zipFile == nullptr)
    {
        return false;
    }

    const boost::filesystem::path dp(Utils::FileUtils::PathFromUtf8(directoryPath));
    if (!boost::filesystem::is_directory(dp))
    {
        return false;
    }

    boost::filesystem::recursive_directory_iterator endIterator;
    for (boost::filesystem::recursive_directory_iterator dirIterator(dp); dirIterator != endIterator; ++dirIterator)
    {
        const boost::filesystem::path cp = (*dirIterator); // Note: cp is a full path
        // Do not explicitly add directories to the zip file, directories are implied by slashes
        if (!boost::filesystem::is_directory(cp))
        {
            // Add to zip file; path will become relative to root used to create Zipper instance
            if (!AddFileToZipFile(Utils::FileUtils::PathToUtf8(cp.native())))
            {
                return false;
            }
        }
    }

    return true;
}

bool Zipper::AddFileToZipFile(const std::string& filePath)
{
    if (m_zipFile == nullptr)
    {
        return false;
    }

    // Compute relative path and store that as filename inside the zip
    std::string pathInZip = filePath;
    NormalizePathInZip(pathInZip, m_sourcePath);

    if (pathInZip.length() >= MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE)
    {
        return false;
    }

    return mz_zip_writer_add_file((mz_zip_archive*)m_zipFile, pathInZip.c_str(), filePath.c_str(), nullptr, 0, MZ_DEFAULT_COMPRESSION);
}

bool Zipper::CloseZipFile()
{
    if (m_zipFile == nullptr)
    {
        return false;
    }

    // Always call both finalize and end, return false if either call fails
    bool finalized = mz_zip_writer_finalize_archive((mz_zip_archive*)m_zipFile);
    bool finished = mz_zip_writer_end((mz_zip_archive*)m_zipFile);

    delete (mz_zip_archive*)m_zipFile;
    m_zipFile = nullptr;

    return finalized && finished;
}

// Determine relative path to be stored internally. On Windows, replace the preferred path "\" with "/"
void Zipper::NormalizePathInZip(std::string& inOutPathInZip, const std::string& relativeSourcePath)
{
    const boost::filesystem::path filePath(Utils::FileUtils::PathFromUtf8(inOutPathInZip));
    const boost::filesystem::path rootPath(Utils::FileUtils::PathFromUtf8(relativeSourcePath));

    // This assigns wstring L"..\" on Windows, string "../" on other platforms
    const auto dotDotSlash = boost::filesystem::path("../").make_preferred().native();

    boost::filesystem::path relativePath = filePath.lexically_relative(rootPath);

    // If lexically_relative cannot find any relationship, or if the relationship begins by
    // moving up a folder with '..' because we only have a partial match on the source path,
    // then we stick to the original filename - but stripped of any absolute path root.
    if (relativePath.empty() || relativePath.native().compare(0, dotDotSlash.length(), dotDotSlash) == 0)
        relativePath = filePath.lexically_normal().relative_path();

    inOutPathInZip = Utils::FileUtils::PathToUtf8(relativePath.native());

#ifdef _WIN32
    // Convert platform path to internal zip file path, which always uses forward slashes.
    // We know _WIN32 uses backslashes; in-place character swap is simpler than generic replacement
    std::replace(inOutPathInZip.begin(), inOutPathInZip.end(), '\\', '/');
    static_assert(boost::filesystem::path::preferred_separator == L'\\', "bad Win32 path separator");
#else
    static_assert(boost::filesystem::path::preferred_separator == L'/', "unexpected path separator");
#endif

    // Internal zip paths should never go above the root; this can happen if filePath starts with '..'
    while (inOutPathInZip.compare(0, 3, "../") == 0 || inOutPathInZip == "..")
        inOutPathInZip.erase(0, 3);

    // Preserving old edge-case behavior: if zip file path would be '.', return empty string instead
    if (inOutPathInZip == ".")
        inOutPathInZip.clear();
}
#pragma endregion

#pragma region Implementation details for miniz
static FILE* mz_fopen(const char* pFilenameUtf8, const char* pMode)
{
#ifdef _WIN32
    // Win32 is the only platform where fopen cannot directly accept a UTF-8 filename
    std::wstring filename = GameKit::Utils::FileUtils::PathFromUtf8(pFilenameUtf8);
    std::wstring mode = GameKit::Utils::FileUtils::PathFromUtf8(pMode);
    return _wfopen(filename.c_str(), mode.c_str());
#else
    return fopen(pFilenameUtf8, pMode);
#endif
}

static FILE* mz_freopen(const char* pFilenameUtf8, const char* pMode, FILE* pStream)
{
#ifdef _WIN32
    // Win32 is the only platform where freopen cannot directly accept a UTF-8 filename
    std::wstring filename = GameKit::Utils::FileUtils::PathFromUtf8(pFilenameUtf8);
    std::wstring mode = GameKit::Utils::FileUtils::PathFromUtf8(pMode);
    return _wfreopen(filename.c_str(), mode.c_str(), pStream);
#else
    return freopen(pFilenameUtf8, pMode, pStream);
#endif
}

#ifdef _WIN32 // Note, signature changes on Win32: "struct _stat" vs "struct stat"
static int mz_stat(const char* pFilenameUtf8, struct _stat* pStat)
{
    // Win32 is the only platform where stat cannot directly accept a UTF-8 filename
    std::wstring filename = GameKit::Utils::FileUtils::PathFromUtf8(pFilenameUtf8);
    return _wstat(filename.c_str(), pStat);
}
#else
static int mz_stat(const char* pFilenameUtf8, struct stat* pStat)
{
    return stat(pFilenameUtf8, pStat);
}
#endif

#include "miniz.inc"

#pragma endregion
