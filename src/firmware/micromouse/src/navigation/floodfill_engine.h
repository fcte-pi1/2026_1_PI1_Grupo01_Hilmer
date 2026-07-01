#ifndef FLOODFILL_ENGINE_H
#define FLOODFILL_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

/** Alvo: 'C' = centro, 'S' = início (0,0) */
void floodfill_init(void);
void floodfill_complete(char target);
void floodfill_modified(char target);
int floodfill_get_min_neighbor(int r, int c);
bool floodfill_is_target(int r, int c, char target);
bool floodfill_queue_push(uint8_t r, uint8_t c);
void floodfill_queue_reset(void);

#endif
