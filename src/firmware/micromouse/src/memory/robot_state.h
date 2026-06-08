#ifndef ROBOT_STATE_H
#define ROBOT_STATE_H

#include <stdint.h>

typedef struct
{
    uint8_t x;
    uint8_t y;
    uint8_t direction;
} RobotState;

extern RobotState robot;

#endif