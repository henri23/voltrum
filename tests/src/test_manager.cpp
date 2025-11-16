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

void test_manager_init() {
}

void test_manager_register_test(u8 (*PFN_test)(), const char* desc) {
    Test_Entry e;
    e.func = PFN_test;
    e.desc = desc;

    tests.push_back(e);
}

void test_manager_run_tests() {
    u32 passed = 0;
    u32 failed = 0;
    u32 skipped = 0;

    u32 count = tests.length;

    Absolute_Clock total_time;
    absolute_clock_start(&total_time);

    for (u32 i = 0; i < count; ++i) {
        // Measure the duration of the singular test execution time
        Absolute_Clock test_time;
        absolute_clock_start(&test_time);
        u8 result = tests[i].func();
        absolute_clock_update(&test_time);

        if (result == true) {
            ++passed;
        } else if (result == BYPASS) {
            CORE_WARN("[SKIPPED]: %s", tests[i].desc);
            ++skipped;
        } else {
            CORE_WARN("[FAILED]: %s", tests[i].desc);
            ++failed;
        }

        char status[20];
        string_format(status, failed ? "*** %d FAILED ***" : "SUCCESS", failed);
        absolute_clock_update(&total_time);

        CORE_INFO(
            "Executed %d of %d (skipped %d) %s (%.6f sec / %.6f sec total)",
            i + 1,
            count,
            skipped,
            status,
            test_time.elapsed_time,
            total_time.elapsed_time);
    }

    absolute_clock_stop(&total_time);

    CORE_INFO(
        "Results: %d passed, %d failed, %d skipped.",
        passed,
        failed,
        skipped);
}

