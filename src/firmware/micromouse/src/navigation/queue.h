#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>

#define QUEUE_MAX 256

typedef struct
{
    uint8_t x;
    uint8_t y;
} QueueNode;

extern QueueNode queue[QUEUE_MAX];

extern uint16_t head;
extern uint16_t tail;

void push(uint8_t x, uint8_t y);

QueueNode pop();

#endif