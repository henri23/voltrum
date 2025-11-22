#include "string_tests.hpp"
#include "expect.hpp"
#include "test_manager.hpp"

#include <core/logger.hpp>
#include <core/string.hpp>
#include <defines.hpp>
#include <math/math_types.hpp>
#include <memory/memory.hpp>

// Basic Parser Tests

INTERNAL_FUNC u8 test_string_to_f32_valid() {
    f32 result;

    expect_should_be(true, string_to_f32((char*)"42.5", &result));
    expect_float_to_be(42.5f, result);

    expect_should_be(true, string_to_f32((char*)"-123.456", &result));
    expect_float_to_be(-123.456f, result);

    expect_should_be(true, string_to_f32((char*)"0.0", &result));
    expect_float_to_be(0.0f, result);

    expect_should_be(true, string_to_f32((char*)"3.14159", &result));
    expect_float_to_be(3.14159f, result);

    return true;
}

INTERNAL_FUNC u8 test_string_to_f32_invalid() {
    f32 result;

    CORE_DEBUG("The next 3 test cases intentionally test invalid input:");

    // Invalid: non-numeric string
    expect_should_be(false, string_to_f32((char*)"abc", &result));

    // Invalid: empty string
    expect_should_be(false, string_to_f32((char*)"", &result));

    // Invalid: null pointer
    expect_should_be(false, string_to_f32(nullptr, &result));

    return true;
}

INTERNAL_FUNC u8 test_string_to_f64_valid() {
    f64 result;

    expect_should_be(true, string_to_f64((char*)"42.5", &result));
    expect_float_to_be(42.5, result);

    expect_should_be(true, string_to_f64((char*)"-123.456789", &result));
    expect_float_to_be(-123.456789, result);

    expect_should_be(true, string_to_f64((char*)"0.0", &result));
    expect_float_to_be(0.0, result);

    return true;
}

INTERNAL_FUNC u8 test_string_to_bool_valid() {
    b8 result;

    // Test "true"
    expect_should_be(true, string_to_bool((char*)"true", &result));
    expect_should_be(true, result);

    // Test "false"
    expect_should_be(true, string_to_bool((char*)"false", &result));
    expect_should_be(false, result);

    // Test "1"
    expect_should_be(true, string_to_bool((char*)"1", &result));
    expect_should_be(true, result);

    // Test "0"
    expect_should_be(true, string_to_bool((char*)"0", &result));
    expect_should_be(false, result);

    // Test case insensitive
    expect_should_be(true, string_to_bool((char*)"TRUE", &result));
    expect_should_be(true, result);

    expect_should_be(true, string_to_bool((char*)"False", &result));
    expect_should_be(false, result);

    return true;
}

INTERNAL_FUNC u8 test_string_to_bool_invalid() {
    b8 result;

    CORE_DEBUG("The next 3 test cases intentionally test invalid input:");

    // Invalid: random string
    expect_should_be(false, string_to_bool((char*)"yes", &result));

    // Invalid: number other than 0 or 1
    expect_should_be(false, string_to_bool((char*)"2", &result));

    // Invalid: null pointer
    expect_should_be(false, string_to_bool(nullptr, &result));

    return true;
}

INTERNAL_FUNC u8 test_string_to_vec2_valid() {
    vec2 result;

    expect_should_be(true, string_to_vec2((char*)"1.0 2.0", &result));
    expect_float_to_be(1.0f, result.x);
    expect_float_to_be(2.0f, result.y);

    expect_should_be(true, string_to_vec2((char*)"-5.5 10.25", &result));
    expect_float_to_be(-5.5f, result.x);
    expect_float_to_be(10.25f, result.y);

    expect_should_be(true, string_to_vec2((char*)"0.0 0.0", &result));
    expect_float_to_be(0.0f, result.x);
    expect_float_to_be(0.0f, result.y);

    return true;
}

INTERNAL_FUNC u8 test_string_to_vec2_invalid() {
    vec2 result;

    CORE_DEBUG("The next 2 test cases intentionally test invalid input:");

    // Invalid: non-numeric
    expect_should_be(false, string_to_vec2((char*)"abc def", &result));

    // Invalid: null pointer
    expect_should_be(false, string_to_vec2(nullptr, &result));

    return true;
}

INTERNAL_FUNC u8 test_string_to_vec3_valid() {
    vec3 result;

    expect_should_be(true, string_to_vec3((char*)"1.0 2.0 3.0", &result));
    expect_float_to_be(1.0f, result.x);
    expect_float_to_be(2.0f, result.y);
    expect_float_to_be(3.0f, result.z);

    expect_should_be(true, string_to_vec3((char*)"-1.5 0.0 99.9", &result));
    expect_float_to_be(-1.5f, result.x);
    expect_float_to_be(0.0f, result.y);
    expect_float_to_be(99.9f, result.z);

    return true;
}

INTERNAL_FUNC u8 test_string_to_vec3_invalid() {
    vec3 result;

    CORE_DEBUG("The next test case intentionally tests invalid input:");

    // Invalid: null pointer
    expect_should_be(false, string_to_vec3(nullptr, &result));

    return true;
}

INTERNAL_FUNC u8 test_string_to_vec4_valid() {
    vec4 result;

    expect_should_be(true, string_to_vec4((char*)"1.0 2.0 3.0 4.0", &result));
    expect_float_to_be(1.0f, result.x);
    expect_float_to_be(2.0f, result.y);
    expect_float_to_be(3.0f, result.z);
    expect_float_to_be(4.0f, result.w);

    expect_should_be(true, string_to_vec4((char*)"0.5 0.5 0.5 1.0", &result));
    expect_float_to_be(0.5f, result.r);
    expect_float_to_be(0.5f, result.g);
    expect_float_to_be(0.5f, result.b);
    expect_float_to_be(1.0f, result.a);

    return true;
}

INTERNAL_FUNC u8 test_string_to_vec4_invalid() {
    vec4 result;

    CORE_DEBUG("The next test case intentionally tests invalid input:");

    // Invalid: null pointer
    expect_should_be(false, string_to_vec4(nullptr, &result));

    return true;
}

// Edge Case Tests

INTERNAL_FUNC u8 test_string_parsers_edge_cases() {
    f32 f32_result;
    f64 f64_result;
    vec2 vec2_result;

    // Very large numbers
    expect_should_be(true, string_to_f32((char*)"999999.999", &f32_result));
    expect_float_to_be(999999.999f, f32_result);

    // Very small numbers
    expect_should_be(true, string_to_f32((char*)"0.0001", &f32_result));
    expect_float_to_be(0.0001f, f32_result);

    // Negative zero
    expect_should_be(true, string_to_f64((char*)"-0.0", &f64_result));
    expect_float_to_be(-0.0, f64_result);

    // Extra spaces between numbers (should still work with sscanf)
    expect_should_be(true, string_to_vec2((char*)"1.0    2.0", &vec2_result));
    expect_float_to_be(1.0f, vec2_result.x);
    expect_float_to_be(2.0f, vec2_result.y);

    return true;
}

// Stress Tests

INTERNAL_FUNC u8 test_string_parsers_stress() {
    CORE_INFO("Starting stress test: parsing 10000 floats...");

    u64 memory_before = memory_get_allocations_count();

    for (u32 i = 0; i < 10000; ++i) {
        f32 result;
        char buffer[32];
        string_format(buffer, "%d.%d", i, i % 100);

        if (!string_to_f32(buffer, &result)) {
            CORE_ERROR("Failed to parse on iteration %d", i);
            return false;
        }
    }

    u64 memory_after = memory_get_allocations_count();

    // Check for memory leaks
    expect_should_be(memory_before, memory_after);

    CORE_INFO("Stress test completed: no memory leaks detected");

    return true;
}

INTERNAL_FUNC u8 test_string_vector_parsers_stress() {
    CORE_INFO("Starting stress test: parsing 5000 vectors...");

    u64 memory_before = memory_get_allocations_count();

    for (u32 i = 0; i < 5000; ++i) {
        vec4 result;
        char buffer[64];
        string_format(
            buffer,
            "%d.0 %d.0 %d.0 %d.0",
            i,
            i + 1,
            i + 2,
            i + 3
        );

        if (!string_to_vec4(buffer, &result)) {
            CORE_ERROR("Failed to parse vec4 on iteration %d", i);
            return false;
        }

        // Validate the parsed values
        f32 expected_x = (f32)i;
        f32 expected_y = (f32)(i + 1);
        f32 expected_z = (f32)(i + 2);
        f32 expected_w = (f32)(i + 3);

        if (result.x != expected_x || result.y != expected_y ||
            result.z != expected_z || result.w != expected_w) {
            CORE_ERROR("Incorrect values on iteration %d", i);
            return false;
        }
    }

    u64 memory_after = memory_get_allocations_count();

    // Check for memory leaks
    expect_should_be(memory_before, memory_after);

    CORE_INFO("Vector stress test completed: no memory leaks detected");

    return true;
}

INTERNAL_FUNC u8 test_string_bool_parsers_stress() {
    CORE_INFO("Starting stress test: parsing 10000 booleans...");

    u64 memory_before = memory_get_allocations_count();

    const char* bool_strings[] = {"true", "false", "1", "0", "TRUE", "FALSE"};
    b8 expected_results[] = {true, false, true, false, true, false};

    for (u32 i = 0; i < 10000; ++i) {
        b8 result;
        u32 index = i % 6;

        if (!string_to_bool((char*)bool_strings[index], &result)) {
            CORE_ERROR("Failed to parse bool on iteration %d", i);
            return false;
        }

        if (result != expected_results[index]) {
            CORE_ERROR(
                "Incorrect bool value on iteration %d: expected %d, got %d",
                i,
                expected_results[index],
                result
            );
            return false;
        }
    }

    u64 memory_after = memory_get_allocations_count();

    // Check for memory leaks
    expect_should_be(memory_before, memory_after);

    CORE_INFO("Boolean stress test completed: no memory leaks detected");

    return true;
}

// Memory Leak Tests

INTERNAL_FUNC u8 test_string_parsers_no_memory_leaks() {
    CORE_INFO("Testing for memory leaks with various inputs...");

    u64 memory_before = memory_get_allocations_count();

    // Test with valid inputs
    f32 f;
    f64 d;
    b8 b;
    vec2 v2;
    vec3 v3;
    vec4 v4;

    string_to_f32((char*)"123.456", &f);
    string_to_f64((char*)"789.012", &d);
    string_to_bool((char*)"true", &b);
    string_to_vec2((char*)"1.0 2.0", &v2);
    string_to_vec3((char*)"1.0 2.0 3.0", &v3);
    string_to_vec4((char*)"1.0 2.0 3.0 4.0", &v4);

    // Test with invalid inputs
    string_to_f32((char*)"invalid", &f);
    string_to_f64((char*)"bad_input", &d);
    string_to_bool((char*)"maybe", &b);
    string_to_vec2((char*)"1.0", &v2);
    string_to_vec3((char*)"1.0 2.0", &v3);
    string_to_vec4((char*)"1.0 2.0 3.0", &v4);

    u64 memory_after = memory_get_allocations_count();

    expect_should_be(memory_before, memory_after);

    CORE_INFO("No memory leaks detected with valid and invalid inputs");

    return true;
}

// Null Pointer Safety Tests

INTERNAL_FUNC u8 test_string_parsers_null_safety() {
    f32 f;
    f64 d;
    b8 b;
    vec2 v2;
    vec3 v3;
    vec4 v4;

    CORE_DEBUG("The next 12 test cases test null pointer safety:");

    // Test null string pointers
    expect_should_be(false, string_to_f32(nullptr, &f));
    expect_should_be(false, string_to_f64(nullptr, &d));
    expect_should_be(false, string_to_bool(nullptr, &b));
    expect_should_be(false, string_to_vec2(nullptr, &v2));
    expect_should_be(false, string_to_vec3(nullptr, &v3));
    expect_should_be(false, string_to_vec4(nullptr, &v4));

    // Test null output pointers
    expect_should_be(false, string_to_f32((char*)"123.0", nullptr));
    expect_should_be(false, string_to_f64((char*)"123.0", nullptr));
    expect_should_be(false, string_to_bool((char*)"true", nullptr));
    expect_should_be(false, string_to_vec2((char*)"1.0 2.0", nullptr));
    expect_should_be(false, string_to_vec3((char*)"1.0 2.0 3.0", nullptr));
    expect_should_be(false, string_to_vec4((char*)"1.0 2.0 3.0 4.0", nullptr));

    return true;
}

// Test Registration

void string_register_tests() {
    // Basic parser tests
    test_manager_register_test(
        test_string_to_f32_valid,
        "String: parse f32 valid inputs"
    );
    test_manager_register_test(
        test_string_to_f32_invalid,
        "String: parse f32 invalid inputs"
    );
    test_manager_register_test(
        test_string_to_f64_valid,
        "String: parse f64 valid inputs"
    );
    test_manager_register_test(
        test_string_to_bool_valid,
        "String: parse bool valid inputs"
    );
    test_manager_register_test(
        test_string_to_bool_invalid,
        "String: parse bool invalid inputs"
    );
    test_manager_register_test(
        test_string_to_vec2_valid,
        "String: parse vec2 valid inputs"
    );
    test_manager_register_test(
        test_string_to_vec2_invalid,
        "String: parse vec2 invalid inputs"
    );
    test_manager_register_test(
        test_string_to_vec3_valid,
        "String: parse vec3 valid inputs"
    );
    test_manager_register_test(
        test_string_to_vec3_invalid,
        "String: parse vec3 invalid inputs"
    );
    test_manager_register_test(
        test_string_to_vec4_valid,
        "String: parse vec4 valid inputs"
    );
    test_manager_register_test(
        test_string_to_vec4_invalid,
        "String: parse vec4 invalid inputs"
    );

    // Edge case tests
    test_manager_register_test(
        test_string_parsers_edge_cases,
        "String: parser edge cases"
    );

    // Stress tests
    test_manager_register_test(
        test_string_parsers_stress,
        "String: stress test f32 parsing (10000 iterations)"
    );
    test_manager_register_test(
        test_string_vector_parsers_stress,
        "String: stress test vec4 parsing (5000 iterations)"
    );
    test_manager_register_test(
        test_string_bool_parsers_stress,
        "String: stress test bool parsing (10000 iterations)"
    );

    // Memory leak tests
    test_manager_register_test(
        test_string_parsers_no_memory_leaks,
        "String: verify no memory leaks"
    );

    // Null safety tests
    test_manager_register_test(
        test_string_parsers_null_safety,
        "String: null pointer safety"
    );
}
