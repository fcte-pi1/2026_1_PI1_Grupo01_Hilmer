#include <Arduino.h>

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

// STRUCTS
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


// MAZE MAP
class MazeMap {
private:
    Cell maze[MAZE_SIZE][MAZE_SIZE];

public:

    void begin() {

        for(int r = 0; r < MAZE_SIZE; r++) {
            for(int c = 0; c < MAZE_SIZE; c++) {

                maze[r][c].dist = INF;
                maze[r][c].visited = false;

                maze[r][c].northWall = false;
                maze[r][c].southWall = false;
                maze[r][c].eastWall  = false;
                maze[r][c].westWall  = false;
            }
        }

        // bordas externas
        for(int i = 0; i < MAZE_SIZE; i++) {

            maze[0][i].southWall = true;
            maze[MAZE_SIZE - 1][i].northWall = true;

            maze[i][0].westWall = true;
            maze[i][MAZE_SIZE - 1].eastWall = true;
        }
    }

    Cell& get(int r, int c) {
        return maze[r][c];
    }

    bool valid(int r, int c) {
        return r >= 0 && r < MAZE_SIZE &&
               c >= 0 && c < MAZE_SIZE;
    }
};

// SENSOR MANAGER
class SensorManager {

public:

    // Substituir pelos GPIOs reais
    const int FRONT_SENSOR = 34;
    const int RIGHT_SENSOR = 35;
    const int LEFT_SENSOR  = 32;

    void begin() {
        // define os pinos que receberão (INPUT) o sinal de cada sensor 
        pinMode(FRONT_SENSOR, INPUT); 
        pinMode(RIGHT_SENSOR, INPUT);
        pinMode(LEFT_SENSOR, INPUT);
    }

    // funções para ler o sinal digital de cada sensor (HIGH/LOW)
    bool wallFront() {
        return digitalRead(FRONT_SENSOR); 
    }

    bool wallRight() {
        return digitalRead(RIGHT_SENSOR);
    }

    bool wallLeft() {
        return digitalRead(LEFT_SENSOR);
    }
};

// MOTOR CONTROLLER
class MotorController {

private: // Ponte H L298N, controlador dos motores
    const int IN1 = 25;
    const int IN2 = 26;
    const int IN3 = 27;
    const int IN4 = 14;

public:

    void begin() {
        // define os pinos de saída para o comando dos motores
        pinMode(IN1, OUTPUT);
        pinMode(IN2, OUTPUT);
        pinMode(IN3, OUTPUT);
        pinMode(IN4, OUTPUT);
    }

    void stop() {
        //envia sinal low para todos os pinos, desligando os motores
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
    }

    void forward() {
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);

        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
    }

    void turnLeft() {

        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);

        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
    }

    void turnRight() {

        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);

        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
    }

    void backward() {

        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);

        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
    }

    void execute(MoveCommand cmd) {

        switch(cmd) {

            case MOVE_FORWARD:
                forward();
                break;

            case TURN_LEFT:
                turnLeft();
                delay(220);
                forward();
                break;

            case TURN_RIGHT:
                turnRight();
                delay(220);
                forward();
                break;

            case TURN_BACK:
                turnRight();
                delay(450);
                forward();
                break;

            case STOP:
                stop();
                break;
        }
    }
};

class FloodFillNavigator {

private:

    MazeMap* map;

public:

    FloodFillNavigator(MazeMap* mazeMap) {
        map = mazeMap;
    }

    void compute(Position goal) {

        for(int r = 0; r < MAZE_SIZE; r++) {
            for(int c = 0; c < MAZE_SIZE; c++) {
                map->get(r, c).dist = INF;
            }
        }

        Position queue[256];

        int head = 0;
        int tail = 0;

        queue[tail++] = goal;

        map->get(goal.r, goal.c).dist = 0;

        while(head != tail) {

            Position cur = queue[head++];

            int currentDist =
                map->get(cur.r, cur.c).dist;

            // norte

            if(!map->get(cur.r, cur.c).northWall &&
               map->valid(cur.r + 1, cur.c)) {

                if(map->get(cur.r + 1, cur.c).dist == INF) {

                    map->get(cur.r + 1, cur.c).dist =
                        currentDist + 1;

                    queue[tail++] =
                        {cur.r + 1, cur.c};
                }
            }

            // sul

            if(!map->get(cur.r, cur.c).southWall &&
               map->valid(cur.r - 1, cur.c)) {

                if(map->get(cur.r - 1, cur.c).dist == INF) {

                    map->get(cur.r - 1, cur.c).dist =
                        currentDist + 1;

                    queue[tail++] =
                        {cur.r - 1, cur.c};
                }
            }

            // leste

            if(!map->get(cur.r, cur.c).eastWall &&
               map->valid(cur.r, cur.c + 1)) {

                if(map->get(cur.r, cur.c + 1).dist == INF) {

                    map->get(cur.r, cur.c + 1).dist =
                        currentDist + 1;

                    queue[tail++] =
                        {cur.r, cur.c + 1};
                }
            }

            // oeste

            if(!map->get(cur.r, cur.c).westWall &&
               map->valid(cur.r, cur.c - 1)) {

                if(map->get(cur.r, cur.c - 1).dist == INF) {

                    map->get(cur.r, cur.c - 1).dist =
                        currentDist + 1;

                    queue[tail++] =
                        {cur.r, cur.c - 1};
                }
            }
        }
    }
};

// NAVIGATION CONTROLLER
class NavigationController {

private:

    MazeMap* map;

public:

    NavigationController(MazeMap* m) {
        map = m;
    }

    MoveCommand decide(Position pos, Direction dir) {

        uint8_t best = INF;
        Direction bestDir = dir;

        // norte

        if(!map->get(pos.r, pos.c).northWall) {

            uint8_t d =
                map->get(pos.r + 1, pos.c).dist;

            if(d < best) {
                best = d;
                bestDir = NORTH;
            }
        }

        // leste

        if(!map->get(pos.r, pos.c).eastWall) {

            uint8_t d =
                map->get(pos.r, pos.c + 1).dist;

            if(d < best) {
                best = d;
                bestDir = EAST;
            }
        }

        // sul

        if(!map->get(pos.r, pos.c).southWall) {

            uint8_t d =
                map->get(pos.r - 1, pos.c).dist;

            if(d < best) {
                best = d;
                bestDir = SOUTH;
            }
        }

        // oeste

        if(!map->get(pos.r, pos.c).westWall) {

            uint8_t d =
                map->get(pos.r, pos.c - 1).dist;

            if(d < best) {
                best = d;
                bestDir = WEST;
            }
        }

        // converter direção em comando

        if(bestDir == dir)
            return MOVE_FORWARD;

        if(bestDir == (dir + 1) % 4)
            return TURN_RIGHT;

        if(bestDir == (dir + 3) % 4)
            return TURN_LEFT;

        return TURN_BACK;
    }
};


// OBJETOS GLOBAIS

MazeMap maze;
SensorManager sensors;
MotorController motors;

FloodFillNavigator navigator(&maze);
NavigationController navController(&maze);

Position robot = {0, 0};

Direction robotDir = NORTH;

// UPDATE WALLS
void updateWalls() {

    bool front = sensors.wallFront();
    bool right = sensors.wallRight();
    bool left  = sensors.wallLeft();

    Cell& cell = maze.get(robot.r, robot.c);

    switch(robotDir) {

        case NORTH:
            cell.northWall = front;
            cell.eastWall  = right;
            cell.westWall  = left;
            break;

        case EAST:
            cell.eastWall  = front;
            cell.southWall = right;
            cell.northWall = left;
            break;

        case SOUTH:
            cell.southWall = front;
            cell.westWall  = right;
            cell.eastWall  = left;
            break;

        case WEST:
            cell.westWall  = front;
            cell.northWall = right;
            cell.southWall = left;
            break;
    }
}


// SETUP
void setup() {

    Serial.begin(115200);

    maze.begin();
    sensors.begin();
    motors.begin();

    navigator.compute({7, 7});
}

// LOOP PRINCIPAL
void loop() {
    // CA-24.1
    // gera comandos a cada ciclo

    updateWalls();

    // CA-24.2 e CA-24.4
    // recalcula rota usando estado atual

    navigator.compute({7, 7});

    // CA-24.3
    // evita colisão usando sensores


    MoveCommand cmd =
        navController.decide(robot, robotDir);

    // CA-24.5
    // envio imediato do comando

    motors.execute(cmd);

    delay(50);
}

