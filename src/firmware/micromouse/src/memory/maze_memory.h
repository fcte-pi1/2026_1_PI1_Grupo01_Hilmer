#ifndef MAZE_MEMORY_H
#define MAZE_MEMORY_H

#include <stdint.h>
#include <stdbool.h>
#include "config/config.h"

#define BLANK 255

extern uint8_t manhattan_dist[MAZE_SIZE][MAZE_SIZE];
extern bool horiz_walls[MAZE_SIZE + 1][MAZE_SIZE];
extern bool vert_walls[MAZE_SIZE][MAZE_SIZE + 1];

/** Compatibilidade com testes legados */
extern uint8_t maze[MAZE_SIZE][MAZE_SIZE];

void maze_memory_init(void);
void maze_memory_reset_walls(void);

#endif
