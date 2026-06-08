#include "navigation/floodfill_engine.h"
#include "memory/maze_memory.h"
#include "navigation/queue.h"
#include <string.h>

#define QUEUE_MAX_FLOOD 256

typedef struct
{
    uint8_t r;
    uint8_t c;
} FloodCell;

static uint32_t in_queue[MAZE_SIZE];
static FloodCell flood_queue[QUEUE_MAX_FLOOD];
static int flood_head = 0;
static int flood_tail = 0;

void floodfill_queue_reset(void)
{
    flood_head = 0;
    flood_tail = 0;
    memset(in_queue, 0, sizeof(in_queue));
}

bool floodfill_queue_push(uint8_t r, uint8_t c)
{
    if (in_queue[r] & (1u << c))
        return false;
    if (((flood_tail + 1) & (QUEUE_MAX_FLOOD - 1)) == flood_head)
        return false;

    in_queue[r] |= (1u << c);
    flood_queue[flood_tail].r = r;
    flood_queue[flood_tail].c = c;
    flood_tail = (flood_tail + 1) & (QUEUE_MAX_FLOOD - 1);
    return true;
}

static FloodCell flood_queue_pop(void)
{
    FloodCell cell = flood_queue[flood_head];
    in_queue[cell.r] &= ~(1u << cell.c);
    flood_head = (flood_head + 1) & (QUEUE_MAX_FLOOD - 1);
    return cell;
}

static bool flood_queue_empty(void)
{
    return flood_head == flood_tail;
}

void floodfill_init(void)
{
    maze_memory_init();
}

bool floodfill_is_target(int r, int c, char target)
{
    if (target == 'S')
        return (r == 0 && c == 0);
    if (target == 'C')
    {
        int mid = MAZE_SIZE / 2;
        return ((r == mid - 1 || r == mid) && (c == mid - 1 || c == mid));
    }
    return false;
}

int floodfill_get_min_neighbor(int r, int c)
{
    int min_val = 255;
    if (!horiz_walls[r + 1][c] && manhattan_dist[r + 1][c] < min_val)
        min_val = manhattan_dist[r + 1][c];
    if (!horiz_walls[r][c] && manhattan_dist[r - 1][c] < min_val)
        min_val = manhattan_dist[r - 1][c];
    if (!vert_walls[r][c + 1] && manhattan_dist[r][c + 1] < min_val)
        min_val = manhattan_dist[r][c + 1];
    if (!vert_walls[r][c] && manhattan_dist[r][c - 1] < min_val)
        min_val = manhattan_dist[r][c - 1];
    return min_val;
}

void floodfill_complete(char target)
{
    memset(manhattan_dist, BLANK, sizeof(manhattan_dist));
    floodfill_queue_reset();

    if (target == 'C')
    {
        int mid = MAZE_SIZE / 2;
        manhattan_dist[mid - 1][mid - 1] = 0;
        floodfill_queue_push((uint8_t)(mid - 1), (uint8_t)(mid - 1));
        manhattan_dist[mid - 1][mid] = 0;
        floodfill_queue_push((uint8_t)(mid - 1), (uint8_t)mid);
        manhattan_dist[mid][mid - 1] = 0;
        floodfill_queue_push((uint8_t)mid, (uint8_t)(mid - 1));
        manhattan_dist[mid][mid] = 0;
        floodfill_queue_push((uint8_t)mid, (uint8_t)mid);
    }
    else if (target == 'S')
    {
        manhattan_dist[0][0] = 0;
        floodfill_queue_push(0, 0);
    }

    while (!flood_queue_empty())
    {
        FloodCell current = flood_queue_pop();
        int r = current.r;
        int c = current.c;
        int current_val = manhattan_dist[r][c];

        if (!horiz_walls[r + 1][c] && manhattan_dist[r + 1][c] == BLANK)
        {
            manhattan_dist[r + 1][c] = current_val + 1;
            floodfill_queue_push((uint8_t)(r + 1), (uint8_t)c);
        }
        if (!horiz_walls[r][c] && manhattan_dist[r - 1][c] == BLANK)
        {
            manhattan_dist[r - 1][c] = current_val + 1;
            floodfill_queue_push((uint8_t)(r - 1), (uint8_t)c);
        }
        if (!vert_walls[r][c + 1] && manhattan_dist[r][c + 1] == BLANK)
        {
            manhattan_dist[r][c + 1] = current_val + 1;
            floodfill_queue_push((uint8_t)r, (uint8_t)(c + 1));
        }
        if (!vert_walls[r][c] && manhattan_dist[r][c - 1] == BLANK)
        {
            manhattan_dist[r][c - 1] = current_val + 1;
            floodfill_queue_push((uint8_t)r, (uint8_t)(c - 1));
        }
    }

    for (int y = 0; y < MAZE_SIZE; y++)
        for (int x = 0; x < MAZE_SIZE; x++)
            maze[y][x] = manhattan_dist[y][x];
}

void floodfill_modified(char target)
{
    while (!flood_queue_empty())
    {
        FloodCell curr = flood_queue_pop();
        int r = curr.r;
        int c = curr.c;

        if (floodfill_is_target(r, c, target))
            continue;

        int min_neighbor = floodfill_get_min_neighbor(r, c);

        if (manhattan_dist[r][c] != min_neighbor + 1)
        {
            manhattan_dist[r][c] = (uint8_t)(min_neighbor + 1);
            maze[r][c] = manhattan_dist[r][c];

            if (!horiz_walls[r + 1][c])
                floodfill_queue_push((uint8_t)(r + 1), (uint8_t)c);
            if (!horiz_walls[r][c])
                floodfill_queue_push((uint8_t)(r - 1), (uint8_t)c);
            if (!vert_walls[r][c + 1])
                floodfill_queue_push((uint8_t)r, (uint8_t)(c + 1));
            if (!vert_walls[r][c])
                floodfill_queue_push((uint8_t)r, (uint8_t)(c - 1));
        }
    }
}
