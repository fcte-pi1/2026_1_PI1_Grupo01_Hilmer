#include "bitmask.h"

bool hasWall(uint8_t cell, uint8_t dir)
{
    return (cell & (1 << dir)) != 0;
}

void setWall(uint8_t *cell, uint8_t dir)
{
    *cell |= (1 << dir);
}
