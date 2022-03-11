// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aws/gamekit/core/utils/validation_utils.h>

namespace GameKit
{
	namespace Utils
	{
		static const int MIN_USERNAME_CHARS = 2;
		static const int MAX_USERNAME_CHARS = 2048;
		static const int MIN_PASSWORD_CHARS = 8;
		static const int MAX_PASSWORD_CHARS = 98;
		static const std::string USERNAME_REQUIREMENTS_TEXT = "Username must contain between " + std::to_string(MIN_USERNAME_CHARS) + " and " + std::to_string(MAX_USERNAME_CHARS) + " characters";
		static const std::string PASSWORD_REGEX = "[a-zA-Z0-9^$*.[\\]{}()?\"!@#%&/\\\\,><':;|_~`]+";
		static const std::string PASSWORD_REQUIREMENTS_TEXT = "Password must contain between "
			+ std::to_string(MIN_PASSWORD_CHARS) + " and "
			+ std::to_string(MAX_PASSWORD_CHARS)
			+ " characters, and may only contain the characters \"a - z\" and \"A - Z\", the numbers \"0 - 9\", and symbols ^$*.[]{}()?\"!@#%&/\\,><':;|_~`]+";

		class GAMEKIT_API CredentialsUtils
		{
		public:
			static bool IsValidUsername(const std::string& username);
			static bool IsValidPassword(const std::string& password);
		};
	}
}
