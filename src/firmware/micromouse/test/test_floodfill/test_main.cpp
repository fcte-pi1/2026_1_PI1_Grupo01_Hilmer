#include <unity.h>

#include "navigation/floodfill.h"
#include "memory/maze_memory.h"

void setUp() {}
void tearDown() {}

void test_floodfill()
{
    floodfill(5);

    TEST_ASSERT_EQUAL(5, maze[0][0]);
    TEST_ASSERT_EQUAL(5, maze[10][10]);
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_floodfill);

    return UNITY_END();
}