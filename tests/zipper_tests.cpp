// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "zipper_tests.h"
#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>
#include <aws/gamekit/core/utils/file_utils.h>
#include <gmock/gmock-spec-builders.h>
#include <boost/filesystem.hpp>

class GameKit::Tests::GameKitZipper::GameKitZipperTestFixture : public ::testing::Test
{
public:
    GameKitZipperTestFixture()
    {}

    ~GameKitZipperTestFixture()
    {}

    void SetUp()
    {
        remove("../core/test_data/testFiles/zipperTests/testZip.zip");
        gamekitZipperInstance = Aws::MakeUnique<GameKit::Zipper>(
            "gamekitAccountInstance",
            "../core/test_data/testFiles/zipperTests",
            "../core/test_data/testFiles/zipperTests/testZip.zip");
    }

    void TearDown()
    {
        gamekitZipperInstance->CloseZipFile();
        gamekitZipperInstance.reset();
        remove("../core/test_data/testFiles/zipperTests/testZip.zip");
        TestExecutionUtils::AbortOnFailureIfEnabled();
    }
};

using namespace GameKit::Tests::GameKitZipper;
TEST_F(GameKitZipperTestFixture, FileExists_AddDirectoryToZip_True)
{
    auto result = gamekitZipperInstance->AddDirectoryToZipFile("../core/test_data/testFiles/zipperTests/testFiles");

    ASSERT_TRUE(result);

    std::ifstream fileExists("../core/test_data/testFiles/zipperTests/testZip.zip");

    ASSERT_TRUE(fileExists);
    fileExists.close();
}

TEST_F(GameKitZipperTestFixture, FileExists_AddFileToZip_True)
{
    auto result = gamekitZipperInstance->AddFileToZipFile("../core/test_data/testFiles/zipperTests/testFiles/intoZip2.txt");

    ASSERT_TRUE(result);

    std::ifstream fileExists("../core/test_data/testFiles/zipperTests/testZip.zip");

    ASSERT_TRUE(fileExists);
    fileExists.close();
}

TEST_F(GameKitZipperTestFixture, NoSuchFile_AddFileToZipFile_ReturnFalse)
{
    bool result = gamekitZipperInstance->AddFileToZipFile("../core/test_data/testFiles/zipperTests/testFiles/ThereIsNoFileWithThisName.txt");
    ASSERT_FALSE(result);
}

TEST_F(GameKitZipperTestFixture, NoSuchFile_AddDirectoryToZipFile_ReturnFalse)
{
    bool result = gamekitZipperInstance->AddDirectoryToZipFile("PathDoesNotExist/testFiles/zipperTests/testFiles");
    ASSERT_FALSE(result);
}

TEST_F(GameKitZipperTestFixture, PathExists_NormalizePath_ReturnNormalizedPath)
{
    std::string testPath = "../core/test_data/testFiles/zipperTests/testFiles/intoZip2.txt";
    std::string const relativePath = "../core";
    GameKit::Zipper::NormalizePathInZip(testPath,relativePath);

    ASSERT_EQ(testPath, "test_data/testFiles/zipperTests/testFiles/intoZip2.txt");
}

TEST_F(GameKitZipperTestFixture, NoPaths_NormalizePath_ReturnEmptyString)
{
    std::string testPath = "";
    std::string const relativePath = "";
    GameKit::Zipper::NormalizePathInZip(testPath, relativePath);

    ASSERT_EQ(testPath, "");
}

TEST_F(GameKitZipperTestFixture, LocalPathTotalEquality_NormalizePath_ReturnEmptyString)
{
    std::string testPath = "../core/test_data/testFiles/zipperTests/testFiles/";
    std::string const relativePath = "../core/test_data/testFiles/zipperTests/testFiles/";
    GameKit::Zipper::NormalizePathInZip(testPath, relativePath);

    ASSERT_EQ(testPath, "");
}

TEST_F(GameKitZipperTestFixture, LocalPath_NormalizePath_ReturnFileName)
{
    std::string testPath = "../core/test_data/testFiles/zipperTests/testFiles/intoZip2.txt";
    std::string const relativePath = "../core/test_data/testFiles/zipperTests/testFiles";
    GameKit::Zipper::NormalizePathInZip(testPath, relativePath);

    ASSERT_EQ(testPath, "intoZip2.txt");
}

TEST_F(GameKitZipperTestFixture, LongerRelativePath_NormalizePath_ReturnInvalidPath)
{
    std::string testPath = "../core/test_data/testFiles";
    std::string const relativePath = "../core/test_data/testFiles/zipperTests/testFiles";
    GameKit::Zipper::NormalizePathInZip(testPath, relativePath);

    // If the path can't be made relative, we expect it to at least be sanitized (no '../' prefixes)
    ASSERT_EQ(testPath, "core/test_data/testFiles");
}

TEST_F(GameKitZipperTestFixture, TwoFullPaths_NormalizePath_ReturnFileName)
{
#ifdef _WIN32
    std::string testPath = "C:/core/test_data/testFiles/zipperTests/testFiles/intoZip2.txt";
    std::string const relativePath = "C:/core";
#else
    std::string testPath = "/tmp/core/test_data/testFiles/zipperTests/testFiles/intoZip2.txt";
    std::string const relativePath = "/tmp/core";
#endif
    GameKit::Zipper::NormalizePathInZip(testPath, relativePath);

    ASSERT_EQ(testPath, "test_data/testFiles/zipperTests/testFiles/intoZip2.txt");
}

TEST_F(GameKitZipperTestFixture, FullPathAndRelativePath_NormalizePath_ReturnInvalidPath)
{
#ifdef _WIN32
    std::string testPath = "C:/core/test_data/testFiles/zipperTests/testFiles/intoZip2.txt";
    std::string const relativePath = "/core";
#else
    std::string testPath = "/tmp/core/test_data/testFiles/zipperTests/testFiles/intoZip2.txt";
    std::string const relativePath = "/tmp";
#endif

    GameKit::Zipper::NormalizePathInZip(testPath, relativePath);

    // If the path can't be made relative, we expect it to at least be sanitized (no absolute root)
    ASSERT_EQ(testPath, "core/test_data/testFiles/zipperTests/testFiles/intoZip2.txt");
}

TEST_F(GameKitZipperTestFixture, UTF8Paths_AddFileToZipFile_True)
{
    // Unicode U+1F642 = "SLIGHTLY SMILING FACE 🙂"
    const char filenameUtf8[] = u8"../core/test_data/testFiles/zipperTests/testFiles/Temporary \U0001F642 Deleted By Test.txt";
    const auto filename = GameKit::Utils::FileUtils::PathFromUtf8(filenameUtf8);

    {
        // temporary scope to create file without holding it open
        std::ofstream createTestFile(filename);
        ASSERT_TRUE(createTestFile);
    }

    auto result = gamekitZipperInstance->AddFileToZipFile(filenameUtf8);
    ASSERT_TRUE(result);

    // std::remove only takes narrow strings, can't handle unicode on Windows
    boost::filesystem::remove(filename);
}

TEST_F(GameKitZipperTestFixture, UTF8Paths_AddDirectoryToZipFile_True)
{
    // Unicode U+1F642 = "SLIGHTLY SMILING FACE 🙂"
    // Unicode U+2757 = "HEAVY EXCLAMATION MARK ❗"
    const std::string dirnameUtf8 = u8"../core/test_data/testFiles/zipperTests/testFiles/TempDir\U0001F642";
    const std::string filepartUtf8 = u8"Hello\U00002757.txt";

    const auto dirname = GameKit::Utils::FileUtils::PathFromUtf8(dirnameUtf8);
    const auto fullpath = GameKit::Utils::FileUtils::PathFromUtf8(dirnameUtf8 + "/" + filepartUtf8);

    boost::filesystem::create_directory(dirname);

    {
        // temporary scope to create file without holding it open
        std::ofstream createTestFile(fullpath);
        ASSERT_TRUE(createTestFile);
    }

    auto result = gamekitZipperInstance->AddDirectoryToZipFile(dirnameUtf8);
    ASSERT_TRUE(result);

    // clean up
    boost::filesystem::remove_all(dirname);
}
