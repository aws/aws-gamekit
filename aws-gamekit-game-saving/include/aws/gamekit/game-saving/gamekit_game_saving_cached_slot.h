// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// Standard Library
#include <string>

// AWS SDK
#include <aws/core/utils/DateTime.h>
#include <aws/core/utils/memory/stl/AWSString.h>

// GameKit
#include <aws/gamekit/core/internal/platform_string.h>
#include <aws/gamekit/game-saving/gamekit_game_saving_models.h>

using namespace Aws::Utils::Json;

namespace GameKit
{
    namespace GameSaving
    {
        class CachedSlot
        {
        private:

            // This check is necessary to avoid crashing the program in case any JSON key is missing.
            // Calling view.GetInt64("myKey") for a missing key leads to a program crash that cannot be caught/handled.
            // The crash cannot be caught because the JsonView code uses an "assert()" statement instead of raising an exception:
            // https://github.com/aws/aws-sdk-cpp/blob/bb1fdce01cc7e8ae2fe7162f24c8836e9d3ab0a2/aws-cpp-sdk-core/source/utils/json/JsonSerializer.cpp#L454
            static bool keysExist(const Aws::Utils::Json::JsonView& json)
            {
                return json.KeyExists("slotName") && json.ValueExists("slotName")
                    && json.KeyExists("metadataLocal")
                    && json.KeyExists("metadataCloud")
                    && json.KeyExists("sizeLocal") && json.ValueExists("sizeLocal")
                    && json.KeyExists("sizeCloud") && json.ValueExists("sizeCloud")
                    && json.KeyExists("lastModifiedLocal") && json.ValueExists("lastModifiedLocal")
                    && json.KeyExists("lastModifiedCloud") && json.ValueExists("lastModifiedCloud")
                    && json.KeyExists("lastSync") && json.ValueExists("lastSync")
                    && json.KeyExists("slotSyncStatus") && json.ValueExists("slotSyncStatus");
            }

        public:
            std::string slotName;
            std::string metadataLocal;
            std::string metadataCloud;
            int64_t sizeLocal;
            int64_t sizeCloud;

            // time in epoch milliseconds
            Aws::Utils::DateTime lastModifiedLocal;
            Aws::Utils::DateTime lastModifiedCloud;
            Aws::Utils::DateTime lastSync;

            SlotSyncStatus slotSyncStatus;

            CachedSlot() :
                slotName(std::string()),
                metadataLocal(std::string()),
                metadataCloud(std::string()),
                sizeLocal(0),
                sizeCloud(0),
                lastModifiedLocal(Aws::Utils::DateTime()),
                lastModifiedCloud(Aws::Utils::DateTime()),
                lastSync(Aws::Utils::DateTime()),
                slotSyncStatus(SlotSyncStatus::UNKNOWN)
            {

            }

            CachedSlot(Slot slot) :
                slotName(slot.slotName),
                metadataLocal(slot.metadataLocal),
                metadataCloud(slot.metadataCloud),
                sizeLocal(slot.sizeLocal),
                sizeCloud(slot.sizeCloud),
                slotSyncStatus(slot.slotSyncStatus)
            {
                lastModifiedLocal = (int64_t)slot.lastModifiedLocal;
                lastModifiedCloud = (int64_t)slot.lastModifiedCloud;
                lastSync = (int64_t)slot.lastSync;
            }

            operator Slot() const
            {
                Slot slot;
                slot.slotName = slotName.c_str();
                slot.metadataLocal = metadataLocal.c_str();
                slot.metadataCloud = metadataCloud.c_str();
                slot.sizeLocal = sizeLocal;
                slot.sizeCloud = sizeCloud;
                slot.lastModifiedLocal = lastModifiedLocal.Millis();
                slot.lastModifiedCloud = lastModifiedCloud.Millis();
                slot.lastSync = lastSync.Millis();
                slot.slotSyncStatus = slotSyncStatus;

                return slot;
            }

            operator JsonValue() const
            {
                return JsonValue()
                    .WithString("slotName", ToAwsString(slotName))
                    .WithString("metadataLocal", ToAwsString(metadataLocal))
                    .WithString("metadataCloud", ToAwsString(metadataCloud))
                    .WithInt64("sizeLocal", sizeLocal)
                    .WithInt64("sizeCloud", sizeCloud)
                    .WithInt64("lastModifiedLocal", lastModifiedLocal.Millis())
                    .WithInt64("lastModifiedCloud", lastModifiedCloud.Millis())
                    .WithInt64("lastSync", lastSync.Millis())
                    .WithInteger("slotSyncStatus", static_cast<int>(slotSyncStatus));
            }

            unsigned int FromJson(const JsonValue& json)
            {
                if (json.WasParseSuccessful() && keysExist(json))
                {
                    const JsonView view = json.View();

                    slotName = ToStdString(view.GetString("slotName"));
                    metadataLocal = ToStdString(view.GetString("metadataLocal"));
                    metadataCloud = ToStdString(view.GetString("metadataCloud"));
                    sizeLocal = view.GetInt64("sizeLocal");
                    sizeCloud = view.GetInt64("sizeCloud");
                    lastModifiedLocal = view.GetInt64("lastModifiedLocal");
                    lastModifiedCloud = view.GetInt64("lastModifiedCloud");
                    lastSync = view.GetInt64("lastSync");
                    slotSyncStatus = static_cast<SlotSyncStatus>(view.GetInteger("slotSyncStatus"));

                    return GAMEKIT_SUCCESS;
                }
                else
                {
                    return GAMEKIT_ERROR_PARSE_JSON_FAILED;
                }
            }
        };
    }
}
