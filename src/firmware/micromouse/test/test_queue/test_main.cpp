#include <unity.h>

#include "navigation/queue.h"

void setUp() {}
void tearDown() {}

void test_queue_push_pop()
{
    head = 0;
    tail = 0;

    push(1, 2);

    QueueNode n = pop();

    TEST_ASSERT_EQUAL(1, n.x);
    TEST_ASSERT_EQUAL(2, n.y);
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_queue_push_pop);

    return UNITY_END();
}