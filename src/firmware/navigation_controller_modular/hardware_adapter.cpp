#include "hardware_adapter.h"

#ifdef ARDUINO
#include <Arduino.h>
#else
// Provide lightweight host-side hooks so tests can inspect motor driver behavior.
#include <string>
#include <vector>

namespace firmware_sim {
namespace {
std::vector<GpioWrite> digital_writes;
std::vector<std::string> motor_actions;
}

void reset() {
    digital_writes.clear();
    motor_actions.clear();
}

std::vector<GpioWrite> getDigitalWrites() {
    return digital_writes;
}

std::vector<std::string> getMotorActions() {
    return motor_actions;
}

static void recordMotorAction(const std::string& action) {
    motor_actions.push_back(action);
}

static void recordDigitalWrite(int pin, int value) {
    digital_writes.push_back({pin, value});
}
}

static void pinMode(int, int) {}
static int digitalRead(int) { return 0; }
static void digitalWrite(int pin, int value) { firmware_sim::recordDigitalWrite(pin, value); }
static void delay(int ms) { firmware_sim::recordMotorAction("delay(" + std::to_string(ms) + ")"); }
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
    #ifndef ARDUINO
    firmware_sim::recordMotorAction("stop");
    #endif
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
}

void MotorController::forward() {
    #ifndef ARDUINO
    firmware_sim::recordMotorAction("forward");
    #endif
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
}

void MotorController::turnLeft() {
    #ifndef ARDUINO
    firmware_sim::recordMotorAction("turnLeft");
    #endif
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
}

void MotorController::turnRight() {
    #ifndef ARDUINO
    firmware_sim::recordMotorAction("turnRight");
    #endif
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
}

void MotorController::backward() {
    #ifndef ARDUINO
    firmware_sim::recordMotorAction("backward");
    #endif
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
