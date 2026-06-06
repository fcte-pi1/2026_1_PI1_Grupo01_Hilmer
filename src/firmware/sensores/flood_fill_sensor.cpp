#include <Arduino.h>
#include <stdint.h>

#define SENSOR_LEFT 34
#define SENSOR_FRONT 35
#define SENSOR_RIGHT 36
#define FILTER_SIZE 1
#define MOTOR_IN1 25
#define MOTOR_IN2 26
#define MOTOR_IN3 27
#define MOTOR_IN4 14

constexpr uint8_t MAZE_SIZE = 16;
constexpr uint8_t INF = 255;
constexpr unsigned long NAVIGATION_STEP_INTERVAL_MS = 100;
constexpr bool DEBUG_DISTANCE_GRID = false;
constexpr int TURN_TIME_90 = 220;
constexpr int TURN_TIME_180 = 450;
constexpr int MAX_PWM = 178;

enum Direction : uint8_t {
  NORTH = 0,
  EAST = 1,
  SOUTH = 2,
  WEST = 3,
};

enum MissionPhase : uint8_t {
  TO_CENTER = 0,
  TO_START = 1,
  SPEED_RUN = 2,
  FINISHED = 3,
};

enum MoveCommand : uint8_t {
  MOVE_FORWARD = 0,
  TURN_LEFT = 1,
  TURN_RIGHT = 2,
  TURN_BACK = 3,
  STOP = 4,
};

enum AbsoluteWallDirection : uint8_t {
  WALL_NORTH = 0,
  WALL_EAST = 1,
  WALL_SOUTH = 2,
  WALL_WEST = 3,
};

struct SensorParede {
  int pin;
  int readings[FILTER_SIZE];
  int index;
  bool wallDetected;
};

struct SensorWalls {
  bool front;
  bool right;
  bool left;
};

struct Position {
  int8_t r;
  int8_t c;
};

struct Cell {
  uint8_t dist;
  uint8_t northWall : 1;
  uint8_t southWall : 1;
  uint8_t eastWall : 1;
  uint8_t westWall : 1;
};

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

  inline bool empty() const {
    return head == tail;
  }

  inline void push(Position p) {
    data[tail++] = p;
  }

  inline Position pop() {
    return data[head++];
  }
};

class MazeMap {
private:
  Cell cells[MAZE_SIZE][MAZE_SIZE];

public:
  void begin() {
    for (uint8_t r = 0; r < MAZE_SIZE; r++) {
      for (uint8_t c = 0; c < MAZE_SIZE; c++) {
        cells[r][c].dist = INF;
        cells[r][c].northWall = 0;
        cells[r][c].southWall = 0;
        cells[r][c].eastWall = 0;
        cells[r][c].westWall = 0;
      }
    }

    for (uint8_t i = 0; i < MAZE_SIZE; i++) {
      cells[0][i].southWall = 1;
      cells[MAZE_SIZE - 1][i].northWall = 1;
      cells[i][0].westWall = 1;
      cells[i][MAZE_SIZE - 1].eastWall = 1;
    }
  }

  inline bool valid(int8_t r, int8_t c) const {
    return r >= 0 && r < MAZE_SIZE && c >= 0 && c < MAZE_SIZE;
  }

  inline Cell& get(uint8_t r, uint8_t c) {
    return cells[r][c];
  }

  inline const Cell& get(uint8_t r, uint8_t c) const {
    return cells[r][c];
  }
};

class FloodFillNavigator {
private:
  MazeMap* maze;
  PositionQueue queue;

public:
  explicit FloodFillNavigator(MazeMap* mazeMap)
      : maze(mazeMap) {}

  void computeCenter() {
    resetDistances();
    queue.clear();

    seedGoal({7, 7});
    seedGoal({7, 8});
    seedGoal({8, 7});
    seedGoal({8, 8});

    propagate();
  }

  void computeStart() {
    resetDistances();
    queue.clear();

    seedGoal({0, 0});
    propagate();
  }

private:
  inline void resetDistances() {
    for (uint8_t r = 0; r < MAZE_SIZE; r++) {
      for (uint8_t c = 0; c < MAZE_SIZE; c++) {
        maze->get(r, c).dist = INF;
      }
    }
  }

  inline void seedGoal(Position goal) {
    if (!maze->valid(goal.r, goal.c)) {
      return;
    }

    Cell& goalCell = maze->get(goal.r, goal.c);
    if (goalCell.dist == 0) {
      return;
    }

    goalCell.dist = 0;
    queue.push(goal);
  }

  void propagate() {
    while (!queue.empty()) {
      Position p = queue.pop();
      Cell& cur = maze->get(p.r, p.c);
      uint8_t base = cur.dist;

      processNeighbor(p.r + 1, p.c, cur.northWall, base);
      processNeighbor(p.r - 1, p.c, cur.southWall, base);
      processNeighbor(p.r, p.c + 1, cur.eastWall, base);
      processNeighbor(p.r, p.c - 1, cur.westWall, base);
    }
  }

  inline void processNeighbor(int8_t nr, int8_t nc, bool wall, uint8_t base) {
    if (wall || !maze->valid(nr, nc)) {
      return;
    }

    Cell& nbr = maze->get(nr, nc);
    if (nbr.dist != INF) {
      return;
    }

    nbr.dist = base + 1;
    queue.push({nr, nc});
  }
};

SensorParede sensorEsq = {SENSOR_LEFT};
SensorParede sensorFrente = {SENSOR_FRONT};
SensorParede sensorDir = {SENSOR_RIGHT};

MazeMap maze;
FloodFillNavigator floodFill(&maze);

Position currentPos = {0, 0};
Direction currentDir = NORTH;
MissionPhase currentPhase = TO_CENTER;
unsigned long lastNavigationStepMs = 0;

void initSensor(SensorParede& sensor) {
  sensor.index = 0;
  sensor.wallDetected = false;

  for (int i = 0; i < FILTER_SIZE; i++) {
    sensor.readings[i] = HIGH;
  }
}

bool lerSensorFiltrado(SensorParede& sensor) {
  int value = digitalRead(sensor.pin);

  sensor.readings[sensor.index] = value;
  sensor.index = (sensor.index + 1) % FILTER_SIZE;

  int sum = 0;
  for (int i = 0; i < FILTER_SIZE; i++) {
    sum += sensor.readings[i];
  }

  float average = sum / (float)FILTER_SIZE;
  return average < 0.5f;
}

void atualizarSensoresParede() {
  sensorEsq.wallDetected = lerSensorFiltrado(sensorEsq);
  sensorFrente.wallDetected = lerSensorFiltrado(sensorFrente);
  sensorDir.wallDetected = lerSensorFiltrado(sensorDir);
}

SensorWalls getCurrentWalls() {
  return {
      sensorFrente.wallDetected,
      sensorDir.wallDetected,
      sensorEsq.wallDetected,
  };
}

void logWalls(const SensorWalls& walls) {
  Serial.print("FRENTE=");
  Serial.print(walls.front ? "PAREDE" : "LIVRE");
  Serial.print(" DIREITA=");
  Serial.print(walls.right ? "PAREDE" : "LIVRE");
  Serial.print(" ESQUERDA=");
  Serial.println(walls.left ? "PAREDE" : "LIVRE");
}

void robotLog(const char* text) {
  Serial.println(text);
}

void beginMotors() {
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT);
  pinMode(MOTOR_IN4, OUTPUT);
}

void motorStop() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, LOW);
}

void motorForward() {
  analogWrite(MOTOR_IN1, MAX_PWM);
  digitalWrite(MOTOR_IN2, LOW);
  analogWrite(MOTOR_IN3, MAX_PWM);
  digitalWrite(MOTOR_IN4, LOW);
}

void motorTurnLeft() {
  digitalWrite(MOTOR_IN1, LOW);
  analogWrite(MOTOR_IN2, MAX_PWM);
  analogWrite(MOTOR_IN3, MAX_PWM);
  digitalWrite(MOTOR_IN4, LOW);
}

void motorTurnRight() {
  analogWrite(MOTOR_IN1, MAX_PWM);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW);
  analogWrite(MOTOR_IN4, MAX_PWM);
}

void executeMoveCommand(MoveCommand cmd) {
  switch (cmd) {
    case MOVE_FORWARD:
      motorForward();
      break;
    case TURN_LEFT:
      motorTurnLeft();
      delay(TURN_TIME_90);
      motorForward();
      break;
    case TURN_RIGHT:
      motorTurnRight();
      delay(TURN_TIME_90);
      motorForward();
      break;
    case TURN_BACK:
      motorTurnRight();
      delay(TURN_TIME_180);
      motorForward();
      break;
    case STOP:
      motorStop();
      break;
  }
}

bool markWall(Position pos, AbsoluteWallDirection dir) {
  if (!maze.valid(pos.r, pos.c)) {
    return false;
  }

  Cell& cur = maze.get(pos.r, pos.c);

  switch (dir) {
    case WALL_NORTH:
      if (cur.northWall) {
        return false;
      }
      cur.northWall = 1;
      if (maze.valid(pos.r + 1, pos.c)) {
        maze.get(pos.r + 1, pos.c).southWall = 1;
      }
      return true;

    case WALL_EAST:
      if (cur.eastWall) {
        return false;
      }
      cur.eastWall = 1;
      if (maze.valid(pos.r, pos.c + 1)) {
        maze.get(pos.r, pos.c + 1).westWall = 1;
      }
      return true;

    case WALL_SOUTH:
      if (cur.southWall) {
        return false;
      }
      cur.southWall = 1;
      if (maze.valid(pos.r - 1, pos.c)) {
        maze.get(pos.r - 1, pos.c).northWall = 1;
      }
      return true;

    case WALL_WEST:
      if (cur.westWall) {
        return false;
      }
      cur.westWall = 1;
      if (maze.valid(pos.r, pos.c - 1)) {
        maze.get(pos.r, pos.c - 1).eastWall = 1;
      }
      return true;
  }

  return false;
}

void printDistanceGrid() {
  if (!DEBUG_DISTANCE_GRID) {
    return;
  }

  for (int r = MAZE_SIZE - 1; r >= 0; r--) {
    for (int c = 0; c < MAZE_SIZE; c++) {
      Serial.print((int)maze.get(r, c).dist);
      Serial.print('\t');
    }
    Serial.println();
  }
}

void recalculateForCurrentPhase() {
  if (currentPhase == TO_START) {
    floodFill.computeStart();
  } else {
    floodFill.computeCenter();
  }
  printDistanceGrid();
}

bool updateWallsFromSensors(const SensorWalls& walls) {
  bool mapUpdated = false;

  switch (currentDir) {
    case NORTH:
      if (walls.front) {
        mapUpdated |= markWall(currentPos, WALL_NORTH);
      }
      if (walls.right) {
        mapUpdated |= markWall(currentPos, WALL_EAST);
      }
      if (walls.left) {
        mapUpdated |= markWall(currentPos, WALL_WEST);
      }
      break;

    case EAST:
      if (walls.front) {
        mapUpdated |= markWall(currentPos, WALL_EAST);
      }
      if (walls.right) {
        mapUpdated |= markWall(currentPos, WALL_SOUTH);
      }
      if (walls.left) {
        mapUpdated |= markWall(currentPos, WALL_NORTH);
      }
      break;

    case SOUTH:
      if (walls.front) {
        mapUpdated |= markWall(currentPos, WALL_SOUTH);
      }
      if (walls.right) {
        mapUpdated |= markWall(currentPos, WALL_WEST);
      }
      if (walls.left) {
        mapUpdated |= markWall(currentPos, WALL_EAST);
      }
      break;

    case WEST:
      if (walls.front) {
        mapUpdated |= markWall(currentPos, WALL_WEST);
      }
      if (walls.right) {
        mapUpdated |= markWall(currentPos, WALL_NORTH);
      }
      if (walls.left) {
        mapUpdated |= markWall(currentPos, WALL_SOUTH);
      }
      break;
  }

  return mapUpdated;
}

bool advancePhaseIfNeeded() {
  uint8_t currentDist = maze.get(currentPos.r, currentPos.c).dist;

  if (currentPhase == TO_CENTER && currentDist == 0) {
    robotLog("Fase 0: Centro alcancado! Iniciando retorno.");
    currentPhase = TO_START;
    recalculateForCurrentPhase();
    return true;
  }

  if (currentPhase == TO_START && currentPos.r == 0 && currentPos.c == 0) {
    robotLog("Fase 1: Retorno concluido! Iniciando caminho otimo.");
    currentPhase = SPEED_RUN;
    recalculateForCurrentPhase();
    return true;
  }

  if (currentPhase == SPEED_RUN && currentDist == 0) {
    robotLog("Fase 2: Vitoria! Caminho otimo concluido.");
    currentPhase = FINISHED;
    return true;
  }

  return false;
}

bool blocked(Position from, Position to) {
  if (!maze.valid(to.r, to.c)) {
    return true;
  }

  const Cell& cur = maze.get(from.r, from.c);

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

MoveCommand chooseNextMove() {
  Position rel[4];

  switch (currentDir) {
    case NORTH:
      rel[0] = {static_cast<int8_t>(currentPos.r + 1), currentPos.c};
      rel[1] = {currentPos.r, static_cast<int8_t>(currentPos.c + 1)};
      rel[2] = {currentPos.r, static_cast<int8_t>(currentPos.c - 1)};
      rel[3] = {static_cast<int8_t>(currentPos.r - 1), currentPos.c};
      break;

    case EAST:
      rel[0] = {currentPos.r, static_cast<int8_t>(currentPos.c + 1)};
      rel[1] = {static_cast<int8_t>(currentPos.r - 1), currentPos.c};
      rel[2] = {static_cast<int8_t>(currentPos.r + 1), currentPos.c};
      rel[3] = {currentPos.r, static_cast<int8_t>(currentPos.c - 1)};
      break;

    case SOUTH:
      rel[0] = {static_cast<int8_t>(currentPos.r - 1), currentPos.c};
      rel[1] = {currentPos.r, static_cast<int8_t>(currentPos.c - 1)};
      rel[2] = {currentPos.r, static_cast<int8_t>(currentPos.c + 1)};
      rel[3] = {static_cast<int8_t>(currentPos.r + 1), currentPos.c};
      break;

    case WEST:
      rel[0] = {currentPos.r, static_cast<int8_t>(currentPos.c - 1)};
      rel[1] = {static_cast<int8_t>(currentPos.r + 1), currentPos.c};
      rel[2] = {static_cast<int8_t>(currentPos.r - 1), currentPos.c};
      rel[3] = {currentPos.r, static_cast<int8_t>(currentPos.c + 1)};
      break;
  }

  uint8_t bestDist = INF;
  int8_t bestIdx = -1;

  for (uint8_t i = 0; i < 4; i++) {
    if (blocked(currentPos, rel[i])) {
      continue;
    }

    uint8_t dist = maze.get(rel[i].r, rel[i].c).dist;
    if (dist < bestDist) {
      bestDist = dist;
      bestIdx = i;
    }
  }

  switch (bestIdx) {
    case 0:
      return MOVE_FORWARD;
    case 1:
      return TURN_RIGHT;
    case 2:
      return TURN_LEFT;
    case 3:
      return TURN_BACK;
    default:
      return STOP;
  }
}

void updatePositionFromCommand(MoveCommand cmd) {
  if (cmd == TURN_LEFT) {
    currentDir = static_cast<Direction>((currentDir + 3) & 0x03);
  } else if (cmd == TURN_RIGHT) {
    currentDir = static_cast<Direction>((currentDir + 1) & 0x03);
  } else if (cmd == TURN_BACK) {
    currentDir = static_cast<Direction>((currentDir + 2) & 0x03);
  } else if (cmd == STOP) {
    return;
  }

  switch (currentDir) {
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

void runFloodFillStep() {
  if (currentPhase == FINISHED) {
    return;
  }

  advancePhaseIfNeeded();
  if (currentPhase == FINISHED) {
    return;
  }

  SensorWalls walls = getCurrentWalls();
  bool mapUpdated = updateWallsFromSensors(walls);
  if (mapUpdated) {
    recalculateForCurrentPhase();
  }

  MoveCommand cmd = chooseNextMove();
  if (cmd == STOP) {
    robotLog("Sem movimento valido. Robo parado.");
    executeMoveCommand(STOP);
    return;
  }

  executeMoveCommand(cmd);
  updatePositionFromCommand(cmd);
}

void setup() {
  Serial.begin(115200);

  pinMode(SENSOR_LEFT, INPUT_PULLUP);
  pinMode(SENSOR_FRONT, INPUT_PULLUP);
  pinMode(SENSOR_RIGHT, INPUT_PULLUP);
  beginMotors();
  motorStop();

  initSensor(sensorEsq);
  initSensor(sensorFrente);
  initSensor(sensorDir);

  maze.begin();
  recalculateForCurrentPhase();
  robotLog("Flood fill inicializado.");
}

void loop() {
  if (currentPhase == FINISHED) {
    delay(1000);
    return;
  }

  if (millis() - lastNavigationStepMs < NAVIGATION_STEP_INTERVAL_MS) {
    return;
  }

  lastNavigationStepMs = millis();

  atualizarSensoresParede();
  logWalls(getCurrentWalls());
  runFloodFillStep();
}
