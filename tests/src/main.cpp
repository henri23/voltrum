#include "test_manager.hpp"

#include <containers/hashmap_tests.hpp>
#include <containers/ring_queue_tests.hpp>
#include <core/string_tests.hpp>
#include <core/logger.hpp>

int main() {
    test_manager_init();

    CORE_DEBUG("Starting tests...");

    test_manager_begin_module("Hashmap");
    hashmap_register_tests();
    test_manager_run_tests();
    test_manager_end_module();

    test_manager_begin_module("Ring_Queue");
    ring_queue_register_tests();
    test_manager_run_tests();
    test_manager_end_module();

    test_manager_begin_module("String");
    str_register_tests();
    test_manager_run_tests();
    test_manager_end_module();

    return 0;
}
