// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

// AWS SDK
#include <aws/core/http/HttpClient.h>

// GameKit
#include <aws/gamekit/core/gamekit_feature.h>
#include <aws/gamekit/core/logging.h>
#include <aws/gamekit/authentication/gamekit_session_manager.h>

namespace GameKit
{
    namespace GameSaving
    {
        static const Aws::String RESPONSE_BODY_KEY_META = "meta";
        static const Aws::String RESPONSE_BODY_KEY_META_MESSAGE = "message";

        enum class ResponseStatus {
            MALFORMED_SLOT_NAME,
            MAX_METADATA_BYTES_EXCEEDED,
            MALFORMED_HASH_SIZE_MISMATCH,
            MAX_CLOUD_SAVE_SLOTS_EXCEEDED,
            GENERIC_STATUS
        };

        inline std::string GetResponseStatusString(ResponseStatus status)
        {
            switch (status)
            {
            case ResponseStatus::MALFORMED_SLOT_NAME: return "Malformed Slot Name";
            case ResponseStatus::MAX_METADATA_BYTES_EXCEEDED: return "Max Metadata Bytes Exceeded";
            case ResponseStatus::MALFORMED_HASH_SIZE_MISMATCH: return "Malformed Hash Size Mismatch";
            case ResponseStatus::MAX_CLOUD_SAVE_SLOTS_EXCEEDED: return "Max Cloud Save Slots Exceeded";
            case ResponseStatus::GENERIC_STATUS: return "Unexpected Error";
            default: return "Unexpected Error";
            }
        }

        inline ResponseStatus GetResponseStatusFromString(const std::string& status)
        {
            if (status == "Malformed Slot Name") return ResponseStatus::MALFORMED_SLOT_NAME;
            if (status == "Max Metadata Bytes Exceeded") return ResponseStatus::MAX_METADATA_BYTES_EXCEEDED;
            if (status == "Malformed Hash Size Mismatch") return ResponseStatus::MALFORMED_HASH_SIZE_MISMATCH;
            if (status == "Max Cloud Save Slots Exceeded") return ResponseStatus::MAX_CLOUD_SAVE_SLOTS_EXCEEDED;
            if (status == "Unexpected Error") return ResponseStatus::GENERIC_STATUS;

            return ResponseStatus::GENERIC_STATUS;
        }

        class Caller
        {
        private:
            Authentication::GameKitSessionManager* m_sessionManager = nullptr;
            FuncLogCallback m_logCb = nullptr;
            std::shared_ptr<Aws::Http::HttpClient>* m_httpClient = nullptr;

        public:
            typedef std::unordered_map<std::string, std::string> CallerParams;
            typedef std::chrono::milliseconds MillisecondDelay;

            void Initialize(Authentication::GameKitSessionManager* sessionManager, FuncLogCallback logCb, std::shared_ptr<Aws::Http::HttpClient>* httpClientPointer);

            unsigned int CallApiGateway(
                const std::string& uri,
                Aws::Http::HttpMethod method,
                const std::string& currentFunctionName,
                Aws::Utils::Json::JsonValue& returnedJsonValue,
                const CallerParams& queryStringParams = CallerParams(),
                const CallerParams& headerParams = CallerParams()) const;
        };
    }
}
