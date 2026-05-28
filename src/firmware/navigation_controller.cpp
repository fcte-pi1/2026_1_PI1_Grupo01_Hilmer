#include <Arduino.h>
#include <stdint.h>

//GPIO CONFIGURATION

// Sensores
#define FRONT_SENSOR_PIN 34
#define RIGHT_SENSOR_PIN 35
#define LEFT_SENSOR_PIN  32

// Motores
#define MOTOR_IN1 25
#define MOTOR_IN2 26
#define MOTOR_IN3 27
#define MOTOR_IN4 14

// CONFIGURAÇÕES


#define MAZE_SIZE 16

#define INF 255

#define TURN_TIME_90   220
#define TURN_TIME_180  450

//ENUMS

enum Direction : uint8_t {
    NORTH = 0,
    EAST  = 1,
    SOUTH = 2,
    WEST  = 3
};

enum MoveCommand : uint8_t {
    MOVE_FORWARD,
    TURN_LEFT,
    TURN_RIGHT,
    TURN_BACK,
    STOP
};

enum AbsoluteWallDirection : uint8_t {
    WALL_NORTH = 0,
    WALL_EAST  = 1,
    WALL_SOUTH = 2,
    WALL_WEST  = 3
};

// STRUCTS


struct Cell {

    uint8_t dist;

    uint8_t visited : 1;

    uint8_t northWall : 1;
    uint8_t southWall : 1;
    uint8_t eastWall  : 1;
    uint8_t westWall  : 1;
};

struct Position {

    int8_t r;
    int8_t c;
};


//FILA ESTÁTICA
class PositionQueue {

private:

    Position data[MAZE_SIZE * MAZE_SIZE];

    uint16_t head = 0;

    uint16_t tail = 0;

public:

    inline void clear() {

        head = 0;

        tail = 0;
    }

    inline bool empty() {

        return head == tail;
    }

    inline void push(Position p) {

        data[tail++] = p;
    }

    inline Position pop() {

        return data[head++];
    }
};


//SENSOR MANAGER
class SensorManager {

public:

    void begin() {

        pinMode(FRONT_SENSOR_PIN, INPUT);

        pinMode(RIGHT_SENSOR_PIN, INPUT);

        pinMode(LEFT_SENSOR_PIN, INPUT);
    }

    inline bool wallFront() {

        return digitalRead(FRONT_SENSOR_PIN);
    }

    inline bool wallRight() {

        return digitalRead(RIGHT_SENSOR_PIN);
    }

    inline bool wallLeft() {

        return digitalRead(LEFT_SENSOR_PIN);
    }
};

//MOTOR CONTROLLER

class MotorController {

public:

    void begin() {

        pinMode(MOTOR_IN1, OUTPUT);
        pinMode(MOTOR_IN2, OUTPUT);

        pinMode(MOTOR_IN3, OUTPUT);
        pinMode(MOTOR_IN4, OUTPUT);

        stop();
    }

    inline void stop() {

        digitalWrite(MOTOR_IN1, LOW);
        digitalWrite(MOTOR_IN2, LOW);

        digitalWrite(MOTOR_IN3, LOW);
        digitalWrite(MOTOR_IN4, LOW);
    }

    inline void forward() {

        digitalWrite(MOTOR_IN1, HIGH);
        digitalWrite(MOTOR_IN2, LOW);

        digitalWrite(MOTOR_IN3, HIGH);
        digitalWrite(MOTOR_IN4, LOW);
    }

    inline void backward() {

        digitalWrite(MOTOR_IN1, LOW);
        digitalWrite(MOTOR_IN2, HIGH);

        digitalWrite(MOTOR_IN3, LOW);
        digitalWrite(MOTOR_IN4, HIGH);
    }

    inline void turnLeft() {

        digitalWrite(MOTOR_IN1, LOW);
        digitalWrite(MOTOR_IN2, HIGH);

        digitalWrite(MOTOR_IN3, HIGH);
        digitalWrite(MOTOR_IN4, LOW);
    }

    inline void turnRight() {

        digitalWrite(MOTOR_IN1, HIGH);
        digitalWrite(MOTOR_IN2, LOW);

        digitalWrite(MOTOR_IN3, LOW);
        digitalWrite(MOTOR_IN4, HIGH);
    }

    void execute(MoveCommand cmd) {

        switch(cmd) {

            case MOVE_FORWARD:

                forward();

                break;

            case TURN_LEFT:

                turnLeft();

                delay(TURN_TIME_90);

                forward();

                break;

            case TURN_RIGHT:

                turnRight();

                delay(TURN_TIME_90);

                forward();

                break;

            case TURN_BACK:

                turnRight();

                delay(TURN_TIME_180);

                forward();

                break;

            case STOP:

                stop();

                break;
        }
    }
};

// MAPEAMENTO
class MazeMap {

private:

    Cell cells[MAZE_SIZE][MAZE_SIZE];

public:

    void begin() {

        for (uint8_t r = 0; r < MAZE_SIZE; r++) {

            for (uint8_t c = 0; c < MAZE_SIZE; c++) {

                cells[r][c].dist = INF;

                cells[r][c].visited = 0;

                cells[r][c].northWall = 0;
                cells[r][c].southWall = 0;
                cells[r][c].eastWall  = 0;
                cells[r][c].westWall  = 0;
            }
        }

        // paredes externas

        for (uint8_t i = 0; i < MAZE_SIZE; i++) {

            cells[0][i].southWall = 1;

            cells[MAZE_SIZE - 1][i].northWall = 1;

            cells[i][0].westWall = 1;

            cells[i][MAZE_SIZE - 1].eastWall = 1;
        }
    }

    inline Cell& get(uint8_t r, uint8_t c) {

        return cells[r][c];
    }

    inline bool valid(int8_t r, int8_t c) {

        return (
            r >= 0 &&
            r < MAZE_SIZE &&
            c >= 0 &&
            c < MAZE_SIZE
        );
    }
};

// FLOOD FILL
class FloodFillNavigator {

private:

    MazeMap* maze;

    PositionQueue queue;

public:

    FloodFillNavigator(MazeMap* m)
        : maze(m) {}

    void compute(Position goal) {

        for (uint8_t r = 0; r < MAZE_SIZE; r++) {

            for (uint8_t c = 0; c < MAZE_SIZE; c++) {

                maze->get(r, c).dist = INF;
            }
        }

        if (!maze->valid(goal.r, goal.c)) {
            return;
        }

        queue.clear();

        maze->get(goal.r, goal.c).dist = 0;

        queue.push(goal);

        while (!queue.empty()) {

            Position p = queue.pop();

            Cell &cur = maze->get(p.r, p.c);

            uint8_t base = cur.dist;

            processNeighbor(
                p.r + 1,
                p.c,
                cur.northWall,
                base
            );

            processNeighbor(
                p.r - 1,
                p.c,
                cur.southWall,
                base
            );

            processNeighbor(
                p.r,
                p.c + 1,
                cur.eastWall,
                base
            );

            processNeighbor(
                p.r,
                p.c - 1,
                cur.westWall,
                base
            );
        }
    }

private:

    inline void processNeighbor(
        int8_t nr,
        int8_t nc,
        bool wall,
        uint8_t base
    ) {

        if (wall) {
            return;
        }

        if (!maze->valid(nr, nc)) {
            return;
        }

        Cell &nbr = maze->get(nr, nc);

        if (nbr.dist != INF) {
            return;
        }

        nbr.dist = base + 1;

        queue.push({nr, nc});
    }
};

// NAVIGATION CONTROLLER 
class NavigationController {

private:

    MazeMap* maze;

public:

    NavigationController(MazeMap* m)
        : maze(m) {}

    MoveCommand decide(
        Position pos,
        Direction dir
    ) {

        Position rel[4];

        switch(dir) {

            case NORTH:

                rel[0] = {pos.r + 1, pos.c};
                rel[1] = {pos.r, pos.c + 1};
                rel[2] = {pos.r, pos.c - 1};
                rel[3] = {pos.r - 1, pos.c};

                break;

            case EAST:

                rel[0] = {pos.r, pos.c + 1};
                rel[1] = {pos.r - 1, pos.c};
                rel[2] = {pos.r + 1, pos.c};
                rel[3] = {pos.r, pos.c - 1};

                break;

            case SOUTH:

                rel[0] = {pos.r - 1, pos.c};
                rel[1] = {pos.r, pos.c - 1};
                rel[2] = {pos.r, pos.c + 1};
                rel[3] = {pos.r + 1, pos.c};

                break;

            case WEST:

                rel[0] = {pos.r, pos.c - 1};
                rel[1] = {pos.r + 1, pos.c};
                rel[2] = {pos.r - 1, pos.c};
                rel[3] = {pos.r, pos.c + 1};

                break;
        }

        uint8_t bestDist = INF;

        uint8_t bestIdx = 3;

        for (uint8_t i = 0; i < 4; i++) {

            if (blocked(pos, rel[i])) {
                continue;
            }

            Cell &nbr = maze->get(
                rel[i].r,
                rel[i].c
            );

            if (nbr.dist < bestDist) {

                bestDist = nbr.dist;

                bestIdx = i;
            }
        }

        switch(bestIdx) {

            case 0:
                return MOVE_FORWARD;

            case 1:
                return TURN_RIGHT;

            case 2:
                return TURN_LEFT;

            default:
                return TURN_BACK;
        }
    }

private:

    inline bool blocked(
        Position from,
        Position to
    ) {

        if (!maze->valid(to.r, to.c)) {
            return true;
        }

        Cell &cur = maze->get(
            from.r,
            from.c
        );

        if (to.r == from.r + 1) {
            return cur.northWall;
        }

        if (to.r == from.r - 1) {
            return cur.southWall;
        }

        if (to.c == from.c + 1) {
            return cur.eastWall;
        }

        if (to.c == from.c - 1) {
            return cur.westWall;
        }

        return true;
    }
};

// OBJETOS GLOBAIS

SensorManager sensors;

MotorController motors;

MazeMap maze;

FloodFillNavigator floodFill(&maze);

NavigationController navigator(&maze);

//ESTADO DO ROBÔ
Position currentPos = {0, 0};

Direction currentDir = NORTH;

// SET WALL
void setWall(
    Position pos,
    AbsoluteWallDirection dir,
    bool wall
) {

    if (!maze.valid(pos.r, pos.c)) {
        return;
    }

    Cell &cur = maze.get(
        pos.r,
        pos.c
    );

    switch(dir) {

        // NORTH

        case WALL_NORTH:

            cur.northWall = wall;

            if (maze.valid(pos.r + 1, pos.c)) {

                maze.get(
                    pos.r + 1,
                    pos.c
                ).southWall = wall;
            }

            break;

        // SOUTH


        case WALL_SOUTH:

            cur.southWall = wall;

            if (maze.valid(pos.r - 1, pos.c)) {

                maze.get(
                    pos.r - 1,
                    pos.c
                ).northWall = wall;
            }

            break;

        // EAST


        case WALL_EAST:

            cur.eastWall = wall;

            if (maze.valid(pos.r, pos.c + 1)) {

                maze.get(
                    pos.r,
                    pos.c + 1
                ).westWall = wall;
            }

            break;

        // WEST
  

        case WALL_WEST:

            cur.westWall = wall;

            if (maze.valid(pos.r, pos.c - 1)) {

                maze.get(
                    pos.r,
                    pos.c - 1
                ).eastWall = wall;
            }

            break;
    }
}

//UPDATE WALLS
void updateWalls() {

    bool front = sensors.wallFront();

    bool right = sensors.wallRight();

    bool left = sensors.wallLeft();

    switch(currentDir) {

        case NORTH:

            setWall(
                currentPos,
                WALL_NORTH,
                front
            );

            setWall(
                currentPos,
                WALL_EAST,
                right
            );

            setWall(
                currentPos,
                WALL_WEST,
                left
            );

            break;

        case EAST:

            setWall(
                currentPos,
                WALL_EAST,
                front
            );

            setWall(
                currentPos,
                WALL_SOUTH,
                right
            );

            setWall(
                currentPos,
                WALL_NORTH,
                left
            );

            break;

        case SOUTH:

            setWall(
                currentPos,
                WALL_SOUTH,
                front
            );

            setWall(
                currentPos,
                WALL_WEST,
                right
            );

            setWall(
                currentPos,
                WALL_EAST,
                left
            );

            break;

        case WEST:

            setWall(
                currentPos,
                WALL_WEST,
                front
            );

            setWall(
                currentPos,
                WALL_NORTH,
                right
            );

            setWall(
                currentPos,
                WALL_SOUTH,
                left
            );

            break;
    }
}

//UPDATE POSITION
void updatePosition(
    MoveCommand cmd
) {

    switch(cmd) {

        case TURN_LEFT:

            currentDir = (Direction)(
                (currentDir + 3) & 0x03
            );

            break;

        case TURN_RIGHT:

            currentDir = (Direction)(
                (currentDir + 1) & 0x03
            );

            break;

        case TURN_BACK:

            currentDir = (Direction)(
                (currentDir + 2) & 0x03
            );

            break;

        default:
            break;
    }

    if (cmd == STOP) {
        return;
    }

    switch(currentDir) {

        case NORTH:

            currentPos.r++;

            break;

        case EAST:

            currentPos.c++;

            break;

        case SOUTH:

            currentPos.r--;

            break;

        case WEST:

            currentPos.c--;

            break;
    }
}

// SETUP
void setup() {

    Serial.begin(115200);

    sensors.begin();

    motors.begin();

    maze.begin();

    // objetivo = centro

    floodFill.compute({7, 7});

    Serial.println(
        "Micromouse iniciado"
    );
}

//LOOP
void loop() {

    // atualiza paredes
    updateWalls();

    // recalcula flood fill
    floodFill.compute({7, 7});

    // decide movimento
    MoveCommand cmd = navigator.decide(
        currentPos,
        currentDir
    );

    // executa movimento
    motors.execute(cmd);

    // atualiza posição lógica
    updatePosition(cmd);

    delay(100);
}