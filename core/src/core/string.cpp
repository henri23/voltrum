#include "core/string.hpp"
#include "memory/memory.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

b8 string_check_equal(
    const char* str1,
    const char* str2) {
    return strcmp(str1, str2) == 0;
}

s32 string_format(
    char* dest,
    const char* format, ...) {
    if (dest) {

        va_list arg_ptr; // Pointer to args

        va_start(arg_ptr, format);
        s32 written = string_format_v(dest, format, arg_ptr);
        va_end(arg_ptr);
        return written;
    }

    return -1;
}

s32 string_format_v(
    char* dest,
    const char* format,
    va_list va_list) {
    if (dest) {
        char buffer[32000];
        s32 written = vsnprintf(buffer, 32000, format, va_list);
        buffer[written] = 0;
        memory_copy(dest, buffer, written + 1);

        return written;
    }

    return -1;
}

u64 string_length(const char* string) {
	u64 length = 0;
	// Continue to iterate inside the string until we find the
	// null terminator /0 whose ASCII value is 0
	while(string[length] != 0) {
		length++;
	}
	return length;
}

void string_copy(
    char* dest,
    const char* source,
    u64 max_length) {
    if (!dest || !source || max_length == 0) {
        return;
    }

    u64 i = 0;
    // Copy characters until we hit null terminator or max_length - 1
    while (i < max_length - 1 && source[i] != 0) {
        dest[i] = source[i];
        i++;
    }
    // Always null terminate
    dest[i] = 0;
}

