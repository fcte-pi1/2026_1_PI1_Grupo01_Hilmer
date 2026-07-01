#ifndef BITMASK_H
#define BITMASK_H

#include <stdint.h>

enum WallDirection
{
    NORTH = 0,
    EAST = 1,
    SOUTH = 2,
    WEST = 3
};

bool hasWall(uint8_t cell, uint8_t dir);
void setWall(uint8_t *cell, uint8_t dir);

#endif