#include "string_tests.hpp"
#include "expect.hpp"
#include "test_manager.hpp"

#include <core/logger.hpp>
#include <defines.hpp>
#include <math/math_types.hpp>
#include <memory/arena.hpp>
#include <utils/string.hpp>

// String construction and basic properties

INTERNAL_FUNC u8 test_STR_LIT()
{
    String s = STR_LIT("hello");

    expect_should_be((u64)5, s.size);
    expect_should_be('h', (char)s.buff[0]);
    expect_should_be('o', (char)s.buff[4]);

    // Empty literal
    String empty = STR_LIT("");
    expect_should_be((u64)0, empty.size);

    return true;
}

INTERNAL_FUNC u8 test_str_from_cstr()
{
    String s = STR("world");
    expect_should_be((u64)5, s.size);
    expect_should_be('w', (char)s.buff[0]);

    // Null input
    String null_s = STR(nullptr);
    expect_should_be((u64)0, null_s.size);

    return true;
}

INTERNAL_FUNC u8 test_str_zero()
{
    String s = string_empty();
    expect_should_be((u64)0, s.size);
    expect_should_be(true, s.buff == nullptr);

    return true;
}

// Fixed-size buffers

INTERNAL_FUNC u8 test_const_str_from_cstr()
{
    char buffer[32] = {};
    string_set(buffer, "hello");
    String view = string_capped(buffer, buffer + sizeof(buffer));
    expect_should_be((u64)5, view.size);
    expect_should_be('h', (char)view.buff[0]);
    expect_should_be('o', (char)view.buff[4]);

    return true;
}

INTERNAL_FUNC u8 test_const_str_truncation()
{
    char buffer[4] = {};
    string_set(buffer, "hello world");
    String view = string_capped(buffer, buffer + sizeof(buffer));
    expect_should_be((u64)3, view.size);
    expect_should_be('h', (char)view.buff[0]);
    expect_should_be('l', (char)view.buff[2]);

    return true;
}

INTERNAL_FUNC u8 test_const_str_from_str()
{
    String s = STR_LIT("test string");
    char   buffer[64] = {};
    string_set(buffer, s);
    String view = string_capped(buffer, buffer + sizeof(buffer));
    expect_should_be(s.size, view.size);
    expect_should_be(true, string_match(s, view));

    return true;
}

// Matching

INTERNAL_FUNC u8 test_str_match_exact()
{
    expect_should_be(true, string_match(STR_LIT("abc"), STR_LIT("abc")));
    expect_should_be(false, string_match(STR_LIT("abc"), STR_LIT("def")));
    expect_should_be(false, string_match(STR_LIT("abc"), STR_LIT("ab")));
    expect_should_be(false, string_match(STR_LIT("ab"), STR_LIT("abc")));

    // Empty strings match
    expect_should_be(true, string_match(string_empty(), string_empty()));

    return true;
}

INTERNAL_FUNC u8 test_str_match_case_insensitive()
{
    expect_should_be(true,
                     string_match(STR_LIT("Hello"),
                               STR_LIT("hello"),
                               String_Match_Flags::CASE_INSENSITIVE));

    expect_should_be(true,
                     string_match(STR_LIT("ABC"),
                               STR_LIT("abc"),
                               String_Match_Flags::CASE_INSENSITIVE));

    expect_should_be(false,
                     string_match(STR_LIT("Hello"),
                               STR_LIT("World"),
                               String_Match_Flags::CASE_INSENSITIVE));

    return true;
}

INTERNAL_FUNC u8 test_str_match_slash_insensitive()
{
    expect_should_be(true,
                     string_match(STR_LIT("path/to/file"),
                               STR_LIT("path\\to\\file"),
                               String_Match_Flags::SLASH_INSENSITIVE));

    return true;
}

INTERNAL_FUNC u8 test_str_find_needle()
{
    String haystack = STR_LIT("hello world");

    expect_should_be((u64)6,
                     string_find(haystack, 0, STR_LIT("world")));
    expect_should_be((u64)0,
                     string_find(haystack, 0, STR_LIT("hello")));
    expect_should_be((u64)-1,
                     string_find(haystack, 0, STR_LIT("xyz")));

    // Start offset
    expect_should_be((u64)-1,
                     string_find(haystack, 7, STR_LIT("world")));

    // Empty needle
    expect_should_be((u64)-1,
                     string_find(haystack, 0, string_empty()));

    return true;
}

// Slicing

INTERNAL_FUNC u8 test_str_prefix()
{
    char   buffer[] = "hello world";
    String s        = string_capped(buffer, buffer + sizeof(buffer));

    String p = string_prefix(s, 5);
    expect_should_be((u64)5, p.size);
    expect_should_be(true, string_match(p, STR_LIT("hello")));

    // Prefix larger than string returns whole string
    char   buffer2[] = "hello world";
    String s2        = string_capped(buffer2, buffer2 + sizeof(buffer2));
    String full = string_prefix(s2, 100);
    expect_should_be(s2.size, full.size);

    return true;
}

INTERNAL_FUNC u8 test_str_skip()
{
    String s = STR_LIT("hello world");

    String skipped = string_skip(s, 6);
    expect_should_be((u64)5, skipped.size);
    expect_should_be(true, string_match(skipped, STR_LIT("world")));

    // Skip more than size returns empty
    String empty = string_skip(s, 100);
    expect_should_be((u64)0, empty.size);

    return true;
}

INTERNAL_FUNC u8 test_str_substr()
{
    char   buffer[] = "hello world";
    String s        = string_capped(buffer, buffer + sizeof(buffer));

    String sub = string_substr(s, 6, 5);
    expect_should_be((u64)5, sub.size);
    expect_should_be(true, string_match(sub, STR_LIT("world")));

    // Out of bounds start
    String empty = string_substr(s, 100, 5);
    expect_should_be((u64)0, empty.size);

    return true;
}

INTERNAL_FUNC u8 test_str_trim_whitespace()
{
    char   s_buffer[] = "  hello  ";
    String s          = string_capped(s_buffer, s_buffer + sizeof(s_buffer));

    String trimmed = string_trim_whitespace(s);
    expect_should_be((u64)5, trimmed.size);
    expect_should_be(true, string_match(trimmed, STR_LIT("hello")));

    // Already trimmed
    char   clean_buffer[] = "hello";
    String clean          = string_capped(clean_buffer, clean_buffer + sizeof(clean_buffer));
    String same           = string_trim_whitespace(clean);
    expect_should_be(clean.size, same.size);

    // All whitespace
    char   ws_buffer[] = "   ";
    String ws          = string_capped(ws_buffer, ws_buffer + sizeof(ws_buffer));
    String ws_trim     = string_trim_whitespace(ws);
    expect_should_be((u64)0, ws_trim.size);

    return true;
}

// Arena-allocated operations

INTERNAL_FUNC u8 test_str_copy()
{
    Arena *arena = arena_create();

    String original = STR_LIT("hello");
    String copy     = string_copy(arena, original);

    expect_should_be(original.size, copy.size);
    expect_should_be(true, string_match(original, copy));

    // Must be a different buffer
    expect_should_be(true, original.buff != copy.buff);

    // Copy of zero returns zero
    String z = string_copy(arena, string_empty());
    expect_should_be((u64)0, z.size);

    arena_release(arena);
    return true;
}

INTERNAL_FUNC u8 test_str_cat()
{
    Arena *arena = arena_create();

    String a      = STR_LIT("hello ");
    String b      = STR_LIT("world");
    String result = string_cat(arena, a, b);

    expect_should_be((u64)11, result.size);
    expect_should_be(true, string_match(result, STR_LIT("hello world")));

    arena_release(arena);
    return true;
}

INTERNAL_FUNC u8 test_str_fmt()
{
    Arena *arena = arena_create();

    String result = string_fmt(arena, "number: %d, float: %.1f", 42, 3.5f);
    expect_should_be(true,
                     string_match(result,
                               STR_LIT("number: 42, float: 3.5")));

    arena_release(arena);
    return true;
}

// Path helpers

INTERNAL_FUNC u8 test_str_path_helpers()
{
    char   dir_buffer[] = "/home/user/file.txt";
    String path_for_dir = string_capped(dir_buffer, dir_buffer + sizeof(dir_buffer));

    // chop_last_slash -> directory
    String dir = string_chop_last_slash(path_for_dir);
    expect_should_be(true, string_match(dir, STR_LIT("/home/user")));

    char   file_buffer[] = "/home/user/file.txt";
    String path_for_file = string_capped(file_buffer, file_buffer + sizeof(file_buffer));
    // skip_last_slash -> filename
    String file = string_skip_last_slash(path_for_file);
    expect_should_be(true, string_match(file, STR_LIT("file.txt")));

    char   no_ext_buffer[] = "/home/user/file.txt";
    String path_for_no_ext = string_capped(no_ext_buffer, no_ext_buffer + sizeof(no_ext_buffer));
    // chop_last_dot -> without extension
    String no_ext = string_chop_last_dot(path_for_no_ext);
    expect_should_be(true,
                     string_match(no_ext, STR_LIT("/home/user/file")));

    char   ext_buffer[] = "/home/user/file.txt";
    String path_for_ext = string_capped(ext_buffer, ext_buffer + sizeof(ext_buffer));
    // skip_last_dot -> extension
    String ext = string_skip_last_dot(path_for_ext);
    expect_should_be(true, string_match(ext, STR_LIT("txt")));

    // No slash
    String name = STR_LIT("file.txt");
    expect_should_be(true,
                     string_match(string_chop_last_slash(name), name));
    expect_should_be(true,
                     string_match(string_skip_last_slash(name), name));

    // No dot
    String no_dot = STR_LIT("Makefile");
    expect_should_be(true,
                     string_match(string_chop_last_dot(no_dot), no_dot));
    expect_should_be((u64)0, string_skip_last_dot(no_dot).size);

    return true;
}

INTERNAL_FUNC u8 test_str_path_join()
{
    Arena *arena = arena_create();

    String result = string_path_join(arena,
                                  STR_LIT("/home/user"),
                                  STR_LIT("file.txt"));
    expect_should_be(true,
                     string_match(result,
                               STR_LIT("/home/user/file.txt")));

    // Dir already ends with slash
    String result2 = string_path_join(arena,
                                   STR_LIT("/home/user/"),
                                   STR_LIT("file.txt"));
    expect_should_be(true,
                     string_match(result2,
                               STR_LIT("/home/user/file.txt")));

    arena_release(arena);
    return true;
}

// Search / Indexing / Hashing

INTERNAL_FUNC u8 test_str_index_of()
{
    String s = STR_LIT("hello world");

    expect_should_be((u64)4, string_index_of(s, 'o'));
    expect_should_be((u64)5, string_index_of(s, ' '));
    expect_should_be((u64)-1, string_index_of(s, 'z'));

    return true;
}

INTERNAL_FUNC u8 test_str_hash()
{
    // Same string should produce same hash
    u64 h1 = string_hash(STR_LIT("hello"));
    u64 h2 = string_hash(STR_LIT("hello"));
    expect_should_be(h1, h2);

    // Different strings should (very likely) produce different hashes
    u64 h3 = string_hash(STR_LIT("world"));
    expect_should_be(true, h1 != h3);

    // Empty string should produce a consistent hash
    u64 h4 = string_hash(string_empty());
    u64 h5 = string_hash(string_empty());
    expect_should_be(h4, h5);

    return true;
}

// Parsing

INTERNAL_FUNC u8 test_str_to_f32_valid()
{
    f32 result;

    expect_should_be(true, string_to_f32(STR_LIT("42.5"), &result));
    expect_float_to_be(42.5f, result);

    expect_should_be(true, string_to_f32(STR_LIT("-123.456"), &result));
    expect_float_to_be(-123.456f, result);

    expect_should_be(true, string_to_f32(STR_LIT("0.0"), &result));
    expect_float_to_be(0.0f, result);

    return true;
}

INTERNAL_FUNC u8 test_str_to_f32_invalid()
{
    f32 result;

    expect_should_be(false, string_to_f32(string_empty(), &result));
    expect_should_be(false, string_to_f32(STR_LIT("abc"), &result));
    expect_should_be(false, string_to_f32(STR_LIT("42.5"), nullptr));

    return true;
}

INTERNAL_FUNC u8 test_str_to_vec4_valid()
{
    vec4 result;

    expect_should_be(true,
                     string_to_vec4(STR_LIT("1.0 2.0 3.0 4.0"), &result));
    expect_float_to_be(1.0f, result.x);
    expect_float_to_be(2.0f, result.y);
    expect_float_to_be(3.0f, result.z);
    expect_float_to_be(4.0f, result.w);

    return true;
}

INTERNAL_FUNC u8 test_str_to_vec3_valid()
{
    vec3 result;

    expect_should_be(true,
                     string_to_vec3(STR_LIT("1.0 2.0 3.0"), &result));
    expect_float_to_be(1.0f, result.x);
    expect_float_to_be(2.0f, result.y);
    expect_float_to_be(3.0f, result.z);

    return true;
}

INTERNAL_FUNC u8 test_str_to_vec2_valid()
{
    vec2 result;

    expect_should_be(true,
                     string_to_vec2(STR_LIT("1.0 2.0"), &result));
    expect_float_to_be(1.0f, result.x);
    expect_float_to_be(2.0f, result.y);

    return true;
}

INTERNAL_FUNC u8 test_str_to_bool_valid()
{
    b8 result;

    expect_should_be(true, string_to_bool(STR_LIT("true"), &result));
    expect_should_be(true, result);

    expect_should_be(true, string_to_bool(STR_LIT("false"), &result));
    expect_should_be(false, result);

    expect_should_be(true, string_to_bool(STR_LIT("1"), &result));
    expect_should_be(true, result);

    expect_should_be(true, string_to_bool(STR_LIT("0"), &result));
    expect_should_be(false, result);

    // Case insensitive
    expect_should_be(true, string_to_bool(STR_LIT("TRUE"), &result));
    expect_should_be(true, result);

    expect_should_be(true, string_to_bool(STR_LIT("False"), &result));
    expect_should_be(false, result);

    return true;
}

INTERNAL_FUNC u8 test_str_to_bool_invalid()
{
    b8 result;

    expect_should_be(false, string_to_bool(STR_LIT("yes"), &result));
    expect_should_be(false, string_to_bool(STR_LIT("2"), &result));
    expect_should_be(false, string_to_bool(string_empty(), &result));

    return true;
}

// Registration

void string_register_tests()
{
    // Construction
    test_manager_register_test(
        test_STR_LIT,
        "Str: STR_LIT construction");
    test_manager_register_test(
        test_str_from_cstr,
        "Str: STR construction");
    test_manager_register_test(
        test_str_zero,
        "Str: string_empty construction");

    // Fixed-size buffers
    test_manager_register_test(
        test_const_str_from_cstr,
        "Str: fixed buffer from cstr");
    test_manager_register_test(
        test_const_str_truncation,
        "Str: fixed buffer truncation");
    test_manager_register_test(
        test_const_str_from_str,
        "Str: fixed buffer from str");

    // Matching
    test_manager_register_test(
        test_str_match_exact,
        "Str: exact match");
    test_manager_register_test(
        test_str_match_case_insensitive,
        "Str: case insensitive match");
    test_manager_register_test(
        test_str_match_slash_insensitive,
        "Str: slash insensitive match");
    test_manager_register_test(
        test_str_find_needle,
        "Str: find needle");

    // Slicing
    test_manager_register_test(
        test_str_prefix,
        "Str: prefix");
    test_manager_register_test(
        test_str_skip,
        "Str: skip");
    test_manager_register_test(
        test_str_substr,
        "Str: substr");
    test_manager_register_test(
        test_str_trim_whitespace,
        "Str: trim whitespace");

    // Arena-allocated
    test_manager_register_test(
        test_str_copy,
        "Str: arena copy");
    test_manager_register_test(
        test_str_cat,
        "Str: arena cat");
    test_manager_register_test(
        test_str_fmt,
        "Str: arena fmt");

    // Path helpers
    test_manager_register_test(
        test_str_path_helpers,
        "Str: path helpers");
    test_manager_register_test(
        test_str_path_join,
        "Str: path join");

    // Search / Hash
    test_manager_register_test(
        test_str_index_of,
        "Str: index_of");
    test_manager_register_test(
        test_str_hash,
        "Str: hash consistency");

    // Parsing
    test_manager_register_test(
        test_str_to_f32_valid,
        "Str: parse f32 valid");
    test_manager_register_test(
        test_str_to_f32_invalid,
        "Str: parse f32 invalid");
    test_manager_register_test(
        test_str_to_vec4_valid,
        "Str: parse vec4 valid");
    test_manager_register_test(
        test_str_to_vec3_valid,
        "Str: parse vec3 valid");
    test_manager_register_test(
        test_str_to_vec2_valid,
        "Str: parse vec2 valid");
    test_manager_register_test(
        test_str_to_bool_valid,
        "Str: parse bool valid");
    test_manager_register_test(
        test_str_to_bool_invalid,
        "Str: parse bool invalid");
}
