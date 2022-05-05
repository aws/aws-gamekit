// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

// AWS SDK
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/utils/StringUtils.h>

// GameKit
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/game-saving/gamekit_game_saving.h>

// Workaround for conflict with user.h PAGE_SIZE macro when compiling for Android
#pragma push_macro("PAGE_SIZE")
#undef PAGE_SIZE

using namespace Aws::Utils;
using namespace Aws::Utils::Json;
using namespace GameKit::GameSaving;
using namespace GameKit::Logger;
using namespace GameKit::Utils;

#pragma region Constants
const std::string GameSaving::START_KEY = "start_key";
const std::string GameSaving::PAGING_TOKEN = "paging_token";
const std::string GameSaving::PAGE_SIZE = "page_size";
const std::string GameSaving::METADATA = "metadata";
const std::string GameSaving::HASH = "hash";
const std::string GameSaving::LAST_MODIFIED_EPOCH_TIME = "last_modified_epoch_time";
const std::string GameSaving::TIME_TO_LIVE = "time_to_live";
const std::string GameSaving::CONSISTENT_READ = "consistent_read";
const Aws::String GameSaving::S3_SHA_256_METADATA_HEADER = "x-amz-meta-hash";
const Aws::String GameSaving::S3_SLOT_METADATA_HEADER = "x-amz-meta-slot_metadata";
const Aws::String GameSaving::S3_EPOCH_METADATA_HEADER = "x-amz-meta-epoch";
const long TIMEOUT = 5000; // 5 seconds
#pragma endregion

#pragma region Constructors/Destructor
GameSaving::GameSaving(Authentication::GameKitSessionManager* sessionManager, FuncLogCallback logCb, const char* const* localSlotInformationFilePaths, unsigned int arraySize, FileActions fileActions) :
    m_sessionManager(sessionManager),
    m_fileWriteCallback(fileActions.fileWriteCallback),
    m_fileReadCallback(fileActions.fileReadCallback),
    m_fileSizeCallback(fileActions.fileSizeCallback),
    m_fileWriteDispatchReceiver(fileActions.fileWriteDispatchReceiver),
    m_fileReadDispatchReceiver(fileActions.fileReadDispatchReceiver),
    m_fileSizeDispatchReceiver(fileActions.fileSizeDispatchReceiver)
{
    m_featureName = "gamesaving";
    m_logCb = logCb;

    GameKit::AwsApiInitializer::Initialize(m_logCb, this);

    Aws::Client::ClientConfiguration clientConfig;
    GameKit::DefaultClients::SetDefaultClientConfiguration(m_sessionManager->GetClientSettings(), clientConfig);
    clientConfig.region = m_sessionManager->GetClientSettings()[ClientSettings::Authentication::SETTINGS_IDENTITY_REGION];
    clientConfig.connectTimeoutMs = TIMEOUT;
    clientConfig.httpRequestTimeoutMs = TIMEOUT;
    clientConfig.requestTimeoutMs = TIMEOUT;
    m_httpClient = Aws::Http::CreateHttpClient(clientConfig);

    m_currentTimeProvider = std::make_shared<Utils::AwsCurrentTimeProvider>();

    m_caller.Initialize(m_sessionManager, logCb, &m_httpClient);

    loadSlotInformation(localSlotInformationFilePaths, arraySize);

    Logging::Log(m_logCb, Level::Info, "Game Saving instantiated");
}

GameSaving::~GameSaving()
{
    AwsApiInitializer::Shutdown(m_logCb, this);
    m_logCb = nullptr;
}
#pragma endregion

#pragma region Public Methods
void GameKit::GameSaving::GameSaving::AddLocalSlots(const char* const* localSlotInformationFilePaths, unsigned int arraySize)
{
    loadSlotInformation(localSlotInformationFilePaths, arraySize);
}

void GameKit::GameSaving::GameSaving::SetFileActions(FileActions fileActions)
{
    m_fileWriteCallback = fileActions.fileWriteCallback;
    m_fileReadCallback = fileActions.fileReadCallback;
    m_fileSizeCallback = fileActions.fileSizeCallback;
    m_fileWriteDispatchReceiver = fileActions.fileWriteDispatchReceiver;
    m_fileReadDispatchReceiver = fileActions.fileReadDispatchReceiver;
    m_fileSizeDispatchReceiver = fileActions.fileSizeDispatchReceiver;
}

unsigned int GameSaving::GetAllSlotSyncStatuses(DISPATCH_RECEIVER_HANDLE receiver, GameSavingResponseCallback resultCb, bool waitForAllPages, unsigned int pageSize)
{
    // to make this function thread safe, lock it behind a mutex
    std::lock_guard<std::mutex> guard(m_gameSavingMutex);

    if (!isPlayerLoggedIn("GetAllSlotSyncStatuses"))
    {
        return invokeCallback(receiver, resultCb, GAMEKIT_ERROR_NO_ID_TOKEN);
    }

    // assume all cached slots are not on the cloud, set all of their status to SlotSyncStatus::SHOULD_UPLOAD_LOCAL
    for (std::pair<std::string, CachedSlot> slot : m_syncedSlots)
    {
        m_syncedSlots.at(slot.first).slotSyncStatus = SlotSyncStatus::SHOULD_UPLOAD_LOCAL;
    }

    const std::string uri = m_sessionManager->GetClientSettings()[ClientSettings::GameSaving::SETTINGS_GAME_SAVING_BASE_URL];

    // apply bounds to pageSize
    pageSize = pageSize > MAX_PAGE_SIZE ? MAX_PAGE_SIZE : pageSize;

    std::unordered_set<std::string> slotsFromCloud;
    Aws::String startKey, pagingToken;
    do
    {
        Caller::CallerParams queryString;

        if (!startKey.empty())
        {
            queryString[START_KEY] = ToStdString(startKey);
        }

        if (!pagingToken.empty())
        {
            queryString[PAGING_TOKEN] = ToStdString(pagingToken);
        }

        if (pageSize > 0)
        {
            queryString[PAGE_SIZE] = std::to_string(pageSize);
        }

        JsonValue jsonBody;
        unsigned int returnCode = m_caller.CallApiGateway(uri, Aws::Http::HttpMethod::HTTP_GET, "GetAllSlotSyncStatuses", jsonBody, queryString);
        if (returnCode != GameKit::GAMEKIT_SUCCESS)
        {
            return invokeCallback(receiver, resultCb, returnCode);
        }

        Aws::Utils::Array<JsonView> jsonArray = jsonBody.View().GetObject("data").GetArray("slots_metadata");

        std::vector<Slot> returnedSlotList;
        for (size_t i = 0; i < jsonArray.GetLength(); ++i)
        {
            const std::string name = ToStdString(jsonArray.GetItem(i).GetString("slot_name"));

            CachedSlot& slot = m_syncedSlots[name];

            slot.slotName = name;

            updateSlotFromJson(jsonArray.GetItem(i), slot);
            updateSlotSyncStatus(slot);

            returnedSlotList.push_back(slot);
            slotsFromCloud.insert(slot.slotName);
        }

        JsonView jsonView = jsonBody.View().GetObject("paging");
        if (jsonView.KeyExists("next_start_key"))
        {
            JsonView nextKey = jsonView.GetObject("next_start_key");
            startKey = nextKey.GetString("slot_name");
            if (!jsonView.KeyExists(ToAwsString(PAGING_TOKEN)))
            {
                Logging::Log(m_logCb, Level::Error, "paging_token missing from response with next_start_key");
                pagingToken = "";
            }
            else
            {
                pagingToken = jsonView.GetString(ToAwsString(PAGING_TOKEN));
            }

            if (!waitForAllPages)
            {
                // pass in the list of slots that have been updated for this page
                invokeCallback(receiver, resultCb, returnedSlotList);
            }
        }
        else
        {
            startKey.clear();
        }
    } while (!startKey.empty());

    return invokeCallback(receiver, resultCb, waitForAllPages, slotsFromCloud);
}

unsigned int GameSaving::GetSlotSyncStatus(DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, const char* slotName)
{
    // to make this function thread safe, lock it behind a mutex
    std::lock_guard<std::mutex> guard(m_gameSavingMutex);

    if (!isPlayerLoggedIn("GetSlotSyncStatus"))
    {
        return invokeCallback(receiver, resultCb, GAMEKIT_ERROR_NO_ID_TOKEN);
    }

    if (!ValidationUtils::IsValidPrimaryIdentifier(slotName))
    {
        const std::string errorMessage = "Error: GameSaving::GetSlotSyncStatus() malformed slot name: " + std::string(slotName) + ". Slot name" + GameKit::Utils::PRIMARY_IDENTIFIER_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return invokeCallback(receiver, resultCb, GAMEKIT_ERROR_GAME_SAVING_MALFORMED_SLOT_NAME);
    }

    const auto foundSlot = m_syncedSlots.find(slotName);
    if (foundSlot == m_syncedSlots.end())
    {
        const std::string errorMessage = "Error: GameSaving::GetSlotSyncStatus() no cached slot found: " + std::string(slotName);
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return invokeCallback(receiver, resultCb, GAMEKIT_ERROR_GAME_SAVING_SLOT_NOT_FOUND);
    }
    CachedSlot& slot = m_syncedSlots.at(slotName);

    const unsigned int status = getSlotSyncStatusInternal(slot);
    if (status != GAMEKIT_SUCCESS) {
        return invokeCallback(receiver, resultCb, status);
    }

    return invokeCallback(receiver, resultCb, GAMEKIT_SUCCESS, slot);
}

unsigned int GameSaving::DeleteSlot(DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, const char* slotName)
{
    // to make this function thread safe, lock it behind a mutex
    std::lock_guard<std::mutex> guard(m_gameSavingMutex);

    if (!isPlayerLoggedIn("DeleteSlot"))
    {
        return invokeCallback(receiver, resultCb, GAMEKIT_ERROR_NO_ID_TOKEN);
    }

    if (!ValidationUtils::IsValidPrimaryIdentifier(slotName))
    {
        const std::string errorMessage = "Error: GameSaving::DeleteSlot() malformed slot name: " + std::string(slotName) + ". Slot name" + GameKit::Utils::PRIMARY_IDENTIFIER_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return invokeCallback(receiver, resultCb, GAMEKIT_ERROR_GAME_SAVING_MALFORMED_SLOT_NAME);
    }

    const auto foundSlot = m_syncedSlots.find(slotName);
    if (foundSlot == m_syncedSlots.end())
    {
        const std::string errorMessage = "Error: GameSaving::DeleteSlot() no cached slot found: " + std::string(slotName);
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return invokeCallback(receiver, resultCb, GAMEKIT_ERROR_GAME_SAVING_SLOT_NOT_FOUND);
    }

    const std::string uri = m_sessionManager->GetClientSettings()[ClientSettings::GameSaving::SETTINGS_GAME_SAVING_BASE_URL] + "/" + slotName;

    JsonValue jsonBody;
    unsigned int returnCode = m_caller.CallApiGateway(uri, Aws::Http::HttpMethod::HTTP_DELETE, "DeleteSlot", jsonBody);
    if (returnCode != GAMEKIT_SUCCESS)
    {
        return invokeCallback(receiver, resultCb, returnCode);
    }

    const auto deletedSlot = m_syncedSlots.at(slotName);
    const auto deletedSlotCopy = Slot(deletedSlot);
    m_syncedSlots.erase(slotName);

    return invokeCallback(receiver, resultCb, GAMEKIT_SUCCESS, deletedSlotCopy);
}

unsigned int GameSaving::SaveSlot(DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, GameSavingModel model)
{
    // To make this function thread safe, lock it behind a mutex
    std::lock_guard<std::mutex> guard(m_gameSavingMutex);

    if (!isPlayerLoggedIn("SaveSlot"))
    {
        return invokeCallback(receiver, resultCb, GAMEKIT_ERROR_NO_ID_TOKEN);
    }

    if (!ValidationUtils::IsValidPrimaryIdentifier(model.slotName))
    {
        const std::string errorMessage = "Error: GameSaving::SaveSlot() malformed slot name: " + std::string(model.slotName) + ". Slot name" + GameKit::Utils::PRIMARY_IDENTIFIER_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return invokeCallback(receiver, resultCb, GAMEKIT_ERROR_GAME_SAVING_MALFORMED_SLOT_NAME);
    }

    // Add the slot if it isn't present
    unsigned int status = addSlot(model.slotName);
    if (status != GAMEKIT_SUCCESS)
    {
        return invokeCallback(receiver, resultCb, status);
    }

    CachedSlot& slot = m_syncedSlots.at(model.slotName);

    // Update the slot's local information, save it to a file, then get the the updated sync status from the cloud.
    status = updateLocalSlotStatus(slot, model);
    if (status != GAMEKIT_SUCCESS)
    {
        return invokeCallback(receiver, resultCb, status);
    }

    // Upload the save from the provided buffer, get the new sync status.
    status = uploadLocalSlot(model, slot);
    if (status != GAMEKIT_SUCCESS)
    {
        return invokeCallback(receiver, resultCb, status);
    }

    // Re-save the metadata with the new sync status and modified times
    status = saveSlotInformation(slot, model.localSlotInformationFilePath);
    if (status != GAMEKIT_SUCCESS)
    {
        return invokeCallback(receiver, resultCb, status);
    }

    return invokeCallback(receiver, resultCb, GAMEKIT_SUCCESS, slot);
}

unsigned int GameSaving::LoadSlot(DISPATCH_RECEIVER_HANDLE receiver, GameSavingDataResponseCallback resultCb, GameSavingModel model)
{
    // To make this function thread safe, lock it behind a mutex
    std::lock_guard<std::mutex> guard(m_gameSavingMutex);

    if (!isPlayerLoggedIn("LoadSlot"))
    {
        return invokeCallback(receiver, resultCb, GAMEKIT_ERROR_NO_ID_TOKEN);
    }

    if (!ValidationUtils::IsValidPrimaryIdentifier(model.slotName))
    {
        const std::string errorMessage = "Error: GameSaving::LoadSlot() malformed slot name: " + std::string(model.slotName) + ". Slot name" + GameKit::Utils::PRIMARY_IDENTIFIER_REQUIREMENTS_TEXT;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return invokeCallback(receiver, resultCb, GAMEKIT_ERROR_GAME_SAVING_MALFORMED_SLOT_NAME);
    }

    const auto foundSlot = m_syncedSlots.find(model.slotName);
    if (foundSlot == m_syncedSlots.end())
    {
        const std::string errorMessage = "Error: GameSaving::LoadSlot() no cached slot found: " + std::string(model.slotName);
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return invokeCallback(receiver, resultCb, GAMEKIT_ERROR_GAME_SAVING_SLOT_NOT_FOUND);
    }
    CachedSlot& slot = m_syncedSlots.at(model.slotName);

    // Fetch the current slot sync status - important to make sure our slot information is up to date
    const unsigned int returnCode = getSlotSyncStatusInternal(slot);
    if (returnCode != GAMEKIT_SUCCESS)
    {
        return invokeCallback(receiver, resultCb, returnCode);
    }

    // Download the requested slot from the cloud, update its sync information and times
    unsigned int outActualSlotSize = 0;
    unsigned int status = downloadCloudSlot(model, slot, outActualSlotSize);
    if (status != GAMEKIT_SUCCESS)
    {
        return invokeCallback(receiver, resultCb, status);
    }

    // save the newly updated metadata to the provided filepath
    status = saveSlotInformation(m_syncedSlots.at(model.slotName), model.localSlotInformationFilePath);
    if (status != GAMEKIT_SUCCESS)
    {
        return invokeCallback(receiver, resultCb, status);
    }

    return invokeCallback(receiver, resultCb, GAMEKIT_SUCCESS, slot, model.data, outActualSlotSize);
}

#pragma endregion

#pragma region Private Methods
bool GameSaving::isPlayerLoggedIn(const std::string& methodName) const
{
    const std::string idToken = m_sessionManager->GetToken(GameKit::TokenType::IdToken);

    if (idToken.empty())
    {
        const std::string message = "GameSaving::" + methodName + "() No ID token in session.";
        Logging::Log(m_logCb, Level::Error, message.c_str());
        return false;
    }

    return true;
}

unsigned int GameSaving::getSlotSyncStatusInternal(CachedSlot& slot)
{
    const std::string uri = m_sessionManager->GetClientSettings()[ClientSettings::GameSaving::SETTINGS_GAME_SAVING_BASE_URL] + "/" + slot.slotName;

    JsonValue jsonBody;
    unsigned int returnCode = m_caller.CallApiGateway(uri, Aws::Http::HttpMethod::HTTP_GET, "GetSlotSyncStatus", jsonBody);
    if (returnCode != GAMEKIT_SUCCESS)
    {
        return returnCode;
    }

    const std::string name = ToStdString(jsonBody.View().GetObject("data").GetString("slot_name"));
    if (name == slot.slotName)
    {
        // json entry found, update the slot cloud info
        updateSlotFromJson(jsonBody.View().GetObject("data"), slot);
    }
    else
    {
        const std::string message = "Info: GameSaving::GetSlotSyncStatus() slot not found in cloud: " + slot.slotName;
        Logging::Log(m_logCb, Level::Info, message.c_str());
    }

    updateSlotSyncStatus(slot);

    return GAMEKIT_SUCCESS;
}

unsigned int GameSaving::uploadLocalSlot(GameSavingModel& model, CachedSlot& slot)
{
    // Validate metadata length
    if (strlen(model.metadata) > MAX_METADATA_BYTES)
    {
        std::stringstream max, found;
        max << MAX_METADATA_BYTES;
        found << strlen(model.metadata);
        const std::string errorMessage = "Info: GameSaving::uploadLocalSlot() metadata is greater than max allowed size. Max Size: " + max.str() + " Bytes, Found Size: " + found.str() + " Bytes";
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_GAME_SAVING_EXCEEDED_MAX_SIZE;
    }

    // convert the databuffer
    const std::shared_ptr<Aws::IOStream> objectStream = Aws::MakeShared<Aws::StringStream>(model.slotName);
    objectStream->write((char*)model.data, model.dataSize);
    objectStream->flush();
    unsigned int size = (unsigned int)objectStream->tellp();

    if (!model.overrideSync)
    {
        // get the updated status for the slot and validate we should be uploading
        std::string message;
        switch (m_syncedSlots.at(model.slotName).slotSyncStatus)
        {
        case SlotSyncStatus::SHOULD_DOWNLOAD_CLOUD:
            message = "Info: GameSaving::uploadLocalSlot() cloud slot may be newer: " + std::string(model.slotName);
            Logging::Log(m_logCb, Level::Info, message.c_str());
            return GAMEKIT_ERROR_GAME_SAVING_CLOUD_SLOT_IS_NEWER;

        case SlotSyncStatus::SYNCED:
            message = "Info: GameSaving::uploadLocalSlot() local slot is already in sync with the cloud, will upload again anyways: " + std::string(model.slotName);
            Logging::Log(m_logCb, Level::Info, message.c_str());

            // note: we continue with the method and do not return for this case
            break;

        case SlotSyncStatus::SHOULD_UPLOAD_LOCAL:
            message = "Info: GameSaving::uploadLocalSlot() slot status is safe to upload: " + std::string(model.slotName);
            Logging::Log(m_logCb, Level::Info, message.c_str());

            // note: we continue with the method and do not return for this case
            break;

        case SlotSyncStatus::IN_CONFLICT:
            /* fall through */
        case SlotSyncStatus::UNKNOWN:
            /* fall through */
        default:
            message = "Info: GameSaving::uploadLocalSlot() sync conflict detected, use overrideSync = true to clear by forcing upload: " + std::string(model.slotName);
            Logging::Log(m_logCb, Level::Info, message.c_str());
            return GAMEKIT_ERROR_GAME_SAVING_SYNC_CONFLICT;
        }
    }

    // SHA-256 of the slot is used to check validity of the file when downloading it later.
    // This value must be present in both the request to generate the presigned S3 url, as well as
    // when uploading to S3 using the presigned url.
    const std::string hash = getSha256(*objectStream);
    const std::string uri = m_sessionManager->GetClientSettings()[ClientSettings::GameSaving::SETTINGS_GAME_SAVING_BASE_URL] + "/" + model.slotName + "/upload_url";

    // Encode the metadata using base64, allowing non-ascii characters when sent to S3
    const std::string encodedMetadata = EncodingUtils::EncodeBase64(model.metadata);

    Caller::CallerParams queryString({
        { CONSISTENT_READ, model.consistentRead ? "True" : "False" }
    });
    if (model.urlTimeToLive > 0)
    {
        queryString[TIME_TO_LIVE] = std::to_string(model.urlTimeToLive);
    }

    Caller::CallerParams headerParams({
        { HASH, hash },
        { LAST_MODIFIED_EPOCH_TIME, std::to_string(model.epochTime)}
    });
    if (strlen(model.metadata) > 0)
    {
        headerParams[METADATA] = encodedMetadata;
    }

    JsonValue jsonBody;
    unsigned int returnCode = m_caller.CallApiGateway(uri, Aws::Http::HttpMethod::HTTP_GET, "uploadLocalSlot", jsonBody, queryString, headerParams);
    if (returnCode != GAMEKIT_SUCCESS)
    {
        return returnCode;
    }

    const Aws::String presignedUrlPut = jsonBody.View().GetObject("data").GetString("url");

    if (presignedUrlPut.empty())
    {
        const std::string errorMessage = "Error: GameSaving::uploadLocalSlot() url response formatted incorrectly or not found";
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_PARSE_JSON_FAILED;
    }

    const std::shared_ptr<Aws::Http::HttpRequest> putRequest = CreateHttpRequest(presignedUrlPut, Aws::Http::HttpMethod::HTTP_PUT, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

    putRequest->SetHeaderValue(S3_SHA_256_METADATA_HEADER, ToAwsString(hash));
    putRequest->SetHeaderValue(S3_SLOT_METADATA_HEADER, ToAwsString(encodedMetadata));
    putRequest->SetHeaderValue(S3_EPOCH_METADATA_HEADER, StringUtils::to_string(model.epochTime));

    putRequest->AddContentBody(objectStream);

    Aws::StringStream intConverter;
    intConverter << size;
    putRequest->SetContentLength(intConverter.str());

    const std::shared_ptr<Aws::Http::HttpResponse> putResponse = m_httpClient->MakeRequest(putRequest);
    if (putResponse->GetResponseCode() != Aws::Http::HttpResponseCode::OK)
    {
        const std::string errorMessage = "Error: GameSaving::uploadLocalSlot() returned with http response code: " + std::to_string(static_cast<int>(putResponse->GetResponseCode()));
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_HTTP_REQUEST_FAILED;
    }

    std::string message = std::string("Info: GameSaving::uploadLocalSlot() Slot save data upload completed for slotName: ") + model.slotName;
    Logging::Log(m_logCb, Level::Info, message.c_str());

    markSlotAsSyncedWithLocal(slot);

    return GAMEKIT_SUCCESS;
}

unsigned int GameSaving::downloadCloudSlot(GameSavingModel& model, CachedSlot& slot, unsigned int& outActualSlotSize)
{
    // Validate slot sync status
    unsigned int returnCode = validateSlotStatusForDownload(slot, model.overrideSync);
    if (returnCode != GAMEKIT_SUCCESS)
    {
        return returnCode;
    }

    // Construct a pre-signed S3 url for the slot
    std::string slotDownloadUrl;
    returnCode = getPresignedS3UrlForSlot(model.slotName, model.urlTimeToLive, slotDownloadUrl);
    if (returnCode != GAMEKIT_SUCCESS)
    {
        return returnCode;
    }

    // Download the slot from S3
    std::shared_ptr<Aws::Http::HttpResponse> response;
    returnCode = downloadSlotFromS3(slotDownloadUrl, response);
    if (returnCode != GAMEKIT_SUCCESS)
    {
        return returnCode;
    }

    // Stream for the slot contents
    Aws::IOStream& body = response->GetResponseBody();

    // Verify that the buffer size is large enough to contain the stream contents
    const std::char_traits<char>::pos_type begin = body.tellg();
    body.seekg(0, std::ios::end);
    const std::char_traits<char>::pos_type end = body.tellg();
    const unsigned int slotSize = (unsigned int)(end - begin);
    body.seekg(0, std::ios::beg);

    // If buffer size is smaller than downloaded slot size, return error
    if (model.dataSize < slotSize)
    {
        const std::string errorMessage = "Error: GameSaving::downloadCloudSlot() download cloud slot failed: Buffer too small : required = " + std::to_string(slotSize) +
            " bytes, found = " + std::to_string(model.dataSize) +" bytes";
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_GAME_SAVING_BUFFER_TOO_SMALL;
    }

    // Stream the slot into the designated data buffer
    body.read(reinterpret_cast<char*>(model.data), model.dataSize);
    outActualSlotSize = slotSize;

    // Synchronize the local timestamps with the cloud timestamps
    markSlotAsSyncedWithCloud(slot);

    return GAMEKIT_SUCCESS;
}

unsigned int GameSaving::updateLocalSlotStatus(CachedSlot& slot, const GameSavingModel& model)
{
    // Update the slot's local attributes based on the GameSavingModel
    const int64_t epochTime = model.epochTime == 0 ? m_currentTimeProvider->GetCurrentTimeMilliseconds() : model.epochTime;
    slot.lastModifiedLocal = Aws::Utils::DateTime(epochTime);
    slot.sizeLocal = model.dataSize;
    slot.metadataLocal = model.metadata;

    // Save the new information to a file
    const unsigned int statusCode = saveSlotInformation(slot, model.localSlotInformationFilePath);
    if (statusCode != GAMEKIT_SUCCESS)
    {
        const std::string errorMessage = "Error: GameSaving::updateLocalSlotStatus() unable to save slot information for slotName: " + slot.slotName;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return statusCode;
    }

    // Update the slot's sync status from the cloud
    return getSlotSyncStatusInternal(slot);
}

unsigned int GameSaving::saveSlotInformation(const Slot& slot, const char* filePath)
{
    const JsonValue json = CachedSlot(slot);
    const Aws::String fileContents = json.View().WriteCompact();

    // Write file
    std::vector<uint8_t> dataVector(fileContents.begin(), fileContents.end());
    bool success = m_fileWriteCallback(m_fileWriteDispatchReceiver, filePath, dataVector.data(), (const unsigned int)dataVector.size());

    return success ? GAMEKIT_SUCCESS : GAMEKIT_ERROR_FILE_WRITE_FAILED;
}

void GameSaving::loadSlotInformation(const char* const* localSlotInformationFilePaths, unsigned int arraySize)
{
    for (unsigned int i = 0; i < arraySize; ++i)
    {
        const char* path = localSlotInformationFilePaths[i];
        unsigned int size = m_fileSizeCallback(m_fileSizeDispatchReceiver, path);
        char* data = new char[size];

        if (!m_fileReadCallback(m_fileReadDispatchReceiver, path, (uint8_t*)data, size))
        {
            const std::string errorMessage = "Error: GameSaving::loadSlotInformation() unable to read slot information file: " + std::string(path);
            Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
            continue;
        }

        CachedSlot loadedSlot;
        Aws::String loadedString(data, data + size);
        delete[] data;

        const unsigned int parseStatus = loadedSlot.FromJson(loadedString);

        if (parseStatus != GAMEKIT_SUCCESS)
        {
            const std::string errorMessage = "Error: GameSaving::loadSlotInformation() unable to parse json from file: " + std::string(path);
            Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
            continue;
        }

        const std::string msg = "GameSaving:: loadSlotInformation() successfully loaded slot from " + std::string(path) + " into local slot.";
        Logging::Log(m_logCb, Level::Info, msg.c_str());
        m_syncedSlots[loadedSlot.slotName] = loadedSlot;
    }
}

unsigned int GameSaving::validateSlotStatusForDownload(CachedSlot& slot, const bool overrideSync) const
{
    if (overrideSync)
    {
        const std::string message = "GameSaving::validateSlotStatusForDownload() overriding local slot: " + std::string(slot.slotName);
        Logging::Log(m_logCb, Level::Info, message.c_str());
        return GAMEKIT_SUCCESS;
    }

    std::string message;
    switch (slot.slotSyncStatus)
    {
    case SlotSyncStatus::SHOULD_DOWNLOAD_CLOUD:
        message = "GameSaving::validateSlotStatusForDownload() local slot prepared for download: " + std::string(slot.slotName);
        Logging::Log(m_logCb, Level::Info, message.c_str());
        return GAMEKIT_SUCCESS;

    case SlotSyncStatus::SHOULD_UPLOAD_LOCAL:
        message = "Error: GameSaving::validateSlotStatusForDownload() local slot is ahead of cloud slot: " + std::string(slot.slotName);
        Logging::Log(m_logCb, Level::Error, message.c_str());
        return GAMEKIT_ERROR_GAME_SAVING_LOCAL_SLOT_IS_NEWER;

    case SlotSyncStatus::SYNCED:
        message = "Info: GameSaving::validateSlotStatusForDownload() local slot is already in sync with the cloud, will download again anyways: " + std::string(slot.slotName);
        Logging::Log(m_logCb, Level::Info, message.c_str());
        return GAMEKIT_SUCCESS;

    case SlotSyncStatus::IN_CONFLICT:
        message = "Error: GameSaving::validateSlotStatusForDownload() slot is in conflict: " + std::string(slot.slotName);
        Logging::Log(m_logCb, Level::Error, message.c_str());
        return GAMEKIT_ERROR_GAME_SAVING_SYNC_CONFLICT;

    case SlotSyncStatus::UNKNOWN:
        /* fall through */
    default:
        message = "Error: GameSaving::validateSlotStatusForDownload() unable to determine sync status for slot: " + std::string(slot.slotName);
        Logging::Log(m_logCb, Level::Error, message.c_str());
        return GAMEKIT_ERROR_GAME_SAVING_SLOT_UNKNOWN_SYNC_STATUS;
    }
}

unsigned int GameSaving::getPresignedS3UrlForSlot(const char* slotName, const unsigned int urlTtl, std::string& returnedS3Url) const
{
    std::stringstream urlTtlString;
    urlTtlString << urlTtl;
    const std::string lambdaFunctionUri = m_sessionManager->GetClientSettings()[ClientSettings::GameSaving::SETTINGS_GAME_SAVING_BASE_URL] + "/" + slotName + "/download_url?time_to_live=" + urlTtlString.str();

    JsonValue jsonBody;
    const unsigned int returnCode = m_caller.CallApiGateway(lambdaFunctionUri, Aws::Http::HttpMethod::HTTP_GET, "getPresignedS3UrlForSlot", jsonBody);
    if (returnCode != GAMEKIT_SUCCESS)
    {
        return returnCode;
    }

    const JsonView view = jsonBody.View();
    returnedS3Url = view.KeyExists("data") ? ToStdString(view.GetObject("data").GetString("url")) : "";
    if (returnedS3Url.empty())
    {
        const std::string errorMessage = "Error: GameSaving::getPresignedS3UrlForSlot() get presigned s3 url response formatted incorrectly or not found";
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_PARSE_JSON_FAILED;
    }
    return GAMEKIT_SUCCESS;
}

unsigned int GameSaving::downloadSlotFromS3(const std::string& presignedSlotDownloadUrl, std::shared_ptr<Aws::Http::HttpResponse>& returnedResponse) const
{
    const std::shared_ptr<Aws::Http::HttpRequest> request = CreateHttpRequest(ToAwsString(presignedSlotDownloadUrl), Aws::Http::HttpMethod::HTTP_GET, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
    const std::shared_ptr<Aws::Http::HttpResponse> response = m_httpClient->MakeRequest(request);

    if (response->GetResponseCode() != Aws::Http::HttpResponseCode::OK)
    {
        const std::string errorMessage = "Error: GameSaving::downloadSlotFromS3() download slot from s3 failed with http response code " + std::to_string(static_cast<int>(response->GetResponseCode()));
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_HTTP_REQUEST_FAILED;
    }

    if (!response->HasHeader(S3_SHA_256_METADATA_HEADER.c_str()))
    {
        const std::string errorMessage = "Error: GameSaving::downloadSlotFromS3() cannot determine validity of file as no SHA-256 was provided";
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_GAME_SAVING_MISSING_SHA;
    }

    const std::string providedSha = ToStdString(response->GetHeader(S3_SHA_256_METADATA_HEADER));
    const std::string expectedSha = getSha256(response->GetResponseBody());
    if (strcmp(providedSha.c_str(), expectedSha.c_str()) != 0)
    {
        const std::string errorMessage = "Error: GameSaving::downloadSlotFromS3() malformed SHA-256 " + providedSha + " found, expected " + expectedSha;
        Logging::Log(m_logCb, Level::Error, errorMessage.c_str());
        return GAMEKIT_ERROR_GAME_SAVING_SLOT_TAMPERED;
    }

    returnedResponse = response;
    return GAMEKIT_SUCCESS;
}

unsigned int GameSaving::addSlot(const std::string& slotName)
{
    // check if this is actually a new slot or is already cached, if new then set defaults
    if (m_syncedSlots.find(slotName) == m_syncedSlots.end())
    {
        CachedSlot slot;

        // record the slot name
        slot.slotName = slotName;

        // the following should be set to 0, the actual values will be added later
        slot.lastModifiedCloud = (int64_t)0;
        slot.lastModifiedLocal = (int64_t)0;
        slot.lastSync = (int64_t)0;
        slot.sizeCloud = 0;
        slot.sizeLocal = 0;

        // we do not know the status until we sync the slot, by default set to unknown
        slot.slotSyncStatus = SlotSyncStatus::UNKNOWN;

        // save the new slot
        m_syncedSlots[slot.slotName] = slot;
    }

    return GAMEKIT_SUCCESS;
}

unsigned int GameSaving::invokeCallback(DISPATCH_RECEIVER_HANDLE receiver, GameSavingResponseCallback resultCb, unsigned int callStatus) const
{
    std::vector<Slot> emptySlotVector;
    const bool isFinalCall = true;
    return invokeCallback(receiver, resultCb, emptySlotVector, isFinalCall, callStatus);
}

unsigned int GameSaving::invokeCallback(DISPATCH_RECEIVER_HANDLE receiver, GameSavingResponseCallback resultCb, std::vector<Slot>& singlePageOfSlots) const
{
    const bool isFinalCall = false;
    const unsigned int callStatus = GAMEKIT_SUCCESS;
    return invokeCallback(receiver, resultCb, singlePageOfSlots, isFinalCall, callStatus);
}

unsigned int GameSaving::invokeCallback(DISPATCH_RECEIVER_HANDLE receiver, GameSavingResponseCallback resultCb, bool waitForAllPages, std::unordered_set<std::string>& slotsFromCloud) const
{
    std::vector<Slot> returnedSlotList;
    returnedSlotList.reserve(m_syncedSlots.size());

    for (std::pair<const std::string, Slot> slotEntry : m_syncedSlots)
    {
        // if we are returning per page, then we only want to return any remaining slots (the local only slots) here, else return all.
        if (waitForAllPages || slotsFromCloud.find(slotEntry.first) == slotsFromCloud.end())
        {
            returnedSlotList.push_back(slotEntry.second);
        }
    }

    const bool isFinalCall = true;
    const unsigned int callStatus = GAMEKIT_SUCCESS;

    // call the resultCb with the final list of all slots.
    return invokeCallback(receiver, resultCb, returnedSlotList, isFinalCall, callStatus);
}

unsigned int GameSaving::invokeCallback(DISPATCH_RECEIVER_HANDLE receiver, GameSavingResponseCallback resultCb, std::vector<Slot>& returnedSlotList, bool isFinalCall, unsigned int callStatus) const
{
    if (!(receiver == nullptr) && !(resultCb == nullptr))
    {
        resultCb(receiver, returnedSlotList.data(), (unsigned int)returnedSlotList.size(), isFinalCall, callStatus);
    }

    return callStatus;
}

unsigned int GameSaving::invokeCallback(DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, unsigned int callStatus) const
{
    const Slot emptySlot{};
    return invokeCallback(receiver, resultCb, callStatus, emptySlot);
}

unsigned int GameSaving::invokeCallback(DISPATCH_RECEIVER_HANDLE receiver, GameSavingSlotActionResponseCallback resultCb, unsigned int callStatus, const Slot& actedOnSlot) const
{
    if (!(receiver == nullptr) && !(resultCb == nullptr))
    {
        std::vector<Slot> returnedSlotList;
        for (std::pair<const std::string, Slot> slotEntry : m_syncedSlots)
        {
            returnedSlotList.push_back(slotEntry.second);
        }

        resultCb(receiver, returnedSlotList.data(), (unsigned int)returnedSlotList.size(), actedOnSlot, callStatus);
    }

    return callStatus;
}

unsigned int GameSaving::invokeCallback(DISPATCH_RECEIVER_HANDLE receiver, GameSavingDataResponseCallback resultCb, unsigned int callStatus) const
{
    const Slot emptySlot{};
    const uint8_t* emptyData = nullptr;
    const unsigned int dataSize = 0;
    return invokeCallback(receiver, resultCb, callStatus, emptySlot, emptyData, dataSize);
}

unsigned int GameSaving::invokeCallback(
    DISPATCH_RECEIVER_HANDLE receiver,
    GameSavingDataResponseCallback resultCb,
    unsigned int callStatus,
    const Slot& actedOnSlot,
    const uint8_t* data,
    unsigned int dataSize) const
{
    if (!(receiver == nullptr) && !(resultCb == nullptr))
    {
        std::vector<Slot> returnedSlotList;
        for (std::pair<const std::string, Slot> slotEntry : m_syncedSlots)
        {
            returnedSlotList.push_back(slotEntry.second);
        }

        resultCb(receiver, returnedSlotList.data(), (unsigned int)returnedSlotList.size(), actedOnSlot, data, dataSize, callStatus);
    }

    return callStatus;
}

bool GameSaving::isValidCallback(DISPATCH_RECEIVER_HANDLE receiver, void* resultCb)
{
    return !(receiver == nullptr) && !(resultCb == nullptr);
}

std::string GameSaving::getSha256(std::iostream& buffer)
{
    // Ensure we're reading the buffer from the beginning
    buffer.seekg(0, std::ios::beg);

    Aws::Utils::Crypto::Sha256 sha256;
    const auto hashResult = sha256.Calculate(buffer);

    // Calculating the sha reads the whole stream; reset the buffer to the beginning
    buffer.seekg(0, std::ios::beg);

    // Convert the returned hash from a byte buffer into a readable base64 string
    const Aws::Utils::Base64::Base64 base64;
    return ToStdString(base64.Encode(hashResult.GetResult()));
}

void GameSaving::updateSlotFromJson(const JsonView& jsonBody, CachedSlot& returnedSlot)
{
    const std::string encodedMetadata = ToStdString(jsonBody.GetString("metadata"));
    returnedSlot.metadataCloud = EncodingUtils::DecodeBase64(encodedMetadata);
    returnedSlot.sizeCloud = stoi(ToStdString(jsonBody.GetString("size")));
    returnedSlot.lastModifiedCloud = Aws::Utils::DateTime(jsonBody.GetInt64("last_modified"));
}

void GameSaving::updateSlotSyncStatus(CachedSlot& returnedSlot)
{
    // SecondsWithMSPrecision returns in the format of seconds.milliseconds. We only want seconds for comparision so truncate off the milliseconds by casting to an int
    const unsigned int cloud = (unsigned int)returnedSlot.lastModifiedCloud.SecondsWithMSPrecision();
    const unsigned int local = (unsigned int)returnedSlot.lastModifiedLocal.SecondsWithMSPrecision();
    const unsigned int last = (unsigned int)returnedSlot.lastSync.SecondsWithMSPrecision();

    if (cloud == local && local == last)
    {
        returnedSlot.slotSyncStatus = SlotSyncStatus::SYNCED;
    }
    else if (cloud > local && local == last)
    {
        returnedSlot.slotSyncStatus = SlotSyncStatus::SHOULD_DOWNLOAD_CLOUD;
    }
    else if (local > cloud && cloud == last)
    {
        returnedSlot.slotSyncStatus = SlotSyncStatus::SHOULD_UPLOAD_LOCAL;
    }
    else
    {
        returnedSlot.slotSyncStatus = SlotSyncStatus::IN_CONFLICT;
    }
}

void GameSaving::markSlotAsSyncedWithLocal(CachedSlot& returnedSlot)
{
    returnedSlot.slotSyncStatus = SlotSyncStatus::SYNCED;
    returnedSlot.metadataCloud = returnedSlot.metadataLocal;
    returnedSlot.lastModifiedCloud = returnedSlot.lastModifiedLocal;
    returnedSlot.lastSync = returnedSlot.lastModifiedLocal;
    returnedSlot.sizeCloud = returnedSlot.sizeLocal;
}

void GameSaving::markSlotAsSyncedWithCloud(CachedSlot& returnedSlot)
{
    returnedSlot.slotSyncStatus = SlotSyncStatus::SYNCED;
    returnedSlot.metadataLocal = returnedSlot.metadataCloud;
    returnedSlot.lastModifiedLocal = returnedSlot.lastModifiedCloud;
    returnedSlot.lastSync = returnedSlot.lastModifiedCloud;
    returnedSlot.sizeLocal = returnedSlot.sizeCloud;
}

#pragma endregion
#pragma pop_macro("PAGE_SIZE")
