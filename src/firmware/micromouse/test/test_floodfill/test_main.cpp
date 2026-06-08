#include <unity.h>

#include "navigation/floodfill_engine.h"
#include "memory/maze_memory.h"

void setUp(void)
{
    floodfill_init();
}

void tearDown(void) {}

void test_floodfill_center_distances_empty_maze(void)
{
    floodfill_complete('C');

    int mid = MAZE_SIZE / 2;
    TEST_ASSERT_EQUAL(0, manhattan_dist[mid - 1][mid - 1]);
    TEST_ASSERT_EQUAL(0, manhattan_dist[mid][mid]);
    TEST_ASSERT_EQUAL(14, manhattan_dist[0][0]);
}

void test_floodfill_start_target_empty_maze(void)
{
    floodfill_complete('S');

    TEST_ASSERT_EQUAL(0, manhattan_dist[0][0]);
    TEST_ASSERT_EQUAL(30, manhattan_dist[MAZE_SIZE - 1][MAZE_SIZE - 1]);
}

void test_floodfill_modified_after_wall(void)
{
    floodfill_complete('C');

    horiz_walls[5][5] = true;
    horiz_walls[5][6] = true;

    floodfill_queue_reset();
    floodfill_queue_push(4, 5);
    floodfill_queue_push(4, 6);
    floodfill_queue_push(5, 5);
    floodfill_queue_push(5, 6);

    floodfill_modified('C');

    TEST_ASSERT_TRUE(manhattan_dist[4][5] == floodfill_get_min_neighbor(4, 5) + 1);
    TEST_ASSERT_TRUE(manhattan_dist[4][6] == floodfill_get_min_neighbor(4, 6) + 1);
}

void test_floodfill_is_target_center(void)
{
    int mid = MAZE_SIZE / 2;
    TEST_ASSERT_TRUE(floodfill_is_target(mid, mid, 'C'));
    TEST_ASSERT_FALSE(floodfill_is_target(0, 0, 'C'));
    TEST_ASSERT_TRUE(floodfill_is_target(0, 0, 'S'));
}

void test_floodfill_legacy_maze_alias(void)
{
    floodfill_complete('C');
    TEST_ASSERT_EQUAL(manhattan_dist[0][0], maze[0][0]);
    TEST_ASSERT_EQUAL(manhattan_dist[7][7], maze[7][7]);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_floodfill_center_distances_empty_maze);
    RUN_TEST(test_floodfill_start_target_empty_maze);
    RUN_TEST(test_floodfill_modified_after_wall);
    RUN_TEST(test_floodfill_is_target_center);
    RUN_TEST(test_floodfill_legacy_maze_alias);

    return UNITY_END();
}
