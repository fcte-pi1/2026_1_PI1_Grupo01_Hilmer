#include "navigation_core.h"
#include <queue>

void MazeMap::begin() {
    for (int r = 0; r < MAZE_SIZE; ++r) {
        for (int c = 0; c < MAZE_SIZE; ++c) {
            cells[r][c].dist = INF;
            cells[r][c].visited = false;
            cells[r][c].northWall = false;
            cells[r][c].southWall = false;
            cells[r][c].eastWall = false;
            cells[r][c].westWall = false;
        }
    }

    for (int i = 0; i < MAZE_SIZE; ++i) {
        cells[0][i].southWall = true;
        cells[MAZE_SIZE - 1][i].northWall = true;
        cells[i][0].westWall = true;
        cells[i][MAZE_SIZE - 1].eastWall = true;
    }
}

Cell& MazeMap::get(int r, int c) {
    return cells[r][c];
}

bool MazeMap::valid(int r, int c) {
    return r >= 0 && r < MAZE_SIZE && c >= 0 && c < MAZE_SIZE;
}

FloodFillNavigator::FloodFillNavigator(MazeMap* m) : maze(m) {}

void FloodFillNavigator::compute(Position goal) {
    for (int r = 0; r < MAZE_SIZE; ++r) {
        for (int c = 0; c < MAZE_SIZE; ++c) {
            maze->get(r,c).dist = INF;
            maze->get(r,c).visited = false;
        }
    }

    if (!maze->valid(goal.r, goal.c)) return;

    std::queue<Position> q;
    maze->get(goal.r, goal.c).dist = 0;
    q.push(goal);

    while (!q.empty()) {
        Position p = q.front(); q.pop();
        Cell &cur = maze->get(p.r, p.c);
        int base = cur.dist;

        // norte -> r+1
        if (!cur.northWall) {
            int nr = p.r + 1;
            int nc = p.c;
            if (maze->valid(nr, nc)) {
                Cell &nbr = maze->get(nr, nc);
                if (nbr.dist == INF) {
                    nbr.dist = base + 1;
                    q.push({nr, nc});
                }
            }
        }

        // sul -> r-1
        if (!cur.southWall) {
            int nr = p.r - 1;
            int nc = p.c;
            if (maze->valid(nr, nc)) {
                Cell &nbr = maze->get(nr, nc);
                if (nbr.dist == INF) {
                    nbr.dist = base + 1;
                    q.push({nr, nc});
                }
            }
        }

        // leste -> c+1
        if (!cur.eastWall) {
            int nr = p.r;
            int nc = p.c + 1;
            if (maze->valid(nr, nc)) {
                Cell &nbr = maze->get(nr, nc);
                if (nbr.dist == INF) {
                    nbr.dist = base + 1;
                    q.push({nr, nc});
                }
            }
        }

        // oeste -> c-1
        if (!cur.westWall) {
            int nr = p.r;
            int nc = p.c - 1;
            if (maze->valid(nr, nc)) {
                Cell &nbr = maze->get(nr, nc);
                if (nbr.dist == INF) {
                    nbr.dist = base + 1;
                    q.push({nr, nc});
                }
            }
        }
    }
}

NavigationController::NavigationController(MazeMap* m) : maze(m) {}

MoveCommand NavigationController::decide(Position pos, Direction dir) {
    // relative directions: 0 forward, 1 right, 2 left, 3 back
    Position rel[4];
    rel[0] = pos; rel[1] = pos; rel[2] = pos; rel[3] = pos;

    switch (dir) {
        case NORTH:
            rel[0].r = pos.r + 1; rel[0].c = pos.c;
            rel[1].r = pos.r;     rel[1].c = pos.c + 1;
            rel[2].r = pos.r;     rel[2].c = pos.c - 1;
            rel[3].r = pos.r - 1; rel[3].c = pos.c;
            break;
        case EAST:
            rel[0].r = pos.r;     rel[0].c = pos.c + 1;
            rel[1].r = pos.r - 1; rel[1].c = pos.c;
            rel[2].r = pos.r + 1; rel[2].c = pos.c;
            rel[3].r = pos.r;     rel[3].c = pos.c - 1;
            break;
        case SOUTH:
            rel[0].r = pos.r - 1; rel[0].c = pos.c;
            rel[1].r = pos.r;     rel[1].c = pos.c - 1;
            rel[2].r = pos.r;     rel[2].c = pos.c + 1;
            rel[3].r = pos.r + 1; rel[3].c = pos.c;
            break;
        case WEST:
            rel[0].r = pos.r;     rel[0].c = pos.c - 1;
            rel[1].r = pos.r + 1; rel[1].c = pos.c;
            rel[2].r = pos.r - 1; rel[2].c = pos.c;
            rel[3].r = pos.r;     rel[3].c = pos.c + 1;
            break;
    }

    int bestDist = INF + 1;
    int bestIdx = 0;

    auto blocked = [&](int idx) -> bool {
        Position to = rel[idx];
        if (!maze->valid(to.r, to.c)) return true;
        Cell &cur = maze->get(pos.r, pos.c);
        if (to.r == pos.r + 1 && to.c == pos.c) return cur.northWall;
        if (to.r == pos.r - 1 && to.c == pos.c) return cur.southWall;
        if (to.r == pos.r && to.c == pos.c + 1) return cur.eastWall;
        if (to.r == pos.r && to.c == pos.c - 1) return cur.westWall;
        return true;
    };

    for (int i = 0; i < 4; ++i) {
        if (blocked(i)) continue;
        Cell &nbr = maze->get(rel[i].r, rel[i].c);
        if (nbr.dist < bestDist) {
            bestDist = nbr.dist;
            bestIdx = i;
        }
    }

    switch (bestIdx) {
        case 0: return MOVE_FORWARD;
        case 1: return TURN_RIGHT;
        case 2: return TURN_LEFT;
        default: return TURN_BACK;
    }
}
