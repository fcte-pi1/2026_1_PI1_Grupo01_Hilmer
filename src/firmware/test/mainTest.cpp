#include <Arduino.h>
#include <unity.h>

#include "pin_mapping.h"

void test_motor_1_pins() {

    TEST_ASSERT_EQUAL(6, PIN_M1B1);
    TEST_ASSERT_EQUAL(7, PIN_M1B2);
}

void test_motor_2_pins() {

    TEST_ASSERT_EQUAL(8, PIN_M2B1);
    TEST_ASSERT_EQUAL(9, PIN_M2B2);
}

void test_proximity_pins() {

    TEST_ASSERT_EQUAL(31, PIN_PROX_1);
    TEST_ASSERT_EQUAL(29, PIN_PROX_2);
    TEST_ASSERT_EQUAL(27, PIN_PROX_3);
}

void test_ina226_pins() {

    TEST_ASSERT_EQUAL(25, PIN_INA226_SCL);
    TEST_ASSERT_EQUAL(33, PIN_INA226_SDA);
}


void test_apds9960_pins() {

    TEST_ASSERT_EQUAL(20, PIN_CLK_APDS9960_SCL);
    TEST_ASSERT_EQUAL(12, PIN_RGB_APDS9960_SDA);
}

void setup() {

    delay(2000);

    UNITY_BEGIN();

    RUN_TEST(test_motor_1_pins);
    RUN_TEST(test_motor_2_pins);

    RUN_TEST(test_proximity_pins);

    RUN_TEST(test_ina226_pins);

    RUN_TEST(test_apds9960_pins);

    UNITY_END();
}

void loop() {

}