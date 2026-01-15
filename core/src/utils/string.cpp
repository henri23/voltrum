#include "utils/string.hpp"
#include "memory/memory.hpp"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if !defined(PLATFORM_WINDOWS)
#    include <strings.h>
#endif

b8 string_check_equal(const char *str1, const char *str2) {
    return strcmp(str1, str2) == 0;
}

b8 string_check_equal_insensitive(const char *str1, const char *str2) {
#if defined(PLATFORM_WINDOWS)
    return _stricmp(str1, str2) == 0;
#else
    return strcasecmp(str1, str2) == 0;
#endif
}

s32 string_format(char *dest, const char *format, ...) {
    if (dest) {

        va_list arg_ptr; // Pointer to args

        va_start(arg_ptr, format);
        s32 written = string_format_v(dest, format, arg_ptr);
        va_end(arg_ptr);
        return written;
    }

    return -1;
}

s32 string_format_v(char *dest, const char *format, va_list va_list) {
    if (dest) {
        char buffer[32000];
        s32 written = vsnprintf(buffer, 32000, format, va_list);
        buffer[written] = 0;
        memory_copy(dest, buffer, written + 1);

        return written;
    }

    return -1;
}

char *string_empty(char *str) {
    if (str) {
        str[0] = '\0';
    }

    return str;
}

u64 string_length(const char *string) {
    u64 length = 0;
    // Continue to iterate inside the string until we find the
    // null terminator /0 whose ASCII value is 0
    while (string[length] != 0) {
        length++;
    }
    return length;
}

char *string_copy(char *dest, const char *source) {
    return strcpy(dest, source);
}

char *string_ncopy(char *dest, const char *source, u64 max_length) {
    return strncpy(dest, source, max_length);
}

char *string_duplicate(const char *source) {
    if (!source) {
        return nullptr;
    }

    u64 length = string_length(source);
    char *duplicate = static_cast<char *>(
        memory_allocate((length + 1) * sizeof(char), Memory_Tag::STRING));

    if (duplicate) {
        string_copy(duplicate, source);
    }

    return duplicate;
}

// This method does not allocate memory, but instead it modifies the existing
// string. If the string provided was dynamically allocated, the original
// pointer must be kept, and used for memory deallocation.
char *string_trim(char *str) {
    char *end;

    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0)
        return str;

    end = str + string_length(str) - 1;

    while (end > str && isspace((unsigned char)*end))
        end--;

    end[1] = '\0';

    return str;
}

// This method dynamically allocates a new substring with trimmed whitespaces
// Must be deallocated by the called. It returns the length of the new allocated
// string
u64 string_trim_copy(const char *src_str, char *out_str) {
    u64 length = 0;

    while (isspace((unsigned)*src_str))
        src_str++;

    if (*src_str == 0) {
        out_str = nullptr;
    }

    const char *end = src_str + string_length(src_str) - 1;
    while (end > src_str && isspace((unsigned char)*end))
        end--;

    length = end - src_str;

    // WARN: Potential memory leak if not properly deallocate
    out_str = static_cast<char *>(
        memory_allocate(length * sizeof(char), Memory_Tag::STRING));

    string_ncopy(out_str, src_str, length);
    out_str[length] = '\0';

    return length;
}

void string_substr(char *dest, const char *source, s32 start, s32 length) {
    if (length == 0)
        return;

    u64 src_length = string_length(source);

    if (start >= src_length) {
        dest[0] = '\0';
        return;
    }

    if (length > 0) {
        for (u64 i = start, j = 0; j < length && source[i]; ++i, ++j) {
            dest[j] = source[i];
        }
        dest[start + length] = '\0';
    } else {
        // If a negative length is provided, we proceede until the end of the
        // source string
        u64 j = 0;
        for (u64 i = start; source[i]; ++i, ++j) {
            dest[j] = source[i];
        }
        dest[start + length] = '\0';
    }
}

s32 string_index_of(char *str, char character) {
    if (!str) {
        return -1;
    }

    u32 length = string_length(str);
    if (length > 0) {
        for (u32 i = 0; i < length; ++i) {
            if (str[i] == character) {
                return i;
            }
        }
    }

    return -1;
}

b8 string_to_vec4(char *str, vec4 *out_vec4) {
    if (!str || !out_vec4) {
        return false;
    }

    s32 result = sscanf(str,
        "%f %f %f %f",
        &out_vec4->x,
        &out_vec4->y,
        &out_vec4->z,
        &out_vec4->w);

    return result == 4;
}

b8 string_to_vec3(char *str, vec3 *out_vec3) {
    if (!str || !out_vec3) {
        return false;
    }

    s32 result =
        sscanf(str, "%f %f %f", &out_vec3->x, &out_vec3->y, &out_vec3->z);

    return result == 3;
}

b8 string_to_vec2(char *str, vec2 *out_vec2) {
    if (!str || !out_vec2) {
        return false;
    }

    s32 result = sscanf(str, "%f %f", &out_vec2->x, &out_vec2->y);

    return result == 2;
}

b8 string_to_f32(char *str, f32 *out_float) {
    if (!str || !out_float) {
        return false;
    }

    s32 result = sscanf(str, "%f", out_float);

    return result == 1;
}

b8 string_to_f64(char *str, f64 *out_double) {
    if (!str || !out_double) {
        return false;
    }

    s32 result = sscanf(str, "%lf", out_double);

    return result == 1;
}

b8 string_to_bool(char *str, b8 *out_bool) {
    if (!str || !out_bool) {
        return false;
    }

    if (string_check_equal_insensitive(str, "true") ||
        string_check_equal(str, "1")) {
        *out_bool = true;
        return true;
    } else if (string_check_equal_insensitive(str, "false") ||
               string_check_equal(str, "0")) {
        *out_bool = false;
        return true;
    }

    return false;
}
