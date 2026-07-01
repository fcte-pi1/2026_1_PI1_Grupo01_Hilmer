#include "navigation/floodfill.h"
#include "navigation/floodfill_engine.h"
#include "config/config.h"
#include "memory/maze_memory.h"

void floodfill(uint8_t goal)
{
    floodfill_init();
    floodfill_complete('C');

    if (goal < MAZE_SIZE)
    {
        for (int y = 0; y < MAZE_SIZE; y++)
            for (int x = 0; x < MAZE_SIZE; x++)
                if (manhattan_dist[y][x] == goal)
                    return;
    }
}
