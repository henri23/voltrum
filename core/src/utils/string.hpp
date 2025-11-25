#pragma once

#include "defines.hpp"
#include "math/math_types.hpp"

#include <stdarg.h>

VOLTRUM_API b8 string_check_equal(const char* str1, const char* str2);

VOLTRUM_API b8 string_check_equal_insensitive(const char* str1,
    const char* str2);

VOLTRUM_API s32 string_format(char* dest, const char* format, ...);

VOLTRUM_API s32 string_format_v(char* dest,
    const char* format,
    va_list va_list);

VOLTRUM_API char* string_empty(char* str);

VOLTRUM_API u64 string_length(const char* string);

VOLTRUM_API char* string_copy(char* dest, const char* source);
VOLTRUM_API char* string_ncopy(char* dest, const char* source, u64 max_length);

// Trim whitespaces from both ends of the string
VOLTRUM_API char* string_trim(char* str);

VOLTRUM_API char* string_trim_copy(char* str);

// Get a substring from a string
// If the length is -1 go to the end of the source string
VOLTRUM_API void
string_substr(char* dest, const char* source, s32 start, s32 length = -1);

// Returns the index of the first occurence of the character.
// Returns -1 if character is not found
VOLTRUM_API s32 string_index_of(char* str, char character);

// Parse vec4 from space delimited string (i.e "1.0 1.0 4.0 0.0")
VOLTRUM_API b8 string_to_vec4(char* str, vec4* out_vec4);

VOLTRUM_API b8 string_to_vec3(char* str, vec3* out_vec3);

VOLTRUM_API b8 string_to_vec2(char* str, vec2* out_vec2);

VOLTRUM_API b8 string_to_f32(char* str, f32* out_float);

VOLTRUM_API b8 string_to_f64(char* str, f64* out_double);

// The input string can either be "true"/"false" or "0"/"1"
VOLTRUM_API b8 string_to_bool(char* str, b8* out_bool);
