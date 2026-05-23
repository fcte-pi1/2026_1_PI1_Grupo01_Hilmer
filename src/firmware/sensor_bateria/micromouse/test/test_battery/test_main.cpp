#include <unity.h>

#include "battery.h"

void setUp()
{
}

void tearDown()
{
}

void test_low_battery()
{
    TEST_ASSERT_TRUE(lowBattery(5.9f));
}

void test_normal_battery()
{
    TEST_ASSERT_FALSE(lowBattery(7.4f));
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_low_battery);

    RUN_TEST(test_normal_battery);

    return UNITY_END();
}