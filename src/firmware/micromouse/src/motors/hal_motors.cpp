#include "hal_motors.h"

#ifdef TARGET_ESP32

#include <Arduino.h>

#define LEFT_PWM_PIN 25
#define RIGHT_PWM_PIN 26

void hal_motor_left(int speed)
{
    analogWrite(LEFT_PWM_PIN, speed);
}

void hal_motor_right(int speed)
{
    analogWrite(RIGHT_PWM_PIN, speed);
}

#endif