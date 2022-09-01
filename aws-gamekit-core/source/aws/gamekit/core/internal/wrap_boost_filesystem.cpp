// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include <aws/gamekit/core/internal/wrap_boost_filesystem.h>

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
    //
    // Rules for adding the Win32 "very long filename" prefix:
    // - converted UTF-16 strings always have prefixes when they are absolute or UNC paths
    // - converted UTF-16 strings with prefixes always use backslashes, never forward slashes
    // - converted UTF-8 strings never have prefixes (for legibility when printing/logging)
    //
    // Technically this means that path strings do not round-trip cleanly from UTF-8 to UTF-16 and back
    // again to UTF-8; absolute paths could lose an existing '\\?\' prefix, and forward slashes will be
    // converted to backslashes. In practice this does not matter since nobody should do raw comparison
    // of UTF-8 paths strings to test for equality - you should use a filesystem helper to determine if
    // two paths refer to the same file or not, like boost::path::equivalent().
    //
    // This also means that strings get longer when converting from UTF-8 to UTF-16! Boost provides
    // a conversion buffer which has at least (UTF-8 byte length * 3) wchar_ts of space. This is
    // sufficient since we only prefix absolute paths, and absolute paths start with three ASCII
    // characters ("C", ":", "\") using one wchar_t each, leaving us with 6 more wchar_ts to fill.
    // Similar logic applies for the conversion of \\HOST to \\?\UNC\HOST ("?\UNC\" is 6 wchar_ts).
    //
    class win32_utf8_codecvt : public std::codecvt<wchar_t, char, std::mbstate_t>
    {
    public:
        explicit win32_utf8_codecvt(std::size_t refs = 0) : std::codecvt<wchar_t, char, std::mbstate_t>(refs) { }
    protected:
        static inline bool is_ascii_alpha(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); }
        static inline bool is_ascii_alnum(char c) { return (c >= '0' && c <= '9') || is_ascii_alpha(c); }
        static inline bool is_ascii_slash(char c) { return c == '/' || c == '\\'; }
        static inline bool is_unprefixed_abs_path(const char* p, const char* pend)
        {
            return (pend - p) >= 3 && is_ascii_alpha(p[0]) && p[1] == ':' && is_ascii_slash(p[2]);
        }
        static inline bool is_unprefixed_unc_path(const char* p, const char* pend)
        {
            return (pend - p) >= 3 && is_ascii_slash(p[0]) && is_ascii_slash(p[1]) && is_ascii_alnum(p[2]);
        }
        static inline bool is_prefixed_long_path(const char *p, const char* pend)
        {
            return (pend - p) >= 4 && memcmp(p, "\\\\?\\", 4) == 0;
        }

        result do_in(std::mbstate_t&, const char* from, const char* from_end, const char*& from_next, wchar_t* to, wchar_t* to_end, wchar_t*& to_next) const override
        {
            using boost::winapi::CP_UTF8_;
            int required = (from_end == from) ? 0 : MultiByteToWideChar(CP_UTF8_, 0, from, (int)(from_end - from), nullptr, 0);
            if (required != 0 && (size_t)required <= to_end - to)
            {
                bool force_backslashes = false;
                if (is_prefixed_long_path(from, from_end))
                {
                    force_backslashes = true;
                }
                else if (is_unprefixed_abs_path(from, from_end) && (size_t)required + 4 <= (to_end - to))
                {
                    // add \\?\ prefix to absolute path with local drive letter
                    memcpy(to, L"\\\\?\\", 4*sizeof(wchar_t));
                    to += 4;
                    force_backslashes = true;
                }
                else if (is_unprefixed_unc_path(from, from_end) && (size_t)required + 7 <= (to_end - to))
                {
                    // modify \\HOST\Share to long-path-prefixed \\?\UNC\HOST\Share
                    memcpy(to, L"\\\\?\\UNC", 7*sizeof(wchar_t));
                    to += 7;
                    from++, required--; // skip first slash in \\HOST to end up with \\?\UNC\HOST
                    force_backslashes = true;
                }
                
                MultiByteToWideChar(CP_UTF8_, 0, from, (int)(from_end - from), to, required);
                
                if (force_backslashes)
                {
                    for (int i = 0; i < required; ++i)
                    {
                        if (to[i] == L'/')
                        {
                            to[i] = L'\\';
                        }
                    }
                }
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

                if (required >= 8 && memcmp(to, "\\\\?\\UNC\\", 8) == 0)
                {
                    // Convert \\?\UNC\HOST to \\HOST by stripping "?\UNC\" middle bit
                    memmove(to + 2, to + 8, required - 8);
                    required -= 6;
                }
                else if (required >= 4 && memcmp(to, "\\\\?\\", 4) == 0)
                {
                    // Convert \\?\C:\Path to C:\Path by stripping "\\?\" prefix
                    memmove(to, to + 4, required - 4);
                    required -= 4;
                }
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
