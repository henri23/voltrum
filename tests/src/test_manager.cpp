#include "test_manager.hpp"
#include <data_structures/dynamic_array.hpp>
#include <core/absolute_clock.hpp>
#include <core/logger.hpp>
#include <memory/arena.hpp>
#include <utils/string.hpp>

struct Test_Entry {
    PFN_test func;
    const char* desc;
};

static Arena *test_arena = nullptr;
static Dynamic_Array<Test_Entry> tests;
static const char* current_module_name = nullptr;

void test_manager_init() {
    test_arena = arena_create();
    tests.init(test_arena);
}

void test_manager_register_test(
    u8 (*PFN_test)(),
    const char* desc
) {
    Test_Entry e;
    e.func = PFN_test;
    e.desc = desc;

    tests.add(e);
}

void test_manager_begin_module(const char* module_name) {
    current_module_name = module_name;
    CORE_INFO("");
    CORE_INFO("MODULE: %s", module_name);
    CORE_INFO("");
}

void test_manager_end_module() {
    current_module_name = nullptr;
    arena_clear(test_arena);
    tests.init(test_arena);
}

void test_manager_run_tests() {
    u32 passed = 0;
    u32 failed = 0;
    u32 skipped = 0;

    u32 count = tests.size;

    Absolute_Clock total_time;
    absolute_clock_start(&total_time);

    for (u32 i = 0; i < count; ++i) {
        CORE_INFO("[RUNNING] %s", tests[i].desc);

        Absolute_Clock test_time;
        absolute_clock_start(&test_time);
        u8 result = tests[i].func();
        absolute_clock_update(&test_time);

        f64 elapsed_us = test_time.elapsed_time * 1000000.0;

        if (result == true) {
            ++passed;
            CORE_INFO("[PASSED] %s (%.2f μs)", tests[i].desc, elapsed_us);
        } else if (result == BYPASS) {
            CORE_WARN("[SKIPPED] %s (%.2f μs)", tests[i].desc, elapsed_us);
            ++skipped;
        } else {
            CORE_WARN("[FAILED] %s (%.2f μs)", tests[i].desc, elapsed_us);
            ++failed;
        }

        CORE_INFO("");
    }

    absolute_clock_update(&total_time);

    f64 total_ms = total_time.elapsed_time * 1000.0;

    absolute_clock_stop(&total_time);

    CORE_INFO("");
    CORE_INFO(
        "[SUMMARY] %d passed, %d failed, %d skipped (%.2f ms)",
        passed,
        failed,
        skipped,
        total_ms
    );
    CORE_INFO("");
}

