#ifndef SENSOR_MEMORY_H
#define SENSOR_MEMORY_H

#include <stdint.h>
#include "../sensors/rgb_sensor.h"

typedef struct
{
    uint16_t front;
    uint16_t left;
    uint16_t right;
    float battery_voltage;
    float battery_current;
    float battery_percentage;
    RGBColor rgb;
} SensorData;

extern SensorData sensors;

#endif