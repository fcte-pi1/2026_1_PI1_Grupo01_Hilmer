#ifndef SENSOR_MEMORY_H
#define SENSOR_MEMORY_H

#include <stdint.h>

typedef struct
{
    uint16_t front;
    uint16_t left;
    uint16_t right;
} SensorData;

extern SensorData sensors;

#endif