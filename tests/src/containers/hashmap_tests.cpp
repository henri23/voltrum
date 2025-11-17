#include "hashmap_tests.hpp"
#include "expect.hpp"
#include "test_manager.hpp"

#include <core/logger.hpp>
#include <data_structures/hashmap.hpp>
#include <defines.hpp>
#include <string.h>

INTERNAL_FUNC u8 test_creation_and_deletion() {
    Hashmap<int> map(3); // capacity should round up to 4

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
    Hashmap<int> map(4);

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

    CORE_DEBUG(
        "The next warning about a missing key is expected (lookup after "
        "removal).");
    expect_should_be(false, map.find("a", &out));

    expect_should_be(true, map.find("b", &out));
    expect_should_be(v2, out);

    return true;
}

INTERNAL_FUNC u8 test_expected_warnings() {
    Hashmap<int> map(2);
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

    CORE_DEBUG("The next error about an overlength key is expected.");
    expect_should_be(false, map.add(long_key, &v));
    expect_should_be(1, map.count);

    return true;
}

INTERNAL_FUNC u8 test_full_hashmap_does_not_change_size() {
    Hashmap<int> map(4); // Should resolve to capacity 4

    int values[5] = {10, 20, 30, 40, 50};

    expect_should_be(true, map.add("one", &values[0]));
    expect_should_be(true, map.add("two", &values[1]));
    expect_should_be(true, map.add("three", &values[2]));
    expect_should_be(true, map.add("four", &values[3]));

    expect_should_be(map.capacity, map.count);

    CORE_DEBUG("The next warning about a full hashmap is expected.");
    expect_should_be(false, map.add("overflow", &values[4]));
    expect_should_be(map.capacity, map.count);

    return true;
}

INTERNAL_FUNC u8 test_debug_log_showcase() {
    Hashmap<int> map(6);

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

void hashmap_register_tests() {
    test_manager_register_test(test_creation_and_deletion,
        "Hash_Map: creation and deletion");
    test_manager_register_test(test_add_find_remove,
        "Hash_Map: add, find, remove");
    test_manager_register_test(test_expected_warnings,
        "Hash_Map: expected warning scenarios");
    test_manager_register_test(test_debug_log_showcase,
        "Hash_Map: debug log showcase");
    test_manager_register_test(test_full_hashmap_does_not_change_size,
        "Hash_Map: full hashmap does not change size");
}
