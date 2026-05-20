#include "hardware_adapter.h"

#ifdef ARDUINO
#include <Arduino.h>
#else
// Provide lightweight stubs so code compiles on host for tests.
#include <iostream>
static void pinMode(int, int) {}
static int digitalRead(int) { return 0; }
static void digitalWrite(int, int) {}
static void delay(int) {}
#define INPUT 0
#define OUTPUT 1
#define HIGH 1
#define LOW 0
#endif

SensorManager::SensorManager(int front,int right,int left)
    : FRONT_SENSOR(front), RIGHT_SENSOR(right), LEFT_SENSOR(left) {}

void SensorManager::begin() {
    pinMode(FRONT_SENSOR, INPUT);
    pinMode(RIGHT_SENSOR, INPUT);
    pinMode(LEFT_SENSOR, INPUT);
}

bool SensorManager::wallFront() { return digitalRead(FRONT_SENSOR); }
bool SensorManager::wallRight() { return digitalRead(RIGHT_SENSOR); }
bool SensorManager::wallLeft()  { return digitalRead(LEFT_SENSOR); }

MotorController::MotorController(int in1,int in2,int in3,int in4)
    : IN1(in1), IN2(in2), IN3(in3), IN4(in4) {}

void MotorController::begin() {
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
}

void MotorController::stop() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
}

void MotorController::forward() {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
}

void MotorController::turnLeft() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
}

void MotorController::turnRight() {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
}

void MotorController::backward() {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
}

void MotorController::execute(MoveCommand cmd) {
    switch(cmd) {
        case MOVE_FORWARD: forward(); break;
        case TURN_LEFT: turnLeft(); delay(220); forward(); break;
        case TURN_RIGHT: turnRight(); delay(220); forward(); break;
        case TURN_BACK: turnRight(); delay(450); forward(); break;
        case STOP: stop(); break;
    }
}
