#include "navigation/floodfill.h"
#include "memory/maze_memory.h"

void floodfill(uint8_t goal)
{
    for (int y = 0; y < 16; y++)
    {
        for (int x = 0; x < 16; x++)
        {
            maze[y][x] = goal;
        }
    }
}