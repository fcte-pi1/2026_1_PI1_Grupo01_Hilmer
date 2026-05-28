#pragma once

#include <cstdint>

#define MAZE_SIZE 16
#define INF 255

enum Direction {
    NORTH = 0,
    EAST  = 1,
    SOUTH = 2,
    WEST  = 3
};

enum MoveCommand {
    MOVE_FORWARD,
    TURN_LEFT,
    TURN_RIGHT,
    TURN_BACK,
    STOP
};

struct Cell {
    uint8_t dist;
    bool visited;

    bool northWall;
    bool southWall;
    bool eastWall;
    bool westWall;
};

struct Position {
    int r;
    int c;
};

class MazeMap {
private:
    Cell cells[MAZE_SIZE][MAZE_SIZE];

public:
    void begin();

    Cell& get(int r, int c);

    bool valid(int r, int c);
};

class FloodFillNavigator {
private:
    MazeMap* maze;

public:
    FloodFillNavigator(MazeMap* m);
    void compute(Position goal);
};

class NavigationController {
private:
    MazeMap* maze;

public:
    NavigationController(MazeMap* m);
    MoveCommand decide(Position pos, Direction dir);
};
