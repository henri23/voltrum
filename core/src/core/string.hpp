#pragma once

#include "defines.hpp"
#include <stdarg.h>

VOLTRUM_API b8 string_check_equal(
    const char* str1,
    const char* str2);

VOLTRUM_API b8 string_check_equal_insensitive(
    const char* str1,
    const char* str2);

VOLTRUM_API s32 string_format(
	char* dest, 
	const char* format, ...);

VOLTRUM_API s32 string_format_v(
    char* dest,
    const char* format,
    va_list va_list);

VOLTRUM_API u64 string_length(
    const char* string);

VOLTRUM_API void string_copy(
    char* dest,
    const char* source,
    u64 max_length);
