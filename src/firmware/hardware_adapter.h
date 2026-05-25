#pragma once

#include "navigation_core.h"

#ifndef ARDUINO
#include <string>
#include <vector>

namespace firmware_sim {
struct GpioWrite {
    int pin;
    int value;
};

void reset();
std::vector<GpioWrite> getDigitalWrites();
std::vector<std::string> getMotorActions();
}
#endif

// SensorManager and MotorController are provided as hardware adapters.
// The implementation for ESP32 should include Arduino.h and control real GPIOs.

class SensorManager {
public:
    // GPIO pins (override in implementation if needed)
    const int FRONT_SENSOR;
    const int RIGHT_SENSOR;
    const int LEFT_SENSOR;

    SensorManager(int front=34, int right=35, int left=32);

    void begin();

    bool wallFront();
    bool wallRight();
    bool wallLeft();
};

class MotorController {
public:
    const int IN1;
    const int IN2;
    const int IN3;
    const int IN4;

    MotorController(int in1=25,int in2=26,int in3=27,int in4=14);

    void begin();
    void stop();
    void forward();
    void turnLeft();
    void turnRight();
    void backward();
    void execute(MoveCommand cmd);
};
