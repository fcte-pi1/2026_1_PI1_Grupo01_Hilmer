#include "hal_sensors.h"
#include "battery.h"
#include "rgb_sensor.h"

#ifdef ARDUINO

#include <Arduino.h>

#define FRONT_SENSOR_PIN 34
#define LEFT_SENSOR_PIN 35
#define RIGHT_SENSOR_PIN 32
#define FILTER_SIZE 3

struct SensorParede {
    int pin;
    int readings[FILTER_SIZE];
    int index;
    bool wallDetected;
};

static SensorParede sensorFrente = {FRONT_SENSOR_PIN};
static SensorParede sensorEsq    = {LEFT_SENSOR_PIN};
static SensorParede sensorDir    = {RIGHT_SENSOR_PIN};

static void initSensor(SensorParede &sensor) {
    sensor.index = 0;
    sensor.wallDetected = false;
    pinMode(sensor.pin, INPUT_PULLUP);
    for (int i = 0; i < FILTER_SIZE; i++) {
        sensor.readings[i] = HIGH;
    }
}

static bool lerSensorFiltrado(SensorParede &sensor) {
    int value = digitalRead(sensor.pin);
    sensor.readings[sensor.index] = value;
    sensor.index = (sensor.index + 1) % FILTER_SIZE;

    int sum = 0;
    for (int i = 0; i < FILTER_SIZE; i++) {
        sum += sensor.readings[i];
    }
    float average = sum / (float)FILTER_SIZE;
    return average < 0.5; // active low (wall present)
}

void hal_sensors_init()
{
    initSensor(sensorFrente);
    initSensor(sensorEsq);
    initSensor(sensorDir);
    initBatteryMonitor();
    initRGBSensor();
}


float hal_read_front_sensor()
{
    sensorFrente.wallDetected = lerSensorFiltrado(sensorFrente);
    return sensorFrente.wallDetected ? 1.0f : 0.0f;
}

float hal_read_left_sensor()
{
    sensorEsq.wallDetected = lerSensorFiltrado(sensorEsq);
    return sensorEsq.wallDetected ? 1.0f : 0.0f;
}

float hal_read_right_sensor()
{
    sensorDir.wallDetected = lerSensorFiltrado(sensorDir);
    return sensorDir.wallDetected ? 1.0f : 0.0f;
}

#endif
