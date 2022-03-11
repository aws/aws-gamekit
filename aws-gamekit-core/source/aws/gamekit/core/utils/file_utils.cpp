// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// GameKit
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>
#include <aws/gamekit/core/utils/file_utils.h>

// Boost
#include <boost/filesystem.hpp>

// yaml-cpp
#include <yaml-cpp/yaml.h>

using namespace GameKit::Utils;
using namespace GameKit::Logger;

#pragma region Public Methods
unsigned int FileUtils::CalculateDirectoryHash(const std::string& directoryPath, std::string& returnedString, FuncLogCallback logCallback)
{
    using namespace boost::filesystem;

    Aws::Utils::Crypto::Sha256 sha256;
    const Aws::Utils::Base64::Base64 base64;

    std::set<std::string> fileHashSet;

    const path dp(directoryPath);

    if(!is_directory(dp))
    {
        if (logCallback)
        {
            const auto errorMessage = "Failed to locate directory " + dp.string();
            Logging::Log(logCallback, Level::Error, errorMessage.c_str());
        }

        return GAMEKIT_ERROR_DIRECTORY_NOT_FOUND;
    }

    recursive_directory_iterator endIterator;
    for (recursive_directory_iterator dirIterator(dp); dirIterator != endIterator; ++dirIterator)
    {
        // if current path is a directory, do not calculate its hash
        const path cp = (*dirIterator);
        if (!is_directory(cp))
        {
            std::string returnedValue = "";
            ReadFileIntoString(cp.string(), returnedValue, logCallback, "FileUtils::CalculateDirectoryHash()");
            Aws::String fileContents(returnedValue);

            const Aws::Utils::Crypto::HashResult fileHashResult = sha256.Calculate(ToAwsString(returnedValue));
            fileHashSet.insert(ToStdString(base64.Encode(fileHashResult.GetResult())));
        }
    }

    std::string tempDirectoryHashString = "";

    for (const auto fileHashString : fileHashSet)
    {
        tempDirectoryHashString.append(fileHashString);
    }

    const auto hashResult = sha256.Calculate(ToAwsString(tempDirectoryHashString));

    returnedString = ToStdString(base64.Encode(hashResult.GetResult()));

    return GAMEKIT_SUCCESS;
}

unsigned int FileUtils::ReadFileIntoString(const std::string& filePath, std::string& returnedString, FuncLogCallback logCallback, const std::string& errorMessagePrefix)
{
    std::ifstream sourceFile(PathFromUtf8(filePath));
    if (!sourceFile)
    {
        if (logCallback)
        {
            const auto errorMessage = errorMessagePrefix + "Failed to open file for reading " + filePath + ": " + strerror(errno);
            Logging::Log(logCallback, Level::Error, errorMessage.c_str());
        }

        returnedString = "";
        return GAMEKIT_ERROR_FILE_OPEN_FAILED;
    }

    if (sourceFile.peek() == std::ifstream::traits_type::eof())
    {
        returnedString = "";
        return GAMEKIT_SUCCESS;
    }

    std::stringstream buffer;
    buffer << sourceFile.rdbuf();

    if (!buffer || !sourceFile)
    {
        if (logCallback)
        {
            const auto errorMessage = errorMessagePrefix + "Failed to copy data from file " + filePath + ": " + strerror(errno);
            Logging::Log(logCallback, Level::Error, errorMessage.c_str());
        }

        returnedString = "";
        return GAMEKIT_ERROR_FILE_READ_FAILED;
    }

    returnedString = buffer.str();

    // This is a text-based utility function, so strip 3-byte UTF-8 signature \xEF\xBB\xBF
    // if we encounter it at the start, since it is not considered part of the text content.
    if (returnedString.length() >= 3 && memcmp(returnedString.data(), "\xEF\xBB\xBF", 3) == 0)
    {
        returnedString.erase(0, 3);
    }

    return GAMEKIT_SUCCESS;
}

unsigned int FileUtils::WriteStringToFile(const std::string& sourceString, const std::string& filePath, FuncLogCallback logCallback, const std::string& errorMessagePrefix)
{
    std::ofstream destFile;
    const unsigned int returnCode = createOrOpenFile(filePath, destFile, logCallback, errorMessagePrefix);
    if (returnCode != GAMEKIT_SUCCESS)
    {
        return returnCode;
    }

    destFile.write(sourceString.c_str(), sourceString.size());
    if (!destFile)
    {
        if (logCallback)
        {
            const auto errorMessage = errorMessagePrefix + "Failed to write to file " + filePath + ": " + strerror(errno);
            Logging::Log(logCallback, Level::Error, errorMessage.c_str());
        }

        return GAMEKIT_ERROR_FILE_WRITE_FAILED;
    }

    return GAMEKIT_SUCCESS;
}

unsigned int FileUtils::WriteStreamToFile(const std::istream& sourceStream, const std::string& filePath, FuncLogCallback logCallback, const std::string& errorMessagePrefix)
{
    std::ofstream destFile;
    const unsigned int returnCode = createOrOpenFile(filePath, destFile, logCallback, errorMessagePrefix);
    if (returnCode != GAMEKIT_SUCCESS)
    {
        return returnCode;
    }

    destFile << sourceStream.rdbuf();
    if (destFile.fail())
    {
        if (logCallback)
        {
            const auto errorMessage = errorMessagePrefix + "Failed to write to file " + filePath + ": " + strerror(errno);
            Logging::Log(logCallback, Level::Error, errorMessage.c_str());
        }

        return GAMEKIT_ERROR_FILE_WRITE_FAILED;
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKit::Utils::FileUtils::ReadFileAsYAML(const std::string& filePath, YAML::Node& returnedNode, FuncLogCallback logCallback, const std::string& errorMessagePrefix)
{
    std::ifstream stream(PathFromUtf8(filePath));
    if (!stream)
    {
        returnedNode.reset();
        if (logCallback)
        {
            const auto errorMessage = errorMessagePrefix + "Failed to open file for reading " + filePath + ": " + strerror(errno);
            Logging::Log(logCallback, Level::Error, errorMessage.c_str());
        }
        return GAMEKIT_ERROR_FILE_OPEN_FAILED;
    }

    try
    {
        returnedNode = YAML::Load(stream);
    }
    catch (YAML::Exception& e)
    {
        if (!stream.fail())
        {
            returnedNode.reset();
            if (logCallback)
            {
                const auto errorMessage = errorMessagePrefix + "Failed to parse YAML file " + filePath + ": " + e.msg;
                Logging::Log(logCallback, Level::Error, errorMessage.c_str());
            }
            return GAMEKIT_ERROR_GENERAL; // we should make a more general PARSE_ERROR instead of JSON_PARSE_ERROR
        }
    }

    if (stream.fail())
    {
        returnedNode.reset();
        if (logCallback)
        {
            const auto errorMessage = errorMessagePrefix + "Failed to read data from file " + filePath + ": " + strerror(errno);
            Logging::Log(logCallback, Level::Error, errorMessage.c_str());
        }
        return GAMEKIT_ERROR_FILE_READ_FAILED;
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKit::Utils::FileUtils::ReadFileContentsAsYAML(const std::string& fileContents, YAML::Node& returnedNode, FuncLogCallback logCallback, const std::string& errorMessagePrefix)
{
    try
    {
        returnedNode = YAML::Load(fileContents);
    }
    catch (YAML::Exception& e)
    {
        
        returnedNode.reset();
        if (logCallback)
        {
            const auto errorMessage = errorMessagePrefix + "Failed to parse YAML contents " + fileContents.data() + ": " + e.msg;
            Logging::Log(logCallback, Level::Error, errorMessage.c_str());
        }
        return GAMEKIT_ERROR_GENERAL;
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameKit::Utils::FileUtils::WriteYAMLToFile(const YAML::Node& rootNode, const std::string& filePath, const std::string& headerComment, FuncLogCallback logCallback, const std::string& errorMessagePrefix)
{
    std::ofstream destFile;
    const unsigned int returnCode = createOrOpenFile(filePath, destFile, logCallback, errorMessagePrefix);
    if (returnCode != GAMEKIT_SUCCESS)
    {
        return returnCode;
    }

    if (!headerComment.empty())
    {
        destFile << headerComment;
        if (headerComment.back() != '\n')
        {
            destFile << std::endl;
        }
    }

    destFile << rootNode;

    if (destFile.fail())
    {
        if (logCallback)
        {
            const auto errorMessage = errorMessagePrefix + "Failed to write to file " + filePath;
            Logging::Log(logCallback, Level::Error, errorMessage.c_str());
        }
        return GAMEKIT_ERROR_FILE_WRITE_FAILED;
    }

    return GAMEKIT_SUCCESS;
}

FileUtils::PlatformPathString FileUtils::PathFromUtf8(const std::string& pathString)
{
#if _WIN32
    return boost::filesystem::path(pathString).native();
#else
    return pathString;
#endif
}

std::string FileUtils::PathToUtf8(const FileUtils::PlatformPathString& pathString)
{
#if _WIN32
    return boost::filesystem::path(pathString).string();
#else
    return pathString;
#endif
}
#pragma endregion

#pragma region Private Methods
unsigned int FileUtils::createOrOpenFile(const std::string& filePath, std::ofstream& returnFileStream, FuncLogCallback logCallback, const std::string& errorMessagePrefix)
{
    using namespace boost::filesystem;

    // Create missing directories for path:
    const path parentPath = path(filePath).parent_path();
    boost::system::error_code errorCode;
    if (!parentPath.empty() && !exists(parentPath) && !create_directories(parentPath, errorCode))
    {
        if (logCallback)
        {
            const auto errorMessage = errorMessagePrefix + "Failed to create non-existent directories for path " + filePath + ": " + errorCode.message();
            Logging::Log(logCallback, Level::Error, errorMessage.c_str());
        }

        return GAMEKIT_ERROR_DIRECTORY_CREATE_FAILED;
    }

    // Open/create the file:
    returnFileStream = std::ofstream(PathFromUtf8(filePath));
    if (!returnFileStream)
    {
        if (logCallback)
        {
            const auto errorMessage = errorMessagePrefix + "Failed to open file for writing " + filePath + ": " + strerror(errno);
            Logging::Log(logCallback, Level::Error, errorMessage.c_str());
        }

        return GAMEKIT_ERROR_FILE_OPEN_FAILED;
    }

    return GAMEKIT_SUCCESS;
}
#pragma endregion
