// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>

#ifndef _WIN32
#include <boost/filesystem.hpp>
#endif

#ifdef _WIN32
#include <boost/winapi/character_code_conversion.hpp>

std::locale GameKitInternal::BoostFilesystemUtf8Initializer::GetLocale()
{
    // The standard library's codecvt_utf8_utf16 class will return an error upon hitting invalid sequences,
    // which causes boost::filesystem::path to throw an exception. Instead, we want the Win32 behavior of
    // MultiByteToWideChar which converts all invalid sequences to U+FFFD and then resumes conversion after.
    // This ensures that boost::filesystem::path will not throw on bad input.
    //
    // NOTE: this is only a partial codecvt implementation that must not be used outside of boost::filesystem!
    class win32_utf8_codecvt : public std::codecvt<wchar_t, char, std::mbstate_t>
    {
    public:
        explicit win32_utf8_codecvt(std::size_t refs = 0) : std::codecvt<wchar_t, char, std::mbstate_t>(refs) { }
    protected:
        result do_in(std::mbstate_t&, const char* from, const char* from_end, const char*& from_next, wchar_t* to, wchar_t* to_end, wchar_t*& to_next) const override
        {
            using boost::winapi::CP_UTF8_;
            int required = (from_end == from) ? 0 : MultiByteToWideChar(CP_UTF8_, 0, from, (int)(from_end - from), nullptr, 0);
            if (required != 0 && (size_t)required <= to_end - to)
            {
                MultiByteToWideChar(CP_UTF8_, 0, from, (int)(from_end - from), to, required);
            }
            from_next = from_end;
            to_next = to + required;
            return ok;
        }
        result do_out(std::mbstate_t&, const wchar_t* from, const wchar_t* from_end, const wchar_t*& from_next, char* to, char* to_end, char*& to_next) const override
        {
            using boost::winapi::CP_UTF8_;
            int required = (from_end == from) ? 0 : WideCharToMultiByte(CP_UTF8_, 0, from, (int)(from_end - from), nullptr, 0, nullptr, nullptr);
            if (required != 0 && (size_t)required <= to_end - to)
            {
                WideCharToMultiByte(CP_UTF8_, 0, from, (int)(from_end - from), to, required, nullptr, nullptr);
            }
            from_next = from_end;
            to_next = to + required;
            return ok;
        }
        result do_unshift(std::mbstate_t&, char* from, char* from_end, char*& from_next) const override
        {
            from_next = from;
            return noconv;
        }
        int do_length(std::mbstate_t&, const char*, const char*, std::size_t) const noexcept override { return 0; }
        bool do_always_noconv() const noexcept override { return false; }
        int do_max_length() const noexcept override { return 4; }
        int do_encoding() const noexcept override { return 0; }
    };

    return std::locale(std::locale(), new win32_utf8_codecvt());
}
#endif
