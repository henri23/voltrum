#pragma once
#include "defines.hpp"

#define BYPASS 2

// The test functions can either return 1 for success, or 2 for bypass.
// Any other return value will be considered as failure
using PFN_test = u8 (*)();

void test_manager_init();
void test_manager_register_test(PFN_test test_func, const char* desc);
void test_manager_run_tests();
void test_manager_begin_module(const char* module_name);
void test_manager_end_module();
