#include "hal_motors.h"

#ifdef TARGET_ESP32

#include <Arduino.h>

// Definidos de acordo com navigation_controller.cpp
#define MOTOR_IN1 25 // Esquerda Avanca
#define MOTOR_IN2 26 // Esquerda Recua
#define MOTOR_IN3 27 // Direita Avanca
#define MOTOR_IN4 14 // Direita Recua
#define MOTOR_MAX_PWM 255

void hal_motors_init()
{
    pinMode(MOTOR_IN1, OUTPUT);
    pinMode(MOTOR_IN2, OUTPUT);
    pinMode(MOTOR_IN3, OUTPUT);
    pinMode(MOTOR_IN4, OUTPUT);
    
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, LOW);
    digitalWrite(MOTOR_IN3, LOW);
    digitalWrite(MOTOR_IN4, LOW);
}

void hal_motor_left(int speed)
{
    if (speed > 0) {
        analogWrite(MOTOR_IN1, speed);
        digitalWrite(MOTOR_IN2, LOW);
    } else if (speed < 0) {
        digitalWrite(MOTOR_IN1, LOW);
        analogWrite(MOTOR_IN2, -speed);
    } else {
        digitalWrite(MOTOR_IN1, LOW);
        digitalWrite(MOTOR_IN2, LOW);
    }
}

void hal_motor_right(int speed)
{
    if (speed > 0) {
        analogWrite(MOTOR_IN3, speed);
        digitalWrite(MOTOR_IN4, LOW);
    } else if (speed < 0) {
        digitalWrite(MOTOR_IN3, LOW);
        analogWrite(MOTOR_IN4, -speed);
    } else {
        digitalWrite(MOTOR_IN3, LOW);
        digitalWrite(MOTOR_IN4, LOW);
    }
}

#endif