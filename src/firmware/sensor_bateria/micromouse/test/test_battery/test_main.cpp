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

void test_simulated_voltage()
{
    float voltage = readVoltage();

    TEST_ASSERT_FLOAT_WITHIN(
        0.01f,
        7.4f,
        voltage);
}

void test_simulated_current()
{
    float current = readCurrent();

    TEST_ASSERT_FLOAT_WITHIN(
        0.01f,
        0.35f,
        current);
}

void test_simulated_power()
{
    float power = readPower();

    TEST_ASSERT_FLOAT_WITHIN(
        0.05f,
        2.59f,
        power);
}

void test_power_consistency()
{
    float voltage = readVoltage();

    float current = readCurrent();

    float expected = voltage * current;

    float measured = readPower();

    TEST_ASSERT_FLOAT_WITHIN(
        0.05f,
        expected,
        measured);
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_low_battery);

    RUN_TEST(test_normal_battery);

    RUN_TEST(test_simulated_voltage);

    RUN_TEST(test_simulated_current);

    RUN_TEST(test_simulated_power);

    RUN_TEST(test_power_consistency);

    return UNITY_END();
}