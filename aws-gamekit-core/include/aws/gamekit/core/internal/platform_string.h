// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

/** @file
 * @brief Helper macros to handle string conversions. Do not include this in public headers.
 *
 * @details Use these macros when you need to convert from std::string to Aws::String e.g.
 * if you have a foo std::string variable but you need to pass Aws::String to a function, do
 * func(ToAwsString(foo));
 */

#if defined(ANDROID) || defined(__ANDROID__)

// It is not possible to assign Aws::String <-> std::string in Android so it must be converted.

// Helper macro to convert Aws::String to std::string.
// Note: the value will go out of scope unless you assign it to a variable.
#define ToStdString(AwsStringVar)   (std::string(AwsStringVar.c_str()))

// Helper macro to convert std::string to Aws::String.
// Note: the value will go out of scope unless you assign it to a variable.
#define ToAwsString(StdStringVar)   (Aws::String(StdStringVar.c_str()))

#else

// Other platforms don't have that limitation so macros evaluate to the same source string.

#define ToStdString(AwsStringVar)   (AwsStringVar)
#define ToAwsString(StdStringVar)   (StdStringVar)

#endif
