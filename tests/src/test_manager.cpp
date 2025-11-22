#include "test_manager.hpp"
#include <data_structures/auto_array.hpp>
#include <core/absolute_clock.hpp>
#include <core/logger.hpp>
#include <core/string.hpp>

struct Test_Entry {
    PFN_test func;
    const char* desc;
};

static Auto_Array<Test_Entry> tests;
static const char* current_module_name = nullptr;

void test_manager_init() {
}

void test_manager_register_test(
    u8 (*PFN_test)(),
    const char* desc
) {
    Test_Entry e;
    e.func = PFN_test;
    e.desc = desc;

    tests.push_back(e);
}

void test_manager_begin_module(const char* module_name) {
    current_module_name = module_name;
    CORE_INFO("");
    CORE_INFO("MODULE: %s", module_name);
    CORE_INFO("");
}

void test_manager_end_module() {
    current_module_name = nullptr;
    tests.clear();
}

void test_manager_run_tests() {
    u32 passed = 0;
    u32 failed = 0;
    u32 skipped = 0;

    u32 count = tests.length;

    Absolute_Clock total_time;
    absolute_clock_start(&total_time);

    for (u32 i = 0; i < count; ++i) {
        CORE_INFO("Running: %s", tests[i].desc);

        u8 result = tests[i].func();

        if (result == true) {
            ++passed;
            CORE_INFO("[PASSED]: %s", tests[i].desc);
        } else if (result == BYPASS) {
            CORE_WARN("[SKIPPED]: %s", tests[i].desc);
            ++skipped;
        } else {
            CORE_WARN("[FAILED]: %s", tests[i].desc);
            ++failed;
        }

        CORE_INFO("");
    }

    absolute_clock_stop(&total_time);

    CORE_INFO("");
    CORE_INFO(
        "Module Summary: %d passed, %d failed, %d skipped (%.6f sec)",
        passed,
        failed,
        skipped,
        total_time.elapsed_time
    );
    CORE_INFO("");
}

