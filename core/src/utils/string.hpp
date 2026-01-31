#pragma once

#include "defines.hpp"
#include "utils/enum.hpp"

#include <stdarg.h>

struct Arena;
struct vec2;
struct vec3;
struct vec4;

// Non-owning view (pointer + length, 16 bytes)
struct String
{
    const u8 *str;
    u64       size;
};

// Fixed-capacity inline storage (no allocation needed)
template <u64 N>
struct Const_String
{
    u8  data[N];
    u64 size;

    operator String() const { return String{data, size}; }
};

enum class String_Match_Flags : u32
{
    NONE              = 0,
    CASE_INSENSITIVE  = (1 << 0),
    SLASH_INSENSITIVE = (1 << 1),
};
ENABLE_BITMASK(String_Match_Flags)

// Construction
#define STR(string) (String{(const u8 *)(string), sizeof(string) - 1})

VOLTRUM_API String str_from_cstr(const char *c);

FORCE_INLINE String
str(const u8 *s, u64 size)
{
    return String{s, size};
}

FORCE_INLINE String
str_zero()
{
    return String{nullptr, 0};
}

template <u64 N>
Const_String<N>
const_str_from_cstr(const char *s)
{
    Const_String<N> result = {};
    if (s)
    {
        u64 i = 0;
        while (s[i] && i < N)
        {
            result.data[i] = (u8)s[i];
            i++;
        }
        result.size = i;
    }
    return result;
}

template <u64 N>
Const_String<N>
const_str_from_str(String s)
{
    Const_String<N> result = {};
    u64             len    = (s.size < N) ? s.size : N;
    for (u64 i = 0; i < len; i++)
    {
        result.data[i] = s.str[i];
    }
    result.size = len;
    return result;
}

// Matching
VOLTRUM_API b8 str_match(String             a,
                         String             b,
                         String_Match_Flags flags = String_Match_Flags::NONE);

VOLTRUM_API u64
str_find_needle(String             haystack,
                u64                start,
                String             needle,
                String_Match_Flags flags = String_Match_Flags::NONE);

// Slicing (no allocation, returns views into the original)
VOLTRUM_API String str_prefix(String s, u64 size);
VOLTRUM_API String str_skip(String s, u64 amt);
VOLTRUM_API String str_substr(String s, u64 start, u64 len);
VOLTRUM_API String str_trim_whitespace(String s);

// Arena-allocated operations
// These are arena-agnostic: pass a scratch arena for temporary strings,
// or a persistent arena for strings that need to outlive the current scope.
VOLTRUM_API String str_copy(Arena *arena, String s);
VOLTRUM_API String str_cat(Arena *arena, String a, String b);
VOLTRUM_API String str_fmt(Arena *arena, const char *fmt, ...);
VOLTRUM_API String str_fmt_v(Arena *arena, const char *fmt, va_list args);

// Path helpers (views unless noted)
VOLTRUM_API String str_chop_last_slash(String s);
VOLTRUM_API String str_skip_last_slash(String s);
VOLTRUM_API String str_chop_last_dot(String s);
VOLTRUM_API String str_skip_last_dot(String s);
VOLTRUM_API String str_path_join(Arena *arena, String dir, String file);

// Search - returns (u64)-1 if not found
VOLTRUM_API u64 str_index_of(String s, u8 character);

// Hashing (FNV-1a)
VOLTRUM_API u64 str_hash(String s);

// Parsing
VOLTRUM_API b8 str_to_f32(String s, f32 *out);
VOLTRUM_API b8 str_to_f64(String s, f64 *out);
VOLTRUM_API b8 str_to_vec2(String s, vec2 *out);
VOLTRUM_API b8 str_to_vec3(String s, vec3 *out);
VOLTRUM_API b8 str_to_vec4(String s, vec4 *out);
VOLTRUM_API b8 str_to_bool(String s, b8 *out);
