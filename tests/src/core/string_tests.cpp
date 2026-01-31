#include "string_tests.hpp"
#include "expect.hpp"
#include "test_manager.hpp"

#include <core/logger.hpp>
#include <defines.hpp>
#include <math/math_types.hpp>
#include <memory/arena.hpp>
#include <utils/string.hpp>

// String construction and basic properties

INTERNAL_FUNC u8 test_STR()
{
    String s = STR("hello");

    expect_should_be((u64)5, s.size);
    expect_should_be('h', (char)s.str[0]);
    expect_should_be('o', (char)s.str[4]);

    // Empty literal
    String empty = STR("");
    expect_should_be((u64)0, empty.size);

    return true;
}

INTERNAL_FUNC u8 test_str_from_cstr()
{
    String s = str_from_cstr("world");
    expect_should_be((u64)5, s.size);
    expect_should_be('w', (char)s.str[0]);

    // Null input
    String null_s = str_from_cstr(nullptr);
    expect_should_be((u64)0, null_s.size);

    return true;
}

INTERNAL_FUNC u8 test_str_zero()
{
    String s = str_zero();
    expect_should_be((u64)0, s.size);
    expect_should_be(true, s.str == nullptr);

    return true;
}

// Const_String

INTERNAL_FUNC u8 test_const_str_from_cstr()
{
    auto cs = const_str_from_cstr<32>("hello");
    expect_should_be((u64)5, cs.size);
    expect_should_be('h', (char)cs.data[0]);
    expect_should_be('o', (char)cs.data[4]);

    // Implicit conversion to String
    String view = cs;
    expect_should_be((u64)5, view.size);
    expect_should_be('h', (char)view.str[0]);

    return true;
}

INTERNAL_FUNC u8 test_const_str_truncation()
{
    // String longer than capacity should be truncated
    auto cs = const_str_from_cstr<4>("hello world");
    expect_should_be((u64)4, cs.size);
    expect_should_be('h', (char)cs.data[0]);
    expect_should_be('l', (char)cs.data[3]);

    return true;
}

INTERNAL_FUNC u8 test_const_str_from_str()
{
    String s  = STR("test string");
    auto   cs = const_str_from_str<64>(s);
    expect_should_be(s.size, cs.size);

    String view = cs;
    expect_should_be(true, str_match(s, view));

    return true;
}

// Matching

INTERNAL_FUNC u8 test_str_match_exact()
{
    expect_should_be(true, str_match(STR("abc"), STR("abc")));
    expect_should_be(false, str_match(STR("abc"), STR("def")));
    expect_should_be(false, str_match(STR("abc"), STR("ab")));
    expect_should_be(false, str_match(STR("ab"), STR("abc")));

    // Empty strings match
    expect_should_be(true, str_match(str_zero(), str_zero()));

    return true;
}

INTERNAL_FUNC u8 test_str_match_case_insensitive()
{
    expect_should_be(true,
                     str_match(STR("Hello"),
                               STR("hello"),
                               String_Match_Flags::CASE_INSENSITIVE));

    expect_should_be(true,
                     str_match(STR("ABC"),
                               STR("abc"),
                               String_Match_Flags::CASE_INSENSITIVE));

    expect_should_be(false,
                     str_match(STR("Hello"),
                               STR("World"),
                               String_Match_Flags::CASE_INSENSITIVE));

    return true;
}

INTERNAL_FUNC u8 test_str_match_slash_insensitive()
{
    expect_should_be(true,
                     str_match(STR("path/to/file"),
                               STR("path\\to\\file"),
                               String_Match_Flags::SLASH_INSENSITIVE));

    return true;
}

INTERNAL_FUNC u8 test_str_find_needle()
{
    String haystack = STR("hello world");

    expect_should_be((u64)6,
                     str_find_needle(haystack, 0, STR("world")));
    expect_should_be((u64)0,
                     str_find_needle(haystack, 0, STR("hello")));
    expect_should_be((u64)-1,
                     str_find_needle(haystack, 0, STR("xyz")));

    // Start offset
    expect_should_be((u64)-1,
                     str_find_needle(haystack, 7, STR("world")));

    // Empty needle
    expect_should_be((u64)-1,
                     str_find_needle(haystack, 0, str_zero()));

    return true;
}

// Slicing

INTERNAL_FUNC u8 test_str_prefix()
{
    String s = STR("hello world");

    String p = str_prefix(s, 5);
    expect_should_be((u64)5, p.size);
    expect_should_be(true, str_match(p, STR("hello")));

    // Prefix larger than string returns whole string
    String full = str_prefix(s, 100);
    expect_should_be(s.size, full.size);

    return true;
}

INTERNAL_FUNC u8 test_str_skip()
{
    String s = STR("hello world");

    String skipped = str_skip(s, 6);
    expect_should_be((u64)5, skipped.size);
    expect_should_be(true, str_match(skipped, STR("world")));

    // Skip more than size returns empty
    String empty = str_skip(s, 100);
    expect_should_be((u64)0, empty.size);

    return true;
}

INTERNAL_FUNC u8 test_str_substr()
{
    String s = STR("hello world");

    String sub = str_substr(s, 6, 5);
    expect_should_be((u64)5, sub.size);
    expect_should_be(true, str_match(sub, STR("world")));

    // Out of bounds start
    String empty = str_substr(s, 100, 5);
    expect_should_be((u64)0, empty.size);

    return true;
}

INTERNAL_FUNC u8 test_str_trim_whitespace()
{
    String s = STR("  hello  ");

    String trimmed = str_trim_whitespace(s);
    expect_should_be((u64)5, trimmed.size);
    expect_should_be(true, str_match(trimmed, STR("hello")));

    // Already trimmed
    String clean = STR("hello");
    String same  = str_trim_whitespace(clean);
    expect_should_be(clean.size, same.size);

    // All whitespace
    String ws      = STR("   ");
    String ws_trim = str_trim_whitespace(ws);
    expect_should_be((u64)0, ws_trim.size);

    return true;
}

// Arena-allocated operations

INTERNAL_FUNC u8 test_str_copy()
{
    Arena *arena = arena_create();

    String original = STR("hello");
    String copy     = str_copy(arena, original);

    expect_should_be(original.size, copy.size);
    expect_should_be(true, str_match(original, copy));

    // Must be a different buffer
    expect_should_be(true, original.str != copy.str);

    // Copy of zero returns zero
    String z = str_copy(arena, str_zero());
    expect_should_be((u64)0, z.size);

    arena_release(arena);
    return true;
}

INTERNAL_FUNC u8 test_str_cat()
{
    Arena *arena = arena_create();

    String a      = STR("hello ");
    String b      = STR("world");
    String result = str_cat(arena, a, b);

    expect_should_be((u64)11, result.size);
    expect_should_be(true, str_match(result, STR("hello world")));

    arena_release(arena);
    return true;
}

INTERNAL_FUNC u8 test_str_fmt()
{
    Arena *arena = arena_create();

    String result = str_fmt(arena, "number: %d, float: %.1f", 42, 3.5f);
    expect_should_be(true,
                     str_match(result,
                               STR("number: 42, float: 3.5")));

    arena_release(arena);
    return true;
}

// Path helpers

INTERNAL_FUNC u8 test_str_path_helpers()
{
    String path = STR("/home/user/file.txt");

    // chop_last_slash -> directory
    String dir = str_chop_last_slash(path);
    expect_should_be(true, str_match(dir, STR("/home/user")));

    // skip_last_slash -> filename
    String file = str_skip_last_slash(path);
    expect_should_be(true, str_match(file, STR("file.txt")));

    // chop_last_dot -> without extension
    String no_ext = str_chop_last_dot(path);
    expect_should_be(true,
                     str_match(no_ext, STR("/home/user/file")));

    // skip_last_dot -> extension
    String ext = str_skip_last_dot(path);
    expect_should_be(true, str_match(ext, STR("txt")));

    // No slash
    String name = STR("file.txt");
    expect_should_be(true,
                     str_match(str_chop_last_slash(name), name));
    expect_should_be(true,
                     str_match(str_skip_last_slash(name), name));

    // No dot
    String no_dot = STR("Makefile");
    expect_should_be(true,
                     str_match(str_chop_last_dot(no_dot), no_dot));
    expect_should_be((u64)0, str_skip_last_dot(no_dot).size);

    return true;
}

INTERNAL_FUNC u8 test_str_path_join()
{
    Arena *arena = arena_create();

    String result = str_path_join(arena,
                                  STR("/home/user"),
                                  STR("file.txt"));
    expect_should_be(true,
                     str_match(result,
                               STR("/home/user/file.txt")));

    // Dir already ends with slash
    String result2 = str_path_join(arena,
                                   STR("/home/user/"),
                                   STR("file.txt"));
    expect_should_be(true,
                     str_match(result2,
                               STR("/home/user/file.txt")));

    arena_release(arena);
    return true;
}

// Search / Indexing / Hashing

INTERNAL_FUNC u8 test_str_index_of()
{
    String s = STR("hello world");

    expect_should_be((u64)4, str_index_of(s, 'o'));
    expect_should_be((u64)5, str_index_of(s, ' '));
    expect_should_be((u64)-1, str_index_of(s, 'z'));

    return true;
}

INTERNAL_FUNC u8 test_str_hash()
{
    // Same string should produce same hash
    u64 h1 = str_hash(STR("hello"));
    u64 h2 = str_hash(STR("hello"));
    expect_should_be(h1, h2);

    // Different strings should (very likely) produce different hashes
    u64 h3 = str_hash(STR("world"));
    expect_should_be(true, h1 != h3);

    // Empty string should produce a consistent hash
    u64 h4 = str_hash(str_zero());
    u64 h5 = str_hash(str_zero());
    expect_should_be(h4, h5);

    return true;
}

// Parsing

INTERNAL_FUNC u8 test_str_to_f32_valid()
{
    f32 result;

    expect_should_be(true, str_to_f32(STR("42.5"), &result));
    expect_float_to_be(42.5f, result);

    expect_should_be(true, str_to_f32(STR("-123.456"), &result));
    expect_float_to_be(-123.456f, result);

    expect_should_be(true, str_to_f32(STR("0.0"), &result));
    expect_float_to_be(0.0f, result);

    return true;
}

INTERNAL_FUNC u8 test_str_to_f32_invalid()
{
    f32 result;

    expect_should_be(false, str_to_f32(str_zero(), &result));
    expect_should_be(false, str_to_f32(STR("abc"), &result));
    expect_should_be(false, str_to_f32(STR("42.5"), nullptr));

    return true;
}

INTERNAL_FUNC u8 test_str_to_vec4_valid()
{
    vec4 result;

    expect_should_be(true,
                     str_to_vec4(STR("1.0 2.0 3.0 4.0"), &result));
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
                     str_to_vec3(STR("1.0 2.0 3.0"), &result));
    expect_float_to_be(1.0f, result.x);
    expect_float_to_be(2.0f, result.y);
    expect_float_to_be(3.0f, result.z);

    return true;
}

INTERNAL_FUNC u8 test_str_to_vec2_valid()
{
    vec2 result;

    expect_should_be(true,
                     str_to_vec2(STR("1.0 2.0"), &result));
    expect_float_to_be(1.0f, result.x);
    expect_float_to_be(2.0f, result.y);

    return true;
}

INTERNAL_FUNC u8 test_str_to_bool_valid()
{
    b8 result;

    expect_should_be(true, str_to_bool(STR("true"), &result));
    expect_should_be(true, result);

    expect_should_be(true, str_to_bool(STR("false"), &result));
    expect_should_be(false, result);

    expect_should_be(true, str_to_bool(STR("1"), &result));
    expect_should_be(true, result);

    expect_should_be(true, str_to_bool(STR("0"), &result));
    expect_should_be(false, result);

    // Case insensitive
    expect_should_be(true, str_to_bool(STR("TRUE"), &result));
    expect_should_be(true, result);

    expect_should_be(true, str_to_bool(STR("False"), &result));
    expect_should_be(false, result);

    return true;
}

INTERNAL_FUNC u8 test_str_to_bool_invalid()
{
    b8 result;

    expect_should_be(false, str_to_bool(STR("yes"), &result));
    expect_should_be(false, str_to_bool(STR("2"), &result));
    expect_should_be(false, str_to_bool(str_zero(), &result));

    return true;
}

// Registration

void str_register_tests()
{
    // Construction
    test_manager_register_test(
        test_STR,
        "Str: STR construction");
    test_manager_register_test(
        test_str_from_cstr,
        "Str: str_from_cstr construction");
    test_manager_register_test(
        test_str_zero,
        "Str: str_zero construction");

    // Const_String
    test_manager_register_test(
        test_const_str_from_cstr,
        "Str: const_str_from_cstr");
    test_manager_register_test(
        test_const_str_truncation,
        "Str: const_str truncation");
    test_manager_register_test(
        test_const_str_from_str,
        "Str: const_str_from_str");

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
