#include "navigation/queue.h"

QueueNode queue[QUEUE_MAX];

uint16_t head = 0;
uint16_t tail = 0;

void push(uint8_t x, uint8_t y)
{
    queue[tail].x = x;
    queue[tail].y = y;

    tail = (tail + 1) % QUEUE_MAX;
}

QueueNode pop()
{
    QueueNode node = queue[head];

    head = (head + 1) % QUEUE_MAX;

    return node;
}