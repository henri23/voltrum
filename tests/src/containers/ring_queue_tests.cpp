#include "ring_queue_tests.hpp"
#include "expect.hpp"
#include "test_manager.hpp"

#include <core/logger.hpp>
#include <data_structures/ring_queue.hpp>
#include <defines.hpp>
#include <memory/arena.hpp>

static Arena *test_arena = nullptr;

INTERNAL_FUNC u8
test_init()
{
    Ring_Queue<int> queue;
    queue.init(test_arena, 8);

    expect_should_be(8, queue.capacity);
    expect_should_be(0, queue.count);
    expect_should_be(0, queue.head);
    expect_should_be(0, queue.tail);
    expect_should_not_be(nullptr, queue.elements);

    return true;
}

INTERNAL_FUNC u8
test_enqueue_dequeue()
{
    Ring_Queue<int> queue;
    queue.init(test_arena, 4);

    int v1 = 10;
    int v2 = 20;
    int v3 = 30;

    queue.enqueue(v1);
    queue.enqueue(v2);
    queue.enqueue(v3);
    expect_should_be(3, queue.count);

    int out = 0;
    expect_should_be(true, queue.dequeue(&out));
    expect_should_be(10, out);

    expect_should_be(true, queue.dequeue(&out));
    expect_should_be(20, out);

    expect_should_be(true, queue.dequeue(&out));
    expect_should_be(30, out);

    expect_should_be(0, queue.count);

    return true;
}

INTERNAL_FUNC u8
test_peek()
{
    Ring_Queue<int> queue;
    queue.init(test_arena, 4);

    expect_should_be(nullptr, queue.peek());

    int v = 42;
    queue.enqueue(v);

    int *peeked = queue.peek();
    expect_should_not_be(nullptr, peeked);
    expect_should_be(42, *peeked);

    // peek should not remove the element
    expect_should_be(1, queue.count);

    return true;
}

INTERNAL_FUNC u8
test_empty_dequeue()
{
    Ring_Queue<int> queue;
    queue.init(test_arena, 4);

    int out = 0;
    expect_should_be(true, queue.is_empty());
    expect_should_be(false, queue.dequeue(&out));
    expect_should_be(0, queue.count);

    return true;
}

INTERNAL_FUNC u8
test_fill_drain_refill()
{
    Ring_Queue<int> queue;
    queue.init(test_arena, 3);

    // Fill to capacity
    queue.enqueue(10);
    queue.enqueue(20);
    queue.enqueue(30);
    expect_should_be(true, queue.is_full());

    // Drain one element to make room
    int out = 0;
    expect_should_be(true, queue.dequeue(&out));
    expect_should_be(10, out);
    expect_should_be(false, queue.is_full());

    // Now enqueue should succeed again
    queue.enqueue(40);
    expect_should_be(true, queue.is_full());

    // Verify remaining order: 20, 30, 40
    expect_should_be(true, queue.dequeue(&out));
    expect_should_be(20, out);
    expect_should_be(true, queue.dequeue(&out));
    expect_should_be(30, out);
    expect_should_be(true, queue.dequeue(&out));
    expect_should_be(40, out);

    expect_should_be(true, queue.is_empty());

    return true;
}

INTERNAL_FUNC u8
test_wraparound()
{
    Ring_Queue<int> queue;
    queue.init(test_arena, 4);

    // Fill, drain, and refill multiple times to force head/tail wraparound
    for (int cycle = 0; cycle < 3; cycle++)
    {
        // Fill to capacity
        for (int i = 0; i < 4; i++)
        {
            queue.enqueue(cycle * 100 + i);
        }
        expect_should_be(true, queue.is_full());

        // Drain all and verify FIFO order
        int out = 0;
        for (int i = 0; i < 4; i++)
        {
            expect_should_be(true, queue.dequeue(&out));
            expect_should_be(cycle * 100 + i, out);
        }
        expect_should_be(true, queue.is_empty());
    }

    return true;
}

INTERNAL_FUNC u8
test_reset()
{
    Ring_Queue<int> queue;
    queue.init(test_arena, 4);

    queue.enqueue(1);
    queue.enqueue(2);
    queue.enqueue(3);
    expect_should_be(3, queue.count);

    queue.reset();

    expect_should_be(0, queue.count);
    expect_should_be(0, queue.head);
    expect_should_be(0, queue.tail);
    expect_should_be(true, queue.is_empty());

    // Queue should be usable again after reset
    queue.enqueue(100);
    expect_should_be(1, queue.count);

    int out = 0;
    expect_should_be(true, queue.dequeue(&out));
    expect_should_be(100, out);

    return true;
}

void
ring_queue_register_tests()
{
    test_arena = arena_create();

    test_manager_register_test(test_init,
                               "Ring_Queue: initialization");
    test_manager_register_test(test_enqueue_dequeue,
                               "Ring_Queue: enqueue and dequeue");
    test_manager_register_test(test_peek,
                               "Ring_Queue: peek behavior");
    test_manager_register_test(test_empty_dequeue,
                               "Ring_Queue: dequeue from empty queue");
    test_manager_register_test(test_fill_drain_refill,
                               "Ring_Queue: fill, drain, refill cycle");
    test_manager_register_test(test_wraparound,
                               "Ring_Queue: wraparound");
    test_manager_register_test(test_reset,
                               "Ring_Queue: reset clears state");
}
