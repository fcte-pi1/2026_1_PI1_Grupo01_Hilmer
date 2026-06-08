#include "hal_sensors.h"

#ifdef TARGET_ESP32

#include <Arduino.h>

#define FRONT_SENSOR_PIN 34
#define LEFT_SENSOR_PIN 35
#define RIGHT_SENSOR_PIN 32

float hal_read_front_sensor()
{
    return analogRead(FRONT_SENSOR_PIN);
}

float hal_read_left_sensor()
{
    return analogRead(LEFT_SENSOR_PIN);
}

float hal_read_right_sensor()
{
    return analogRead(RIGHT_SENSOR_PIN);
}

#endif