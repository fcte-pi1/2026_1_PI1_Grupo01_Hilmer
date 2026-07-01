#include <unity.h>

#include "utils/bitmask.h"

void setUp() {}
void tearDown() {}

void test_set_wall()
{
    uint8_t cell = 0;

    setWall(&cell, NORTH);

    TEST_ASSERT_TRUE(hasWall(cell, NORTH));
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_set_wall);

    return UNITY_END();
}