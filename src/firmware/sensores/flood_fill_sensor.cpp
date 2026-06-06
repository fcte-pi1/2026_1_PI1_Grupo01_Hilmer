#include <Arduino.h>

#define SENSOR_LEFT 34
#define SENSOR_FRONT 35
#define SENSOR_RIGHT 36
#define FILTER_SIZE 1
#define MOTOR_IN1 25
#define MOTOR_IN2 26
#define MOTOR_IN3 27
#define MOTOR_IN4 14

const int MAZE_SIZE = 16;
const int BLANK = 255;
const unsigned long NAVIGATION_STEP_INTERVAL_MS = 100;
const bool DEBUG_DISTANCE_GRID = false;
const int TURN_TIME_90 = 220;
const int TURN_TIME_180 = 450;
const int MAX_PWM = 178;

enum Direction {
  NORTH = 0,
  EAST = 1,
  SOUTH = 2,
  WEST = 3,
};

enum MissionPhase {
  TO_CENTER = 0,
  TO_START = 1,
  SPEED_RUN = 2,
  FINISHED = 3,
};

enum MoveCommand {
  MOVE_FORWARD = 0,
  TURN_LEFT = 1,
  TURN_RIGHT = 2,
  TURN_BACK = 3,
  STOP = 4,
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

struct Coordinates {
  int r;
  int c;
};

SensorParede sensorEsq = {SENSOR_LEFT};
SensorParede sensorFrente = {SENSOR_FRONT};
SensorParede sensorDir = {SENSOR_RIGHT};

int manhattan_dist[MAZE_SIZE][MAZE_SIZE];
bool horiz_walls[MAZE_SIZE + 1][MAZE_SIZE];
bool vert_walls[MAZE_SIZE][MAZE_SIZE + 1];

Coordinates fila[512];
int head = 0;
int tail = 0;

int currentRow = 0;
int currentCol = 0;
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
  SensorWalls walls = {
      sensorFrente.wallDetected,
      sensorDir.wallDetected,
      sensorEsq.wallDetected,
  };
  return walls;
}

void logWalls(const SensorWalls& walls) {
  Serial.print("FRENTE=");
  Serial.print(walls.front ? "PAREDE" : "LIVRE");
  Serial.print(" DIREITA=");
  Serial.print(walls.right ? "PAREDE" : "LIVRE");
  Serial.print(" ESQUERDA=");
  Serial.println(walls.left ? "PAREDE" : "LIVRE");
}

void push(Coordinates cell) {
  fila[tail] = cell;
  tail = (tail + 1) % 512;
}

Coordinates pop() {
  Coordinates cell = fila[head];
  head = (head + 1) % 512;
  return cell;
}

bool isEmpty() {
  return head == tail;
}

bool canMoveNorth(int row, int col) {
  return row + 1 < MAZE_SIZE && !horiz_walls[row + 1][col];
}

bool canMoveEast(int row, int col) {
  return col + 1 < MAZE_SIZE && !vert_walls[row][col + 1];
}

bool canMoveSouth(int row, int col) {
  return row - 1 >= 0 && !horiz_walls[row][col];
}

bool canMoveWest(int row, int col) {
  return col - 1 >= 0 && !vert_walls[row][col];
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
      break; }
}

void inicializeMaze() {
    for (int r = 0; r < MAZE_SIZE; r++) {
        for (int c = 0; c < MAZE_SIZE; c++) {
            manhattan_dist[r][c] = BLANK; horiz_walls[r][c] = false; vert_walls[r][c] = false;
        }
    }
    for (int i = 0; i < MAZE_SIZE; i++) {
        horiz_walls[0][i] = true; horiz_walls[MAZE_SIZE][i] = true;
        vert_walls[i][0] = true; vert_walls[i][MAZE_SIZE] = true;
    }
}

void floodfill(char target) {
  for (int r = 0; r < MAZE_SIZE; r++) {
    for (int c = 0; c < MAZE_SIZE; c++) {
      manhattan_dist[r][c] = BLANK;
    }
  }
  head = 0; tail = 0;

  if (target == 'C') {
    manhattan_dist[7][7] = 0; push({7, 7});
    manhattan_dist[7][8] = 0; push({7, 8});
    manhattan_dist[8][7] = 0; push({8, 7});
    manhattan_dist[8][8] = 0; push({8, 8});
  } else if (target == 'S') {
    manhattan_dist[0][0] = 0; push({0, 0});
  }

  while (!isEmpty()) {
    Coordinates current = pop();
    int r = current.r; int c = current.c; int currentVal = manhattan_dist[r][c];

    if (canMoveNorth(r, c) && manhattan_dist[r + 1][c] == BLANK) { manhattan_dist[r + 1][c] = currentVal + 1; push({r + 1, c}); }
    if (canMoveSouth(r, c) && manhattan_dist[r - 1][c] == BLANK) { manhattan_dist[r - 1][c] = currentVal + 1; push({r - 1, c}); }
    if (canMoveEast(r, c) && manhattan_dist[r][c + 1] == BLANK) { manhattan_dist[r][c + 1] = currentVal + 1; push({r, c + 1}); }
    if (canMoveWest(r, c) && manhattan_dist[r][c - 1] == BLANK) { manhattan_dist[r][c - 1] = currentVal + 1; push({r, c - 1}); }
  }

}

void printDistanceGrid() {
  if (!DEBUG_DISTANCE_GRID) {
    return;
  }

  for (int r = MAZE_SIZE - 1; r >= 0; r--) {
    for (int c = 0; c < MAZE_SIZE; c++) {
      Serial.print(manhattan_dist[r][c]);
      Serial.print('\t');
    }
    Serial.println();
  }
}

void recalculateForCurrentPhase() {
  if (currentPhase == TO_START) {
    floodfill('S');
  } else {
    floodfill('C');
  }
  printDistanceGrid();
}

bool updateWallsFromSensors(const SensorWalls& walls) {
  bool wF = walls.front;
  bool wR = walls.right;
  bool wL = walls.left;
  bool mapUpdated = false;

  if (currentDir == NORTH) {
    if (wF && !horiz_walls[currentRow + 1][currentCol]) { horiz_walls[currentRow + 1][currentCol] = true; mapUpdated = true; }
    if (wR && !vert_walls[currentRow][currentCol + 1]) { vert_walls[currentRow][currentCol + 1] = true; mapUpdated = true; }
    if (wL && !vert_walls[currentRow][currentCol]) { vert_walls[currentRow][currentCol] = true; mapUpdated = true; }
  } else if (currentDir == EAST) {
    if (wF && !vert_walls[currentRow][currentCol + 1]) { vert_walls[currentRow][currentCol + 1] = true; mapUpdated = true; }
    if (wR && !horiz_walls[currentRow][currentCol]) { horiz_walls[currentRow][currentCol] = true; mapUpdated = true; }
    if (wL && !horiz_walls[currentRow + 1][currentCol]) { horiz_walls[currentRow + 1][currentCol] = true; mapUpdated = true; }
  } else if (currentDir == SOUTH) {
    if (wF && !horiz_walls[currentRow][currentCol]) { horiz_walls[currentRow][currentCol] = true; mapUpdated = true; }
    if (wR && !vert_walls[currentRow][currentCol]) { vert_walls[currentRow][currentCol] = true; mapUpdated = true; }
    if (wL && !vert_walls[currentRow][currentCol + 1]) { vert_walls[currentRow][currentCol + 1] = true; mapUpdated = true; }
  } else if (currentDir == WEST) {
    if (wF && !vert_walls[currentRow][currentCol]) { vert_walls[currentRow][currentCol] = true; mapUpdated = true; }
    if (wR && !horiz_walls[currentRow + 1][currentCol]) { horiz_walls[currentRow + 1][currentCol] = true; mapUpdated = true; }
    if (wL && !horiz_walls[currentRow][currentCol]) { horiz_walls[currentRow][currentCol] = true; mapUpdated = true; }
  }

  return mapUpdated;
}

bool advancePhaseIfNeeded() {
  if (currentPhase == TO_CENTER && manhattan_dist[currentRow][currentCol] == 0) {
    robotLog("Fase 0: Centro alcancado! Iniciando retorno.");
    currentPhase = TO_START;
    recalculateForCurrentPhase();
    return true;
  }

  if (currentPhase == TO_START && currentRow == 0 && currentCol == 0) {
    robotLog("Fase 1: Retorno concluido! Iniciando caminho otimo.");
    currentPhase = SPEED_RUN;
    recalculateForCurrentPhase();
    return true;
  }

  if (currentPhase == SPEED_RUN && manhattan_dist[currentRow][currentCol] == 0) {
    robotLog("Fase 2: Vitoria! Caminho otimo concluido.");
    currentPhase = FINISHED;
    return true;
  }

  return false;
}

Direction chooseNextDirection() {
  int minDist = 999;
  Direction nextDirection = currentDir;

  if (canMoveNorth(currentRow, currentCol) && manhattan_dist[currentRow + 1][currentCol] < minDist) {
    minDist = manhattan_dist[currentRow + 1][currentCol];
    nextDirection = NORTH;
  }
  if (canMoveEast(currentRow, currentCol) && manhattan_dist[currentRow][currentCol + 1] < minDist) {
    minDist = manhattan_dist[currentRow][currentCol + 1];
    nextDirection = EAST;
  }
  if (canMoveSouth(currentRow, currentCol) && manhattan_dist[currentRow - 1][currentCol] < minDist) {
    minDist = manhattan_dist[currentRow - 1][currentCol];
    nextDirection = SOUTH;
  }
  if (canMoveWest(currentRow, currentCol) && manhattan_dist[currentRow][currentCol - 1] < minDist) {
    nextDirection = WEST;
  }

  return nextDirection;
}

MoveCommand getMoveCommandForDirection(Direction nextDirection) {
  if (nextDirection == currentDir) {
    return MOVE_FORWARD;
  }

  if (nextDirection == (Direction)((currentDir + 1) % 4)) {
    return TURN_RIGHT;
  }

  if (nextDirection == (Direction)((currentDir + 3) % 4)) {
    return TURN_LEFT;
  }

  return TURN_BACK;
}

void updatePositionFromCommand(MoveCommand cmd) {
  if (cmd == TURN_LEFT) { currentDir = (Direction)((currentDir + 3) % 4); 
  } else if (cmd == TURN_RIGHT) { currentDir = (Direction)((currentDir + 1) % 4);
  } else if (cmd == TURN_BACK) { currentDir = (Direction)((currentDir + 2) % 4);
  } else if (cmd == STOP) { return; }

  if (currentDir == NORTH) { currentRow++;
  } else if (currentDir == EAST) { currentCol++;
  } else if (currentDir == SOUTH) { currentRow--;
  } else if (currentDir == WEST) { currentCol--; }
}

void runFloodFillStep() {
  if (currentPhase == FINISHED) { return; }

  advancePhaseIfNeeded();
  if (currentPhase == FINISHED) { return; }

  SensorWalls walls = getCurrentWalls();
  bool mapUpdated = updateWallsFromSensors(walls);

  if (mapUpdated) { recalculateForCurrentPhase(); }

  Direction nextDirection = chooseNextDirection();
  MoveCommand cmd = getMoveCommandForDirection(nextDirection);
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

  inicializeMaze();
  floodfill('C');
  printDistanceGrid();
  robotLog("Flood fill inicializado.");
}

void loop() {
  if (currentPhase == FINISHED) { delay(1000); return; }

  if (millis() - lastNavigationStepMs < NAVIGATION_STEP_INTERVAL_MS) { return; }

  lastNavigationStepMs = millis();

  atualizarSensoresParede();
  logWalls(getCurrentWalls());
  runFloodFillStep();
}
