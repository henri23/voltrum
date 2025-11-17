#include "test_manager.hpp"

#include <containers/hashmap_tests.hpp>
#include <core/logger.hpp>

int main() {
    test_manager_init();

    CORE_DEBUG("Starting tests...");
    hashmap_register_tests();

    test_manager_run_tests();

    return 0;
}
