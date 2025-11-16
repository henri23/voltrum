#include "hash_map_tests.hpp"
#include "expect.hpp"
#include "test_manager.hpp"

#include <data_structures/hash_map.hpp>
#include <defines.hpp>
#include <core/logger.hpp>
#include <string.h>

INTERNAL_FUNC u8 test_creation_and_deletion() {
    Hash_Map<int> map(3); // capacity should round up to 4

    expect_should_be(4, map.capacity);
    expect_should_be(0, map.count);

    int value = 42;
    expect_should_be(true, map.add("answer", &value));
    expect_should_be(1, map.count);

    expect_should_be(true, map.remove("answer"));
    expect_should_be(0, map.count);

    return true;
}

INTERNAL_FUNC u8 test_add_find_remove() {
    Hash_Map<int> map(4);

    int v1 = 10;
    int v2 = 20;

    expect_should_be(true, map.add("a", &v1));
    expect_should_be(true, map.add("b", &v2));
    expect_should_be(2, map.count);

    int out = 0;
    expect_should_be(true, map.find("a", &out));
    expect_should_be(v1, out);

    expect_should_be(true, map.remove("a"));
    expect_should_be(1, map.count);

    CORE_DEBUG("The next warning about a missing key is expected (lookup after removal).");
    expect_should_be(false, map.find("a", &out));

    expect_should_be(true, map.find("b", &out));
    expect_should_be(v2, out);

    return true;
}

INTERNAL_FUNC u8 test_expected_warnings() {
    Hash_Map<int> map(2);
    int v = 7;

    expect_should_be(true, map.add("dup", &v));

    CORE_DEBUG("The next warning about duplicate key is expected.");
    expect_should_be(false, map.add("dup", &v));
    expect_should_be(1, map.count);

    CORE_DEBUG("The next warning about removing a missing key is expected.");
    expect_should_be(false, map.remove("missing"));
    expect_should_be(1, map.count);

    char long_key[55];
    memset(long_key, 'x', sizeof(long_key));
    long_key[54] = '\0';

    CORE_DEBUG("The next warning about an overlength key is expected.");
    expect_should_be(false, map.add(long_key, &v));
    expect_should_be(1, map.count);

    return true;
}

INTERNAL_FUNC u8 test_debug_log_showcase() {
    Hash_Map<int> map(6);

    int v1 = 1;
    int v2 = 2;
    int v3 = 3;

    expect_should_be(true, map.add("one", &v1));
    expect_should_be(true, map.add("two", &v2));
    expect_should_be(true, map.add("three", &v3));

    CORE_DEBUG("HashMap debug log (after insertions) follows:");
    map.debug_log_table();

    expect_should_be(true, map.remove("two"));
    expect_should_be(2, map.count);

    CORE_DEBUG("HashMap debug log (after removal) follows:");
    map.debug_log_table();

    return true;
}

void hash_map_register_tests() {
    test_manager_register_test(test_creation_and_deletion,
        "Hash_Map: creation and deletion");
    test_manager_register_test(test_add_find_remove,
        "Hash_Map: add, find, remove");
    test_manager_register_test(test_expected_warnings,
        "Hash_Map: expected warning scenarios");
    test_manager_register_test(test_debug_log_showcase,
        "Hash_Map: debug log showcase");
}
