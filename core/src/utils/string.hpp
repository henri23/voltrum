#pragma once

#include "defines.hpp"
#include "utils/enum.hpp"

#include <stdarg.h>

struct Arena;
struct vec2;
struct vec3;
struct vec4;

// Non-owning view (pointer + length, 16 bytes).
// The pointed buffer is expected to be null-terminated.
struct String
{
    char *buff;
    u64   size;
};

enum class String_Match_Flags : u32
{
    NONE              = 0,
    CASE_INSENSITIVE  = (1 << 0),
    SLASH_INSENSITIVE = (1 << 1),
};
ENABLE_BITMASK(String_Match_Flags)

// RAD-like helpers.
#define STR_LIT(string) (String{(char *)(string), sizeof(string) - 1})

// Construction from const char* (runtime length scan).
#define STR(cstr) str(cstr)

FORCE_INLINE String
str(char *buff, u64 size)
{
    return String{buff, size};
}

FORCE_INLINE String
str(const char *cstr)
{
    if (!cstr)
    {
        return String{nullptr, 0};
    }

    u64 len = 0;
    while (cstr[len] != '\0')
    {
        len += 1;
    }

    return String{(char *)cstr, len};
}

FORCE_INLINE String
string_empty()
{
    return String{nullptr, 0};
}

FORCE_INLINE String
string_capped(char *cstr, char *cap)
{
    if (!cstr || !cap || cap < cstr)
    {
        return string_empty();
    }

    u64 len = 0;
    while ((cstr + len) < cap && cstr[len] != '\0')
    {
        len += 1;
    }

    return String{cstr, len};
}

FORCE_INLINE String
string_capped(const char *cstr, const char *cap)
{
    return string_capped((char *)cstr, (char *)cap);
}

template <u64 N>
FORCE_INLINE void
string_set(char (&dst)[N], String src)
{
    if (N == 0)
    {
        return;
    }

    if (!src.buff || src.size == 0)
    {
        dst[0] = '\0';
        return;
    }

    const u64 copy_size = (src.size < (N - 1)) ? src.size : (N - 1);
    for (u64 i = 0; i < copy_size; ++i)
    {
        dst[i] = src.buff[i];
    }
    dst[copy_size] = '\0';
}

template <u64 N>
FORCE_INLINE void
string_set(char (&dst)[N], const char *src)
{
    string_set(dst, str(src));
}

// Matching
VOLTRUM_API b8 string_match(String             a,
                         String             b,
                         String_Match_Flags flags = String_Match_Flags::NONE);

VOLTRUM_API u64
string_find(String             haystack,
                u64                start,
                String             needle,
                String_Match_Flags flags = String_Match_Flags::NONE);

// Slicing (no allocation, returns views into the original).
// Clipping operations are destructive and place a null-terminator.
VOLTRUM_API String string_prefix(String s, u64 size);
VOLTRUM_API String string_skip(String s, u64 amt);
VOLTRUM_API String string_substr(String s, u64 start, u64 len);
VOLTRUM_API String string_trim_whitespace(String s);

// Arena-allocated operations
VOLTRUM_API String string_copy(Arena *arena, String s);
VOLTRUM_API String string_cat(Arena *arena, String a, String b);
VOLTRUM_API String string_fmt(Arena *arena, const char *fmt, ...);
VOLTRUM_API String string_fmt_v(Arena *arena, const char *fmt, va_list args);

// Path helpers (views unless noted)
VOLTRUM_API String string_chop_last_slash(String s);
VOLTRUM_API String string_skip_last_slash(String s);
VOLTRUM_API String string_chop_last_dot(String s);
VOLTRUM_API String string_skip_last_dot(String s);
VOLTRUM_API String string_path_join(Arena *arena, String dir, String file);

// Search - returns (u64)-1 if not found
VOLTRUM_API u64 string_index_of(String s, char character);

// Hashing (FNV-1a)
VOLTRUM_API u64 string_hash(String s);

// Parsing
VOLTRUM_API b8 string_to_f32(String s, f32 *out);
VOLTRUM_API b8 string_to_f64(String s, f64 *out);
VOLTRUM_API b8 string_to_vec2(String s, vec2 *out);
VOLTRUM_API b8 string_to_vec3(String s, vec3 *out);
VOLTRUM_API b8 string_to_vec4(String s, vec4 *out);
VOLTRUM_API b8 string_to_bool(String s, b8 *out);
