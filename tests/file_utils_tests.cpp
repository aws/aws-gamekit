// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "file_utils_tests.h"
#include "aws/gamekit/core/errors.h"
#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>
#include <aws/gamekit/core/utils/file_utils.h>

#include <aws/core/utils/crypto/Factories.h>
#include <aws/core/utils/memory/stl/AWSAllocator.h>

#include <boost/filesystem.hpp>

using namespace GameKit;
namespace fs = boost::filesystem;

class GameKit::Tests::FileUtils::GameKitUtilsFileTestFixture : public ::testing::Test
{
public:
    GameKitUtilsFileTestFixture()
    {}

    ~GameKitUtilsFileTestFixture()
    {}

    void SetUp()
    {
        Aws::Utils::Crypto::InitCrypto();
    }

    void TearDown()
    {
        Aws::Utils::Crypto::CleanupCrypto();
    }
};

using namespace GameKit::Tests::FileUtils;

TEST_F(GameKitUtilsFileTestFixture, DirectoryExists_HashDirectory_StringIsCorrect)
{
    // arrange
    const char* directoryPath = "../core/test_data/testFiles/fileUtilTests/HashDirTest";

    // create a file in the current directory:
    const char* filePath = "../core/test_data/testFiles/fileUtilTests/HashDirTest/TestNewFileForHashOne.txt";
    GameKit::Utils::FileUtils::WriteStringToFile("test", filePath);

    // act
    std::string hashStringOne;
    auto calculateHashStatus = GameKit::Utils::FileUtils::CalculateDirectoryHash(directoryPath, hashStringOne);

    // expected hash before changing file
    const char* expectedHashSingleFile = "PB0KWVxeuirXQRhJnxwt+q0sYoch1hh/EzffJJawE/M=";

    // assert
    ASSERT_EQ(calculateHashStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(hashStringOne, expectedHashSingleFile);

    // update the file so the hash changes
    GameKit::Utils::FileUtils::WriteStringToFile("testTwo", filePath);

    // act
    std::string hashStringTwo;
    calculateHashStatus = GameKit::Utils::FileUtils::CalculateDirectoryHash(directoryPath, hashStringTwo);

    // expected hash after the file changes
    expectedHashSingleFile = "UOb8/ITsCwVftIOwtvMmYdARTFBmWzeHniX3EjypzMs=";

    // assert
    ASSERT_EQ(calculateHashStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(hashStringTwo, expectedHashSingleFile);

    // cleanup
    remove(filePath);
}

TEST_F(GameKitUtilsFileTestFixture, DirectoryDoesNotExist_HashDirectory_ReturnError)
{
    // arrange
    const char* directoryPath = "../core/test_data/noneexistantdir";

    // act
    std::string hashString;
    const auto calculateHashStatus = GameKit::Utils::FileUtils::CalculateDirectoryHash(directoryPath, hashString);

    // assert
    ASSERT_EQ(calculateHashStatus, GAMEKIT_ERROR_DIRECTORY_NOT_FOUND);
    ASSERT_EQ(hashString, "");
}

TEST_F(GameKitUtilsFileTestFixture, AttemptToHashFile_HashDirectory_ReturnError)
{
    // create a file in the current directory:
    const char* filePath = "../core/test_data/testFiles/fileUtilTests/TestNewFileForHashOne.txt";
    GameKit::Utils::FileUtils::WriteStringToFile("test", filePath);

    // act
    std::string hashString;
    const auto calculateHashStatus = GameKit::Utils::FileUtils::CalculateDirectoryHash(filePath, hashString);

    // assert
    ASSERT_EQ(calculateHashStatus, GAMEKIT_ERROR_DIRECTORY_NOT_FOUND);
    ASSERT_EQ(hashString, "");

    remove(filePath);
}

TEST_F(GameKitUtilsFileTestFixture, FileExists_ReadFileIntoString_StringIsCorrect)
{
    // arrange
    const char* filePath = "../core/test_data/testFiles/fileUtilTests/TestReadFile.txt";
    const char* expectedFileContents = "T\nE\nS\nT\nFile\n";

    // act
    std::string loadedString;
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(readStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(loadedString, expectedFileContents);
}

TEST_F(GameKitUtilsFileTestFixture, NonAscii_ReadFileIntoString_StringIsCorrect)
{
    // arrange
    const auto filePath = "../core/test_data/testFiles/fileUtilTests/TestReadNonAsciiCharacters.txt";
    // Note, this data file contains a three-byte UTF-8 header that we expect to be stripped on read
    const auto expectedFileContents = "\xf0\x9f\x99\x82\xe6\xb5\x8b\xe8\xaf\x95";

    // act
    std::string loadedString;
    const auto readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(readStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(loadedString, expectedFileContents);
}

TEST_F(GameKitUtilsFileTestFixture, RelativePathSameDirectory_ReadFileIntoString_StringIsCorrect)
{
    // arrange
    // create a file in the current directory:
    const char* filePath = "TestNewFile.txt";
    const char* expectedFileContents = "test";
    GameKit::Utils::FileUtils::WriteStringToFile(expectedFileContents, filePath);

    // act
    std::string loadedString;
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(readStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(loadedString, expectedFileContents);

    // teardown
    remove(filePath);
}

TEST_F(GameKitUtilsFileTestFixture, RelativePathDotNotation_ReadFileIntoString_StringIsCorrect)
{
    // arrange
    // create a file one directory above the current directory:
    const char* filePath = "../TestNewFile.txt";
    const char* expectedFileContents = "test";
    GameKit::Utils::FileUtils::WriteStringToFile(expectedFileContents, filePath);

    // act
    std::string loadedString;
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(readStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(loadedString, expectedFileContents);

    // teardown
    remove(filePath);
}

TEST_F(GameKitUtilsFileTestFixture, AbsolutePath_ReadFileIntoString_StringIsCorrect)
{
    // arrange
    // create a file at an absolute path (i.e. a fully qualified path like "C:\foo\bar.txt"):
    const std::string filePath = fs::temp_directory_path().append("TestNewFile.txt").string();
    const std::string expectedFileContents = "test";
    GameKit::Utils::FileUtils::WriteStringToFile(expectedFileContents, filePath);

    // act
    std::string loadedString;
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(readStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(loadedString, expectedFileContents);

    // teardown
    remove(filePath.c_str());
}

TEST_F(GameKitUtilsFileTestFixture, EmptyFile_ReadFileIntoString_StringIsCorrect)
{
    // arrange
    const char* filePath = "../core/test_data/testFiles/fileUtilTests/TestReadEmptyFile.txt";
    const char* expectedFileContents = "";

    // act
    std::string loadedString;
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(readStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(loadedString, expectedFileContents);
}

TEST_F(GameKitUtilsFileTestFixture, NonEmptyInputStringAndSuccess_ReadFileIntoString_StringIsCorrect)
{
    // arrange
    const char* filePath = "../core/test_data/testFiles/fileUtilTests/TestReadFile.txt";
    const char* expectedString = "T\nE\nS\nT\nFile\n";

    std::string loadedString = "non-empty, already contains text";

    // act
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(readStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(loadedString, expectedString);
}

TEST_F(GameKitUtilsFileTestFixture, NonEmptyInputStringAndError_ReadFileIntoString_EmptyString)
{
    // arrange
    const char* filePath = "../fakePath/TestReadFile.txt";
    const char* expectedString = "";

    std::string loadedString = "non-empty, already contains text";

    // act
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(readStatus, GAMEKIT_ERROR_FILE_OPEN_FAILED);
    ASSERT_EQ(loadedString, expectedString);
}

TEST_F(GameKitUtilsFileTestFixture, PathDoesNotExist_ReadFileIntoString_ReadFails)
{
    // arrange
    const char* filePath = "../fakePath/TestReadFile.txt";
    const char* expectedString = "";

    // act
    std::string loadedString;
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(readStatus, GAMEKIT_ERROR_FILE_OPEN_FAILED);
    ASSERT_EQ(loadedString, expectedString);
}

TEST_F(GameKitUtilsFileTestFixture, PathEmpty_ReadFileIntoString_ReadFails)
{
    // arrange
    const char* filePath = "";
    const char* expectedString = "";

    // act
    std::string loadedString;
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(readStatus, GAMEKIT_ERROR_FILE_OPEN_FAILED);
    ASSERT_EQ(loadedString, expectedString);
}

TEST_F(GameKitUtilsFileTestFixture, FileDoesNotExist_ReadFileIntoString_EmptyString)
{
    // arrange
    const auto filePath = "../core/test_data/testFiles/fileUtilTests/DoesNotExist.txt";
    const auto expectedString = "";

    // act
    std::string loadedString;
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(readStatus, GAMEKIT_ERROR_FILE_OPEN_FAILED);
    ASSERT_EQ(loadedString, expectedString);
}

TEST_F(GameKitUtilsFileTestFixture, FileDoesNotExist_WriteStringIntoFile_StringIsCorrect)
{
    // arrange
    const char* filePath = "../core/test_data/testFiles/fileUtilTests/TestWriteNewFile.txt";
    const char* expectedFileContents = "T\nE\nS\nT\nWriteNewFile\n";

    // act
    const unsigned int writeStatus = GameKit::Utils::FileUtils::WriteStringToFile(expectedFileContents, filePath);

    std::string loadedString;
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(writeStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(loadedString, expectedFileContents);

    // teardown
    remove(filePath);
}

TEST_F(GameKitUtilsFileTestFixture, PathDoesNotExist_WriteStringIntoFile_StringIsCorrect)
{
    // arrange
    const char* filePath = "./fakePath/fakePath2/FakeFile.txt";
    const char* expectedFileContents = "T\nE\nS\nT\nWriteExistingFile\n";
    remove(filePath);

    // act
    const unsigned int writeStatus = GameKit::Utils::FileUtils::WriteStringToFile(expectedFileContents, filePath);

    std::string loadedString;
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(writeStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(loadedString, expectedFileContents);

    // teardown
    remove(filePath);
}

TEST_F(GameKitUtilsFileTestFixture, FileAlreadyExist_WriteStringIntoFile_StringIsCorrect)
{
    // act
    const char* filePath = "../core/test_data/testFiles/fileUtilTests/TestWriteExistingFile.txt";
    const char* expectedFileContents = "T\nE\nS\nT\nWriteExistingFile\n";
    const unsigned int writeStatus = GameKit::Utils::FileUtils::WriteStringToFile(expectedFileContents, filePath);

    std::string loadedString;
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(writeStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(loadedString, expectedFileContents);

    // teardown
    // skipped - this file is checked into version control
}

TEST_F(GameKitUtilsFileTestFixture, RelativePathSameDirectory_WriteStringIntoFile_StringIsCorrect)
{
    // act
    const char* filePath = "TestWriteNewFile.txt";
    const char* expectedFileContents = "T\nE\nS\nT\nWriteNewFile\n";
    const unsigned int writeStatus = GameKit::Utils::FileUtils::WriteStringToFile(expectedFileContents, filePath);

    std::string loadedString;
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(writeStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(loadedString, expectedFileContents);

    // teardown
    remove(filePath);
}

TEST_F(GameKitUtilsFileTestFixture, RelativePathDotNotation_WriteStringIntoFile_StringIsCorrect)
{
    // act
    const char* filePath = "../TestWriteNewFile.txt";
    const char* expectedFileContents = "T\nE\nS\nT\nWriteNewFile\n";
    const unsigned int writeStatus = GameKit::Utils::FileUtils::WriteStringToFile(expectedFileContents, filePath);

    std::string loadedString;
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(writeStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(loadedString, expectedFileContents);

    // teardown
    remove(filePath);
}

TEST_F(GameKitUtilsFileTestFixture, AbsolutePath_WriteStringIntoFile_StringIsCorrect)
{
    // arrange
    // create a file at an absolute path (i.e. a fully qualified path like "C:\foo\bar.txt"):
    const std::string absolutePath = fs::temp_directory_path().append("TestWriteNewFile.txt").string();
    const std::string expectedFileContents = "T\nE\nS\nT\nWriteNewFile\n";

    // act
    const unsigned int writeStatus = GameKit::Utils::FileUtils::WriteStringToFile(expectedFileContents, absolutePath);

    std::string loadedString;
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(absolutePath, loadedString);

    // assert
    ASSERT_EQ(writeStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(loadedString, expectedFileContents);

    // teardown
    remove(absolutePath.c_str());
}

TEST_F(GameKitUtilsFileTestFixture, EmptySourceString_WriteStringIntoFile_StringIsCorrect)
{
    // arrange
    const char* filePath = "../core/test_data/testFiles/fileUtilTests/TestWriteNewFile.txt";
    const char* expectedFileContents = "";

    // act
    const unsigned int writeStatus = GameKit::Utils::FileUtils::WriteStringToFile(expectedFileContents, filePath);

    std::string loadedString;
    const unsigned int readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(writeStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(loadedString, expectedFileContents);

    // teardown
    remove(filePath);
}

TEST_F(GameKitUtilsFileTestFixture, PathEmpty_WriteStringIntoFile_WriteFails)
{
    // arrange
    const char* filePath = "";
    const char* stringToWrite = "T\nE\nS\nT\nWriteExistingFile\n";

    // act
    const unsigned int writeStatus = GameKit::Utils::FileUtils::WriteStringToFile(stringToWrite, filePath);

    // assert
    ASSERT_EQ(writeStatus, GAMEKIT_ERROR_FILE_OPEN_FAILED);
    ASSERT_FALSE(fs::exists(filePath));
}

TEST_F(GameKitUtilsFileTestFixture, FileDoesNotExist_WriteStreamToFile_StringIsCorrect)
{
    // arrange
    const char* filePath = "../core/test_data/testFiles/fileUtilTests/TestWriteNewFile.txt";
    const char* expectedFileContents = "T\nE\nS\nT\nWriteNewFile\n";
    const std::stringstream stream(expectedFileContents);

    // act
    const unsigned int writeResult = GameKit::Utils::FileUtils::WriteStreamToFile(stream, filePath);

    std::string loadedString;
    const unsigned int readResult = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(writeResult, GAMEKIT_SUCCESS);
    ASSERT_EQ(readResult, GAMEKIT_SUCCESS);
    ASSERT_EQ(0, strcmp(loadedString.c_str(), expectedFileContents));

    // teardown
    remove(filePath);
}

TEST_F(GameKitUtilsFileTestFixture, PathDoesNotExist_WriteStreamToFile_StringIsCorrect)
{
    // arrange
    const char* filePath = "./fakePath/fakePath2/FakeFile.txt";
    const char* expectedFileContents = "T\nE\nS\nT\nWriteNewFile\n";
    const std::stringstream stream(expectedFileContents);

    // act
    const unsigned int writeResult = GameKit::Utils::FileUtils::WriteStreamToFile(stream, filePath);

    std::string loadedString;
    const unsigned int readResult = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_EQ(writeResult, GAMEKIT_SUCCESS);
    ASSERT_EQ(readResult, GAMEKIT_SUCCESS);
    ASSERT_EQ(0, strcmp(loadedString.c_str(), expectedFileContents));

    // teardown
    remove(filePath);
}

TEST_F(GameKitUtilsFileTestFixture, NonAscii_PathFromUtf8_Conversions)
{
    // arrange
    const char* const filePath = u8"./Hello world - Καλημέρα κόσμε - コンニチハ.txt";
    const wchar_t* const widePath = L"./Hello world - Καλημέρα κόσμε - コンニチハ.txt";

    // act
    const auto nativePath = GameKit::Utils::FileUtils::PathFromUtf8(filePath);
    const auto utf8Path = GameKit::Utils::FileUtils::PathToUtf8(nativePath);

    // assert
#if _WIN32
    ASSERT_EQ(nativePath, widePath);
#else
    ASSERT_EQ(nativePath, filePath);
#endif
    ASSERT_EQ(utf8Path, filePath);
}

TEST_F(GameKitUtilsFileTestFixture, NonAscii_WriteStringIntoFile_RoundTrip)
{
    // arrange
    // create a file in the current directory:
    const char* const filePath = u8"TestNonAsciiFile - 🙂.txt";
    const std::string contents = u8"Hello world\nΚαλημέρα κόσμε\nコンニチハ";

    // act
    auto nativePath = GameKit::Utils::FileUtils::PathFromUtf8(filePath);
    boost::filesystem::remove(nativePath);

    std::string loadedString;
    const auto writeStatus = GameKit::Utils::FileUtils::WriteStringToFile(contents, filePath);
    const auto readStatus = GameKit::Utils::FileUtils::ReadFileIntoString(filePath, loadedString);

    // assert
    ASSERT_TRUE(boost::filesystem::exists(nativePath));
    ASSERT_EQ(writeStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(readStatus, GAMEKIT_SUCCESS);
    ASSERT_EQ(loadedString, contents);

    // teardown
    // Using boost::filesystem because it can always accept platform-native strings
    boost::filesystem::remove(nativePath);
}

TEST_F(GameKitUtilsFileTestFixture, NonAscii_BoostFilesystemPath_Conversions)
{
    const std::string filePath = u8"./Hello world - Καλημέρα κόσμε - コンニチハ.txt";
    const std::wstring widePath = L"./Hello world - Καλημέρα κόσμε - コンニチハ.txt";
    ASSERT_EQ(filePath, boost::filesystem::path(filePath).string());
    ASSERT_EQ(filePath, boost::filesystem::path(widePath).string());
#ifdef _WIN32
    ASSERT_EQ(widePath, boost::filesystem::path(filePath).native());
    ASSERT_EQ(widePath, boost::filesystem::path(widePath).native());
#else
    ASSERT_EQ(widePath, boost::filesystem::path(filePath).wstring());
    ASSERT_EQ(widePath, boost::filesystem::path(widePath).wstring());
#endif
}

TEST_F(GameKitUtilsFileTestFixture, WindowsOnly_BoostFilesystemPath_LongPaths)
{
#ifdef _WIN32
    // conversion to wide should add prefix on absolute paths, convert to backslash
    ASSERT_EQ(std::wstring(L"\\\\?\\C:\\"), boost::filesystem::path("C:/").wstring());
    ASSERT_EQ(std::wstring(L"\\\\?\\UNC\\net\\share"), boost::filesystem::path("\\\\net\\share").wstring());

    // simple un-prefixed paths should round-trip cleanly aside from backslash conversion
    ASSERT_EQ(std::string("C:\\"), boost::filesystem::path("C:/").string());
    ASSERT_EQ(std::string("\\\\net\\share"), boost::filesystem::path("\\\\net\\share").string());

    // conversion to narrow should remove prefix from absolute paths
    ASSERT_EQ(std::string("C:\\"), boost::filesystem::path(L"\\\\?\\C:\\").string());
    ASSERT_EQ(std::string("\\\\net\\share"), boost::filesystem::path(L"\\\\?\\UNC\\net\\share").string());

    // user-prefixed narrow paths should have prefix stripped due to internal wstring conversion
    // (this is a little unintuitive, but we are testing the expected behavior)
    ASSERT_EQ(std::string("C:\\"), boost::filesystem::path("\\\\?\\C:\\").string());
    ASSERT_EQ(std::string("\\\\net\\share"), boost::filesystem::path("\\\\?\\UNC\\net\\share").string());
#endif
}
