#include <iostream>
#include <string>
#include <stdint.h>

#define WALL_N (1 << 0)
#define WALL_S (1 << 1)
#define WALL_E (1 << 2)
#define WALL_W (1 << 3)

const int MAZE_SIZE = 16;
const uint8_t BLANK = 255;

struct Coordinates { uint8_t r; uint8_t c; };

struct Cell {
    uint8_t walls;
    uint8_t dist;
};

Cell maze[MAZE_SIZE][MAZE_SIZE];

Coordinates fila[512];
int head = 0;
int tail = 0;

void push(Coordinates cell) {
    fila[tail] = cell;
    tail++;
    if (tail == 512) tail = 0;
}

Coordinates pop() {
    Coordinates cell = fila[head];
    head++;
    if (head == 512) head = 0;
    return cell;
}

bool isEmpty() { return head == tail; }

inline bool hasWall(int r, int c, uint8_t dir) {
    return maze[r][c].walls & dir;
}

inline void setWall(int r, int c, uint8_t dir) {
    maze[r][c].walls |= dir;
}

// API DO SIMULADOR MMS

void API_log(const std::string& text) { std::cerr << text << std::endl; }
bool API_wallFront() { std::cout << "wallFront" << std::endl; std::string response; std::cin >> response; return response == "true"; }
bool API_wallRight() { std::cout << "wallRight" << std::endl; std::string response; std::cin >> response; return response == "true"; }
bool API_wallLeft() { std::cout << "wallLeft" << std::endl; std::string response; std::cin >> response; return response == "true"; }
void API_moveForward() { std::cout << "moveForward" << std::endl; std::string response; std::cin >> response; }
void API_turnRight() { std::cout << "turnRight" << std::endl; std::string response; std::cin >> response; }
void API_turnLeft() { std::cout << "turnLeft" << std::endl; std::string response; std::cin >> response; }
void API_setText(int x, int y, const std::string& text) { std::cout << "setText " << x << " " << y << " " << text << std::endl; }

void inicializeMaze() {
    for (int r = 0; r < MAZE_SIZE; r++) {
        for (int c = 0; c < MAZE_SIZE; c++) {
            maze[r][c].dist = BLANK;
            maze[r][c].walls = 0;
        }
    }

    for (int i = 0; i < MAZE_SIZE; i++) {
        maze[0][i].walls |= WALL_S;
        maze[MAZE_SIZE-1][i].walls |= WALL_N;
        maze[i][0].walls |= WALL_W;
        maze[i][MAZE_SIZE-1].walls |= WALL_E;
    }
}

void floodfill(char target) {
    for(int r = 0; r < MAZE_SIZE; r++) {
        for(int c = 0; c < MAZE_SIZE; c++) {
            maze[r][c].dist = BLANK;
        }
    }

    head = 0; 
    tail = 0;

    if (target == 'C') {
        maze[7][7].dist = 0; push({7,7});
        maze[7][8].dist = 0; push({7,8});
        maze[8][7].dist = 0; push({8,7});
        maze[8][8].dist = 0; push({8,8});
    } else {
        maze[0][0].dist = 0; push({0,0});
    }

    while (!isEmpty()) {
        Coordinates current = pop();
        int r = current.r;
        int c = current.c;
        uint8_t current_val = maze[r][c].dist;

        if (r < MAZE_SIZE-1 && !hasWall(r,c,WALL_N) && maze[r+1][c].dist == BLANK) {
            maze[r+1][c].dist = current_val + 1;
            push({(uint8_t)(r+1), c});
        }

        if (r > 0 && !hasWall(r,c,WALL_S) && maze[r-1][c].dist == BLANK) {
            maze[r-1][c].dist = current_val + 1;
            push({(uint8_t)(r-1), c});
        }

        if (c < MAZE_SIZE-1 && !hasWall(r,c,WALL_E) && maze[r][c+1].dist == BLANK) {
            maze[r][c+1].dist = current_val + 1;
            push({r, (uint8_t)(c+1)});
        }

        if (c > 0 && !hasWall(r,c,WALL_W) && maze[r][c-1].dist == BLANK) {
            maze[r][c-1].dist = current_val + 1;
            push({r, (uint8_t)(c-1)});
        }
    }
}

void desenharDistanciasNoSimulador() {
    for (int r = 0; r < MAZE_SIZE; r++) {
        for (int c = 0; c < MAZE_SIZE; c++) {
            API_setText(c, r, std::to_string(maze[r][c].dist));
        }
    }
}

int main() {
    inicializeMaze();
    floodfill('C');
    desenharDistanciasNoSimulador();

    int r = 0, c = 0, dir = 0;
    int fase = 0;

    while (true) {

        if (fase == 0 && maze[r][c].dist == 0) {
            fase = 1;
            floodfill('S');
        }
        else if (fase == 1 && r == 0 && c == 0) {
            fase = 2;
            floodfill('C');
        }
        else if (fase == 2 && maze[r][c].dist == 0) {
            break;
        }

        bool wF = API_wallFront();
        bool wR = API_wallRight();
        bool wL = API_wallLeft();
        bool mapUpdated = false;

        // Atualização com proteção de limites
        if (dir == 0) {
            if (wF && !hasWall(r,c,WALL_N)) { 
                setWall(r,c,WALL_N); 
                if (r < MAZE_SIZE-1) setWall(r+1,c,WALL_S);
                mapUpdated = true;
            }
        }

        if (mapUpdated) {
            (fase == 1) ? floodfill('S') : floodfill('C');
        }

        uint8_t min_dist = 255;
        int proxima_direcao = dir;

        if (r < MAZE_SIZE-1 && !hasWall(r,c,WALL_N) && maze[r+1][c].dist < min_dist) {
            min_dist = maze[r+1][c].dist;
            proxima_direcao = 0;
        }

        if (c < MAZE_SIZE-1 && !hasWall(r,c,WALL_E) && maze[r][c+1].dist < min_dist) {
            min_dist = maze[r][c+1].dist;
            proxima_direcao = 1;
        }

        if (r > 0 && !hasWall(r,c,WALL_S) && maze[r-1][c].dist < min_dist) {
            min_dist = maze[r-1][c].dist;
            proxima_direcao = 2;
        }

        if (c > 0 && !hasWall(r,c,WALL_W) && maze[r][c-1].dist < min_dist) {
            min_dist = maze[r][c-1].dist;
            proxima_direcao = 3;
        }

        if (proxima_direcao == (dir + 1) % 4) API_turnRight();
        else if (proxima_direcao == (dir + 3) % 4) API_turnLeft();
        else if (proxima_direcao != dir) { API_turnRight(); API_turnRight(); }

        dir = proxima_direcao;
        API_moveForward();

        if (dir == 0) r++;
        else if (dir == 1) c++;
        else if (dir == 2) r--;
        else if (dir == 3) c--;
    }

    return 0;
}