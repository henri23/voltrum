#include "utils/string.hpp"

#include "core/asserts.hpp"
#include "math/math_types.hpp"
#include "memory/arena.hpp"
#include "memory/memory.hpp"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

// Matching helpers
INTERNAL_FUNC char
char_to_lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}

INTERNAL_FUNC char
char_to_forward_slash(char c)
{
    return (c == '\\') ? '/' : c;
}

b8
string_match(String a, String b, String_Match_Flags flags)
{
    if (a.size != b.size)
    {
        return false;
    }

    b8 case_insensitive = (flags & String_Match_Flags::CASE_INSENSITIVE) !=
                          String_Match_Flags::NONE;

    b8 slash_insensitive = (flags & String_Match_Flags::SLASH_INSENSITIVE) !=
                           String_Match_Flags::NONE;

    for (u64 i = 0; i < a.size; i++)
    {
        char ca = a.buff[i];
        char cb = b.buff[i];

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
        {
            return false;
        }
    }

    return true;
}

u64
string_find(String             haystack,
                u64                start,
                String             needle,
                String_Match_Flags flags)
{
    if (needle.size == 0 || start + needle.size > haystack.size)
    {
        return (u64)-1;
    }

    b8 case_insensitive = (flags & String_Match_Flags::CASE_INSENSITIVE) !=
                          String_Match_Flags::NONE;

    b8 slash_insensitive = (flags & String_Match_Flags::SLASH_INSENSITIVE) !=
                           String_Match_Flags::NONE;

    for (u64 i = start; i + needle.size <= haystack.size; i++)
    {
        b8 found = true;
        for (u64 j = 0; j < needle.size; j++)
        {
            char ch = haystack.buff[i + j];
            char cn = needle.buff[j];

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
        {
            return i;
        }
    }

    return (u64)-1;
}

// Slicing
String
string_prefix(String s, u64 size)
{
    if (!s.buff)
    {
        return string_empty();
    }

    u64 clamped = (size < s.size) ? size : s.size;
    if (clamped < s.size)
    {
        s.buff[clamped] = '\0';
    }

    return String{s.buff, clamped};
}

String
string_skip(String s, u64 amt)
{
    if (!s.buff)
    {
        return string_empty();
    }

    u64 clamped = (amt < s.size) ? amt : s.size;
    return String{s.buff + clamped, s.size - clamped};
}

String
string_substr(String s, u64 start, u64 len)
{
    if (!s.buff || start >= s.size)
    {
        return string_empty();
    }

    String skipped = string_skip(s, start);
    return string_prefix(skipped, len);
}

String
string_trim_whitespace(String s)
{
    if (!s.buff)
    {
        return string_empty();
    }

    u64 start = 0;
    while (start < s.size && isspace((unsigned char)s.buff[start]))
    {
        start += 1;
    }

    u64 end = s.size;
    while (end > start && isspace((unsigned char)s.buff[end - 1]))
    {
        end -= 1;
    }

    if (end < s.size)
    {
        s.buff[end] = '\0';
    }

    return String{s.buff + start, end - start};
}

// Arena-allocated operations
String
string_copy(Arena *arena, String s)
{
    ENSURE(arena);

    if (s.size > 0 && !s.buff)
    {
        return string_empty();
    }

    char *buf = push_array(arena, char, s.size + 1);
    if (s.size > 0 && s.buff)
    {
        memory_copy(buf, s.buff, s.size);
    }
    buf[s.size] = 0;
    return String{buf, s.size};
}

String
string_cat(Arena *arena, String a, String b)
{
    if (!arena)
    {
        return string_empty();
    }

    if ((a.size > 0 && !a.buff) || (b.size > 0 && !b.buff))
    {
        return string_empty();
    }

    u64 total = a.size + b.size;
    char *buf = push_array(arena, char, total + 1);

    if (a.size > 0)
    {
        memory_copy(buf, a.buff, a.size);
    }

    if (b.size > 0)
    {
        memory_copy(buf + a.size, b.buff, b.size);
    }

    buf[total] = 0;
    return String{buf, total};
}

String
string_fmt(Arena *arena, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    String result = string_fmt_v(arena, fmt, args);
    va_end(args);
    return result;
}

String
string_fmt_v(Arena *arena, const char *fmt, va_list args)
{
    if (!arena || !fmt)
    {
        return string_empty();
    }

    va_list args_copy;
    va_copy(args_copy, args);
    s32 len = vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);

    if (len < 0)
    {
        return string_empty();
    }

    u64 needed = (u64)len + 1;
    char *buf  = push_array(arena, char, needed);
    vsnprintf(buf, needed, fmt, args);

    return String{buf, (u64)len};
}

// Path helpers
INTERNAL_FUNC u64
find_last_slash(String s)
{
    if (!s.buff)
    {
        return (u64)-1;
    }

    u64 pos = (u64)-1;
    for (u64 i = 0; i < s.size; i++)
    {
        if (s.buff[i] == '/' || s.buff[i] == '\\')
        {
            pos = i;
        }
    }
    return pos;
}

String
string_chop_last_slash(String s)
{
    u64 pos = find_last_slash(s);
    if (pos == (u64)-1)
    {
        return s;
    }

    s.buff[pos] = '\0';
    return String{s.buff, pos};
}

String
string_skip_last_slash(String s)
{
    u64 pos = find_last_slash(s);
    if (pos == (u64)-1)
    {
        return s;
    }

    return String{s.buff + pos + 1, s.size - pos - 1};
}

String
string_chop_last_dot(String s)
{
    if (!s.buff)
    {
        return string_empty();
    }

    u64 pos = (u64)-1;
    for (u64 i = 0; i < s.size; i++)
    {
        if (s.buff[i] == '.')
        {
            pos = i;
        }
    }

    if (pos == (u64)-1)
    {
        return s;
    }

    s.buff[pos] = '\0';
    return String{s.buff, pos};
}

String
string_skip_last_dot(String s)
{
    if (!s.buff)
    {
        return string_empty();
    }

    u64 pos = (u64)-1;
    for (u64 i = 0; i < s.size; i++)
    {
        if (s.buff[i] == '.')
        {
            pos = i;
        }
    }

    if (pos == (u64)-1)
    {
        return string_empty();
    }

    return String{s.buff + pos + 1, s.size - pos - 1};
}

String
string_path_join(Arena *arena, String dir, String file)
{
    if (!arena)
    {
        return string_empty();
    }

    if (dir.size == 0)
    {
        return string_copy(arena, file);
    }

    if (file.size == 0)
    {
        return string_copy(arena, dir);
    }

    char last = dir.buff[dir.size - 1];
    if (last == '/' || last == '\\')
    {
        return string_cat(arena, dir, file);
    }

    u64 total = dir.size + 1 + file.size;
    char *buf = push_array(arena, char, total + 1);
    memory_copy(buf, dir.buff, dir.size);

    buf[dir.size] = '/';
    memory_copy(buf + dir.size + 1, file.buff, file.size);
    buf[total] = 0;

    return String{buf, total};
}

// Search
u64
string_index_of(String s, char character)
{
    for (u64 i = 0; i < s.size; i++)
    {
        if (s.buff[i] == character)
        {
            return i;
        }
    }
    return (u64)-1;
}

// Hashing (FNV-1a)
u64
string_hash(String s)
{
    u64 hash = 14695981039346656037ULL;
    for (u64 i = 0; i < s.size; i++)
    {
        hash ^= (u64)(unsigned char)s.buff[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

b8
string_to_f32(String s, f32 *out)
{
    if (!out || !s.buff || s.size == 0)
    {
        return false;
    }

    return sscanf(s.buff, "%f", out) == 1;
}

b8
string_to_f64(String s, f64 *out)
{
    if (!out || !s.buff || s.size == 0)
    {
        return false;
    }

    return sscanf(s.buff, "%lf", out) == 1;
}

b8
string_to_vec2(String s, vec2 *out)
{
    if (!out || !s.buff || s.size == 0)
    {
        return false;
    }

    return sscanf(s.buff, "%f %f", &out->x, &out->y) == 2;
}

b8
string_to_vec3(String s, vec3 *out)
{
    if (!out || !s.buff || s.size == 0)
    {
        return false;
    }

    return sscanf(s.buff, "%f %f %f", &out->x, &out->y, &out->z) == 3;
}

b8
string_to_vec4(String s, vec4 *out)
{
    if (!out || !s.buff || s.size == 0)
    {
        return false;
    }

    return sscanf(s.buff, "%f %f %f %f", &out->x, &out->y, &out->z, &out->w) ==
           4;
}

b8
string_to_bool(String s, b8 *out)
{
    if (!out || s.size == 0 || !s.buff)
    {
        return false;
    }

    u64 start = 0;
    while (start < s.size && isspace((unsigned char)s.buff[start]))
    {
        start += 1;
    }

    u64 end = s.size;
    while (end > start && isspace((unsigned char)s.buff[end - 1]))
    {
        end -= 1;
    }

    String trimmed = String{s.buff + start, end - start};

    if (string_match(trimmed,
                  STR_LIT("true"),
                  String_Match_Flags::CASE_INSENSITIVE) ||
        string_match(trimmed, STR_LIT("1")))
    {
        *out = true;
        return true;
    }

    if (string_match(trimmed,
                  STR_LIT("false"),
                  String_Match_Flags::CASE_INSENSITIVE) ||
        string_match(trimmed, STR_LIT("0")))
    {
        *out = false;
        return true;
    }

    return false;
}
