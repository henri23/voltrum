#include "test_manager.hpp"

#include <containers/hash_map_tests.hpp>
#include <core/logger.hpp>

int main() {
    test_manager_init();

    CORE_DEBUG("Starting tests...");
    hash_map_register_tests();

    test_manager_run_tests();

    return 0;
}
