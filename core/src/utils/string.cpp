#include "utils/string.hpp"

#include "math/math_types.hpp"
#include "memory/arena.hpp"
#include "memory/memory.hpp"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

// Construction
String
str_from_cstr(const char *c)
{
    String result = {};
    if (c)
    {
        u64 len = 0;
        while (c[len])
        {
            len++;
        }
        result.str  = (const u8 *)c;
        result.size = len;
    }
    return result;
}

// Matching
INTERNAL_FUNC u8
char_to_lower(u8 c)
{
    return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

INTERNAL_FUNC u8
char_to_forward_slash(u8 c)
{
    return (c == '\\') ? '/' : c;
}

b8
str_match(String a, String b, String_Match_Flags flags)
{
    if (a.size != b.size)
        return false;

    b8 case_insensitive = (flags & String_Match_Flags::CASE_INSENSITIVE) !=
                          String_Match_Flags::NONE;

    b8 slash_insensitive = (flags & String_Match_Flags::SLASH_INSENSITIVE) !=
                           String_Match_Flags::NONE;

    for (u64 i = 0; i < a.size; i++)
    {
        u8 ca = a.str[i];
        u8 cb = b.str[i];

        if (case_insensitive)
        {
            ca = char_to_lower(ca);
            cb = char_to_lower(cb);
        }
        if (slash_insensitive)
        {
            ca = char_to_forward_slash(ca);
            cb = char_to_forward_slash(cb);
        }

        if (ca != cb)
            return false;
    }

    return true;
}

u64
str_find_needle(String             haystack,
                u64                start,
                String             needle,
                String_Match_Flags flags)
{
    if (needle.size == 0 || start + needle.size > haystack.size)
        return (u64)-1;

    b8 case_insensitive = (flags & String_Match_Flags::CASE_INSENSITIVE) !=
                          String_Match_Flags::NONE;

    b8 slash_insensitive = (flags & String_Match_Flags::SLASH_INSENSITIVE) !=
                           String_Match_Flags::NONE;

    for (u64 i = start; i + needle.size <= haystack.size; i++)
    {
        b8 found = true;
        for (u64 j = 0; j < needle.size; j++)
        {
            u8 ch = haystack.str[i + j];
            u8 cn = needle.str[j];

            if (case_insensitive)
            {
                ch = char_to_lower(ch);
                cn = char_to_lower(cn);
            }
            if (slash_insensitive)
            {
                ch = char_to_forward_slash(ch);
                cn = char_to_forward_slash(cn);
            }

            if (ch != cn)
            {
                found = false;
                break;
            }
        }

        if (found)
            return i;
    }

    return (u64)-1;
}

// Slicing
String
str_prefix(String s, u64 size)
{
    u64 clamped = (size < s.size) ? size : s.size;
    return String{s.str, clamped};
}

String
str_skip(String s, u64 amt)
{
    u64 clamped = (amt < s.size) ? amt : s.size;
    return String{s.str + clamped, s.size - clamped};
}

String
str_substr(String s, u64 start, u64 len)
{
    if (start >= s.size)
        return str_zero();

    u64 max_len = s.size - start;
    u64 clamped = (len < max_len) ? len : max_len;
    return String{s.str + start, clamped};
}

String
str_trim_whitespace(String s)
{
    u64 start = 0;
    while (start < s.size && isspace(s.str[start]))
        start++;

    u64 end = s.size;
    while (end > start && isspace(s.str[end - 1]))
        end--;

    return String{s.str + start, end - start};
}

// Arena-allocated operations
String
str_copy(Arena *arena, String s)
{
    if (s.size == 0 || !s.str)
        return str_zero();

    u8 *buf = push_array(arena, u8, s.size);
    memory_copy(buf, s.str, s.size);
    return String{buf, s.size};
}

String
str_cat(Arena *arena, String a, String b)
{
    u64 total = a.size + b.size;
    if (total == 0)
        return str_zero();

    u8 *buf = push_array(arena, u8, total);

    if (a.size > 0)
        memory_copy(buf, a.str, a.size);

    if (b.size > 0)
        memory_copy(buf + a.size, b.str, b.size);

    return String{buf, total};
}

String
str_fmt(Arena *arena, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    String result = str_fmt_v(arena, fmt, args);
    va_end(args);
    return result;
}

String
str_fmt_v(Arena *arena, const char *fmt, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);
    s32 len = vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);

    if (len <= 0)
        return str_zero();

    u8 *buf = push_array(arena, u8, (u64)len + 1);
    vsnprintf((char *)buf, (u64)len + 1, fmt, args);

    return String{buf, (u64)len};
}

// Path helpers
INTERNAL_FUNC u64
find_last_slash(String s)
{
    u64 pos = (u64)-1;
    for (u64 i = 0; i < s.size; i++)
    {
        if (s.str[i] == '/' || s.str[i] == '\\')
            pos = i;
    }
    return pos;
}

String
str_chop_last_slash(String s)
{
    u64 pos = find_last_slash(s);
    if (pos == (u64)-1)
        return s;

    return String{s.str, pos};
}

String
str_skip_last_slash(String s)
{
    u64 pos = find_last_slash(s);
    if (pos == (u64)-1)
        return s;

    return String{s.str + pos + 1, s.size - pos - 1};
}

String
str_chop_last_dot(String s)
{
    u64 pos = (u64)-1;
    for (u64 i = 0; i < s.size; i++)
    {
        if (s.str[i] == '.')
            pos = i;
    }

    if (pos == (u64)-1)
        return s;

    return String{s.str, pos};
}

String
str_skip_last_dot(String s)
{
    u64 pos = (u64)-1;
    for (u64 i = 0; i < s.size; i++)
    {
        if (s.str[i] == '.')
            pos = i;
    }

    if (pos == (u64)-1)
        return str_zero();

    return String{s.str + pos + 1, s.size - pos - 1};
}

String
str_path_join(Arena *arena, String dir, String file)
{
    if (dir.size == 0)
        return str_copy(arena, file);

    if (file.size == 0)
        return str_copy(arena, dir);

    u8 last = dir.str[dir.size - 1];
    if (last == '/' || last == '\\')
        return str_cat(arena, dir, file);

    u64 total = dir.size + 1 + file.size;
    u8 *buf   = push_array(arena, u8, total);
    memory_copy(buf, dir.str, dir.size);

    buf[dir.size] = '/';
    memory_copy(buf + dir.size + 1, file.str, file.size);

    return String{buf, total};
}

// Search
u64
str_index_of(String s, u8 character)
{
    for (u64 i = 0; i < s.size; i++)
    {
        if (s.str[i] == character)
            return i;
    }
    return (u64)-1;
}

// Hashing (FNV-1a)
u64
str_hash(String s)
{
    u64 hash = 14695981039346656037ULL;
    for (u64 i = 0; i < s.size; i++)
    {
        hash ^= (u64)s.str[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

// Parsing - uses stack buffer to null-terminate for sscanf
INTERNAL_FUNC b8
make_cstr_buffer(String s, char *buf, u64 buf_size)
{
    if (s.size == 0 || !s.str)
        return false;

    if (s.size >= buf_size)
        return false;

    memory_copy(buf, s.str, s.size);
    buf[s.size] = '\0';

    return true;
}

b8
str_to_f32(String s, f32 *out)
{
    if (!out)
        return false;

    char buf[128];
    if (!make_cstr_buffer(s, buf, sizeof(buf)))
        return false;

    return sscanf(buf, "%f", out) == 1;
}

b8
str_to_f64(String s, f64 *out)
{
    if (!out)
        return false;

    char buf[128];
    if (!make_cstr_buffer(s, buf, sizeof(buf)))
        return false;

    return sscanf(buf, "%lf", out) == 1;
}

b8
str_to_vec2(String s, vec2 *out)
{
    if (!out)
        return false;

    char buf[256];
    if (!make_cstr_buffer(s, buf, sizeof(buf)))
        return false;

    return sscanf(buf, "%f %f", &out->x, &out->y) == 2;
}

b8
str_to_vec3(String s, vec3 *out)
{
    if (!out)
        return false;

    char buf[256];
    if (!make_cstr_buffer(s, buf, sizeof(buf)))
        return false;

    return sscanf(buf, "%f %f %f", &out->x, &out->y, &out->z) == 3;
}

b8
str_to_vec4(String s, vec4 *out)
{
    if (!out)
        return false;

    char buf[256];
    if (!make_cstr_buffer(s, buf, sizeof(buf)))
        return false;

    return sscanf(buf, "%f %f %f %f", &out->x, &out->y, &out->z, &out->w) == 4;
}

b8
str_to_bool(String s, b8 *out)
{
    if (!out || s.size == 0 || !s.str)
        return false;

    String trimmed = str_trim_whitespace(s);

    if (str_match(trimmed, STR_LIT("true"), String_Match_Flags::CASE_INSENSITIVE) ||
        str_match(trimmed, STR_LIT("1")))
    {
        *out = true;
        return true;
    }

    if (str_match(trimmed,
                  STR_LIT("false"),
                  String_Match_Flags::CASE_INSENSITIVE) ||
        str_match(trimmed, STR_LIT("0")))
    {
        *out = false;
        return true;
    }

    return false;
}
