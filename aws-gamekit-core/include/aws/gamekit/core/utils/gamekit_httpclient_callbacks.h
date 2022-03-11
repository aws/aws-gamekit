// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
 
#pragma once
 
typedef void* NETWORK_STATE_RECEIVER_HANDLE;
typedef void(*NetworkStatusChangeCallback)(NETWORK_STATE_RECEIVER_HANDLE dispatchReceiver, bool isConnectionOk, const char* connectionClient);
typedef void* CACHE_PROCESSED_RECEIVER_HANDLE;
typedef void(*CacheProcessedCallback)(CACHE_PROCESSED_RECEIVER_HANDLE dispatchReceiver, bool isCacheProcessed);