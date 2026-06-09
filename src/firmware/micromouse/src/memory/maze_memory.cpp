#include "maze_memory.h"
#include <string.h>

uint8_t manhattan_dist[MAZE_SIZE][MAZE_SIZE];
bool horiz_walls[MAZE_SIZE + 1][MAZE_SIZE];
bool vert_walls[MAZE_SIZE][MAZE_SIZE + 1];
uint8_t maze[MAZE_SIZE][MAZE_SIZE];

void maze_memory_reset_walls(void)
{
    memset(horiz_walls, 0, sizeof(horiz_walls));
    memset(vert_walls, 0, sizeof(vert_walls));

    for (int i = 0; i < MAZE_SIZE; i++)
    {
        horiz_walls[0][i] = true;
        horiz_walls[MAZE_SIZE][i] = true;
        vert_walls[i][0] = true;
        vert_walls[i][MAZE_SIZE] = true;
    }
}

void maze_memory_init(void)
{
    memset(manhattan_dist, BLANK, sizeof(manhattan_dist));
    maze_memory_reset_walls();
    memset(maze, 0, sizeof(maze));
}
