#include "hashmap_tests.hpp"
#include "expect.hpp"
#include "test_manager.hpp"

#include <core/logger.hpp>
#include <data_structures/hashmap.hpp>
#include <defines.hpp>
#include <memory/arena.hpp>
#include <memory/memory.hpp>
#include <utils/string.hpp>

static Arena *test_arena = nullptr;

INTERNAL_FUNC u8
test_creation_and_deletion()
{
    Hashmap<int> map;
    map.init(test_arena, 3); // capacity should round up to 4

    expect_should_be(4, map.capacity);
    expect_should_be(0, map.count);

    int value = 42;
    expect_should_be(true, map.add(STR_LIT("answer"), &value));
    expect_should_be(1, map.count);

    expect_should_be(true, map.remove(STR_LIT("answer")));
    expect_should_be(0, map.count);

    return true;
}

INTERNAL_FUNC u8
test_add_find_remove()
{
    Hashmap<int> map;
    map.init(test_arena, 4);

    int v1 = 10;
    int v2 = 20;

    expect_should_be(true, map.add(STR_LIT("a"), &v1));
    expect_should_be(true, map.add(STR_LIT("b"), &v2));
    expect_should_be(2, map.count);

    // Test find_ptr
    int *out_ptr = nullptr;
    expect_should_be(true, map.find_ptr(STR_LIT("a"), &out_ptr));
    expect_should_be(v1, *out_ptr);

    // Test find
    int out_copy = 0;
    expect_should_be(true, map.find(STR_LIT("a"), &out_copy));
    expect_should_be(v1, out_copy);

    expect_should_be(true, map.remove(STR_LIT("a")));
    expect_should_be(1, map.count);

    CORE_DEBUG(
        "The next 2 warnings about a missing key are expected (lookup after "
        "removal).");
    expect_should_be(false, map.find_ptr(STR_LIT("a"), &out_ptr));
    expect_should_be(false, map.find(STR_LIT("a"), &out_copy));

    // Test both find methods on remaining element
    expect_should_be(true, map.find_ptr(STR_LIT("b"), &out_ptr));
    expect_should_be(v2, *out_ptr);

    out_copy = 0;
    expect_should_be(true, map.find(STR_LIT("b"), &out_copy));
    expect_should_be(v2, out_copy);

    return true;
}

INTERNAL_FUNC u8
test_expected_warnings()
{
    Hashmap<int> map;
    map.init(test_arena, 2);
    int v = 7;

    expect_should_be(true, map.add(STR_LIT("dup"), &v));

    CORE_DEBUG("The next warning about duplicate key is expected.");
    expect_should_be(false, map.add(STR_LIT("dup"), &v));
    expect_should_be(1, map.count);

    CORE_DEBUG("The next warning about removing a missing key is expected.");
    expect_should_be(false, map.remove(STR_LIT("missing")));
    expect_should_be(1, map.count);

    return true;
}

INTERNAL_FUNC u8
test_full_hashmap_does_not_change_size()
{
    Hashmap<int> map;
    map.init(test_arena, 4); // Should resolve to capacity 4

    int values[5] = {10, 20, 30, 40, 50};

    expect_should_be(true, map.add(STR_LIT("one"), &values[0]));
    expect_should_be(true, map.add(STR_LIT("two"), &values[1]));
    expect_should_be(true, map.add(STR_LIT("three"), &values[2]));
    expect_should_be(true, map.add(STR_LIT("four"), &values[3]));

    expect_should_be(map.capacity, map.count);

    CORE_DEBUG("The next warning about a full hashmap is expected.");
    expect_should_be(false, map.add(STR_LIT("overflow"), &values[4]));
    expect_should_be(map.capacity, map.count);

    return true;
}

INTERNAL_FUNC u8
test_debug_log_showcase()
{
    Hashmap<int> map;
    map.init(test_arena, 6);

    int v1 = 1;
    int v2 = 2;
    int v3 = 3;

    expect_should_be(true, map.add(STR_LIT("one"), &v1));
    expect_should_be(true, map.add(STR_LIT("two"), &v2));
    expect_should_be(true, map.add(STR_LIT("three"), &v3));

    CORE_DEBUG("HashMap debug log (after insertions) follows:");
    map.debug_log_table();

    expect_should_be(true, map.remove(STR_LIT("two")));
    expect_should_be(2, map.count);

    CORE_DEBUG("HashMap debug log (after removal) follows:");
    map.debug_log_table();

    return true;
}

INTERNAL_FUNC u8
test_init_and_shutdown()
{
    Hashmap<int> map;

    // Verify initial state
    expect_should_be(0, map.capacity);
    expect_should_be(0, map.count);
    expect_should_be(nullptr, map.items);

    // Initialize with capacity 5, should round to 8
    map.init(test_arena, 5);
    expect_should_be(8, map.capacity);
    expect_should_be(0, map.count);
    expect_should_not_be(nullptr, map.items);

    // Add some data
    int value = 100;
    expect_should_be(true, map.add(STR_LIT("test"), &value));
    expect_should_be(1, map.count);

    // Shutdown — arena manages memory, just zero the struct
    memory_zero(&map, sizeof(map));
    expect_should_be(0, map.capacity);
    expect_should_be(0, map.count);
    expect_should_be(nullptr, map.items);

    // Re-initialize with different capacity
    arena_clear(test_arena);
    map.init(test_arena, 3);
    expect_should_be(4, map.capacity);
    expect_should_be(0, map.count);
    expect_should_not_be(nullptr, map.items);

    return true;
}

INTERNAL_FUNC u8
test_find_ptr_vs_find()
{
    Hashmap<int> map;
    map.init(test_arena, 4);

    int original_value = 100;
    expect_should_be(true, map.add(STR_LIT("test"), &original_value));

    // Test find_ptr returns a pointer to the internal value
    int *ptr_result = nullptr;
    expect_should_be(true, map.find_ptr(STR_LIT("test"), &ptr_result));
    expect_should_be(100, *ptr_result);

    // Test find returns a copy of the value
    int copy_result = 0;
    expect_should_be(true, map.find(STR_LIT("test"), &copy_result));
    expect_should_be(100, copy_result);

    // Modify through the pointer should affect the hashmap
    *ptr_result = 200;

    // Verify the hashmap value changed
    int *ptr_check = nullptr;
    expect_should_be(true, map.find_ptr(STR_LIT("test"), &ptr_check));
    expect_should_be(200, *ptr_check);

    // Verify the copy_result is still the old value (independent)
    expect_should_be(100, copy_result);

    // Get a new copy and verify it has the updated value
    int new_copy = 0;
    expect_should_be(true, map.find(STR_LIT("test"), &new_copy));
    expect_should_be(200, new_copy);

    // Modify the copy should NOT affect the hashmap
    new_copy       = 300;
    int *final_ptr = nullptr;
    expect_should_be(true, map.find_ptr(STR_LIT("test"), &final_ptr));
    expect_should_be(200, *final_ptr);

    return true;
}

INTERNAL_FUNC u8
test_operations_before_init()
{
    Hashmap<int> map;
    int          value    = 42;
    int         *out_ptr  = nullptr;
    int          out_copy = 0;

    CORE_DEBUG("The next 4 errors about uninitialized hashmap are expected.");

    // Try to add before init
    expect_should_be(false, map.add(STR_LIT("key"), &value));

    // Try to find_ptr before init
    expect_should_be(false, map.find_ptr(STR_LIT("key"), &out_ptr));

    // Try to find before init
    expect_should_be(false, map.find(STR_LIT("key"), &out_copy));

    // Try to remove before init
    expect_should_be(false, map.remove(STR_LIT("key")));

    // Verify map is still in uninitialized state
    expect_should_be(0, map.capacity);
    expect_should_be(0, map.count);
    expect_should_be(nullptr, map.items);

    // Now properly initialize and verify operations work
    map.init(test_arena, 4);
    expect_should_be(true, map.add(STR_LIT("key"), &value));
    expect_should_be(true, map.find_ptr(STR_LIT("key"), &out_ptr));
    expect_should_be(value, *out_ptr);

    out_copy = 0;
    expect_should_be(true, map.find(STR_LIT("key"), &out_copy));
    expect_should_be(value, out_copy);

    return true;
}

INTERNAL_FUNC u8
test_add_with_overwrite()
{
    Hashmap<int> map;
    map.init(test_arena, 4);

    int original_value = 100;
    int new_value      = 200;

    // Add initial value
    expect_should_be(true, map.add(STR_LIT("key"), &original_value));
    expect_should_be(1, map.count);

    // Verify initial value
    int *ptr_result = nullptr;
    expect_should_be(true, map.find_ptr(STR_LIT("key"), &ptr_result));
    expect_should_be(100, *ptr_result);

    CORE_DEBUG("The next warning about duplicate key is expected.");
    // Try to add without overwrite flag (should fail)
    expect_should_be(false, map.add(STR_LIT("key"), &new_value, false));
    expect_should_be(1, map.count);

    // Verify value is still the original
    expect_should_be(true, map.find_ptr(STR_LIT("key"), &ptr_result));
    expect_should_be(100, *ptr_result);

    // Now add with overwrite flag (should succeed)
    expect_should_be(true, map.add(STR_LIT("key"), &new_value, true));
    expect_should_be(1, map.count); // Count should NOT increment on overwrite

    // Verify value has been updated
    expect_should_be(true, map.find_ptr(STR_LIT("key"), &ptr_result));
    expect_should_be(200, *ptr_result);

    // Test overwrite with a different key to ensure normal operation still
    // works
    int another_value = 300;
    expect_should_be(true, map.add(STR_LIT("another_key"), &another_value, true));
    expect_should_be(2, map.count);

    expect_should_be(true, map.find_ptr(STR_LIT("another_key"), &ptr_result));
    expect_should_be(300, *ptr_result);

    return true;
}

void
hashmap_register_tests()
{
    test_arena = arena_create();

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
    test_manager_register_test(test_init_and_shutdown,
                               "Hash_Map: init and shutdown behavior");
    test_manager_register_test(test_find_ptr_vs_find,
                               "Hash_Map: find_ptr vs find behavior");
    test_manager_register_test(test_operations_before_init,
                               "Hash_Map: operations before initialization");
    test_manager_register_test(test_add_with_overwrite,
                               "Hash_Map: add with overwrite flag");
}
