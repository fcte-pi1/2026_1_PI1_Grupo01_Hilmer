#include <unity.h>

#include "memory/maze_memory.h"

void setUp() {}
void tearDown() {}

void test_maze_memory()
{
    maze[1][1] = 42;

    TEST_ASSERT_EQUAL(42, maze[1][1]);
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_maze_memory);

    return UNITY_END();
}