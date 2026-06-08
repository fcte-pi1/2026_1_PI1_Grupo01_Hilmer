#include <Arduino.h>

// ==========================================
// CONFIGURAÇÕES DOS SENSORES E PINOS
// ==========================================
#define SENSOR_LEFT   25
#define SENSOR_FRONT  26
#define SENSOR_RIGHT  27
#define FILTER_SIZE 1

struct SensorParede {
  int pin;
  int readings[FILTER_SIZE];
  int index;
  bool wallDetected;
};

SensorParede sensorEsq    = {SENSOR_LEFT};
SensorParede sensorFrente = {SENSOR_FRONT};
SensorParede sensorDir    = {SENSOR_RIGHT};

// ==========================================
// ESTRUTURAS DO FLOODFILL
// ==========================================
const int MAX_MAZE_SIZE = 16;
int MAZE_SIZE = 16; 
const int BLANK = 255;

struct Coordinates { uint8_t r; uint8_t c; }; 

uint8_t manhattan_dist[MAX_MAZE_SIZE][MAX_MAZE_SIZE]; 
bool horiz_walls[MAX_MAZE_SIZE + 1][MAX_MAZE_SIZE];
bool vert_walls[MAX_MAZE_SIZE][MAX_MAZE_SIZE + 1];

#define QUEUE_MAX 256
uint32_t inQueue[MAX_MAZE_SIZE];
Coordinates fila[QUEUE_MAX];
int head = 0; int tail = 0;

// ==========================================
// VARIÁVEIS DE ESTADO GLOBAIS DO RATO
// ==========================================
int pos_r = 0;
int pos_c = 0;
int dir = 0;   // 0 = Norte, 1 = Leste, 2 = Sul, 3 = Oeste
int fase = 0;  // 0 = Indo pro centro, 1 = Voltando pro inicio, 2 = Speed Run
bool finalizado = false;

// ==========================================
// FUNÇÕES DE HARDWARE
// ==========================================
void initSensor(SensorParede &sensor) {
  sensor.index = 0;
  sensor.wallDetected = false;
  for (int i = 0; i < FILTER_SIZE; i++) {
    sensor.readings[i] = HIGH;
  }
}

bool lerSensorFiltrado(SensorParede &sensor) {
  int value = digitalRead(sensor.pin);
  sensor.readings[sensor.index] = value;
  sensor.index = (sensor.index + 1) % FILTER_SIZE;

  int sum = 0;
  for (int i = 0; i < FILTER_SIZE; i++) {
    sum += sensor.readings[i];
  }
  float average = sum / (float)FILTER_SIZE;
  return average < 0.5; 
}

// ==========================================
// ADAPTAÇÃO DA API PARA O ESP32
// ==========================================
void API_log(String text) { Serial.println("[LOG]: " + text); }
bool API_wallFront() { return lerSensorFiltrado(sensorFrente); }
bool API_wallRight() { return lerSensorFiltrado(sensorDir); }
bool API_wallLeft()  { return lerSensorFiltrado(sensorEsq); }

void API_setText(int x, int y, String text) { /* Ignorado no hardware físico */ }

void API_moveForward() { 
  Serial.println(">>> MOTOR: Andando 1 celula para FRENTE"); 
  delay(1500); 
}
void API_turnRight() { 
  Serial.println(">>> MOTOR: Girando 90 graus para DIREITA"); 
  delay(800); 
}
void API_turnLeft() { 
  Serial.println(">>> MOTOR: Girando 90 graus para ESQUERDA"); 
  delay(800); 
}

// ==========================================
// LÓGICA MATEMÁTICA DO FLOODFILL
// ==========================================
void resetQueue() { head = 0; tail = 0; memset(inQueue, 0, sizeof(inQueue)); }

void pushUnique(Coordinates cell) {
  if (inQueue[cell.r] & (1u << cell.c)) return;
  if (((tail + 1) & (QUEUE_MAX - 1)) == head) return;
  inQueue[cell.r] |= (1u << cell.c);
  fila[tail] = cell;
  tail = (tail + 1) & (QUEUE_MAX - 1);
}

void push(Coordinates cell) { pushUnique(cell); }

Coordinates pop() {
  Coordinates cell = fila[head];
  inQueue[cell.r] &= ~(1u << cell.c);
  head = (head + 1) & (QUEUE_MAX - 1);
  return cell;
}

bool isEmpty() { return head == tail; }

void inicializeMaze() {
  memset(manhattan_dist, BLANK, sizeof(manhattan_dist));
  memset(horiz_walls, 0, sizeof(horiz_walls));
  memset(vert_walls, 0, sizeof(vert_walls));
  for (int i = 0; i < MAZE_SIZE; i++) {
    horiz_walls[0][i] = true; horiz_walls[MAZE_SIZE][i] = true;
    vert_walls[i][0] = true; vert_walls[i][MAZE_SIZE] = true;
  }
}

void floodfill_completo(char target) {
  memset(manhattan_dist, BLANK, sizeof(manhattan_dist));
  resetQueue();
  if (target == 'C') { 
    int mid = MAZE_SIZE / 2;
    manhattan_dist[mid - 1][mid - 1] = 0; push({(uint8_t)(mid - 1), (uint8_t)(mid - 1)});
    manhattan_dist[mid - 1][mid] = 0; push({(uint8_t)(mid - 1), (uint8_t)mid});
    manhattan_dist[mid][mid - 1] = 0; push({(uint8_t)mid, (uint8_t)(mid - 1)});
    manhattan_dist[mid][mid] = 0; push({(uint8_t)mid, (uint8_t)mid});
  } else if (target == 'S') { 
    manhattan_dist[0][0] = 0; push({0, 0});
  }
  while (!isEmpty()) {
    Coordinates current = pop();
    int r = current.r; int c = current.c; int current_val = manhattan_dist[r][c];
    if (!horiz_walls[r + 1][c] && manhattan_dist[r + 1][c] == BLANK) { manhattan_dist[r + 1][c] = current_val + 1; push({(uint8_t)(r + 1), (uint8_t)c}); }
    if (!horiz_walls[r][c] && manhattan_dist[r - 1][c] == BLANK) { manhattan_dist[r - 1][c] = current_val + 1; push({(uint8_t)(r - 1), (uint8_t)c}); }
    if (!vert_walls[r][c + 1] && manhattan_dist[r][c + 1] == BLANK) { manhattan_dist[r][c + 1] = current_val + 1; push({(uint8_t)r, (uint8_t)(c + 1)}); }
    if (!vert_walls[r][c] && manhattan_dist[r][c - 1] == BLANK) { manhattan_dist[r][c - 1] = current_val + 1; push({(uint8_t)r, (uint8_t)(c - 1)}); }
  }
}

int getMinNeighbor(int r, int c) {
  int min_val = 255;
  if (!horiz_walls[r + 1][c] && manhattan_dist[r + 1][c] < min_val) min_val = manhattan_dist[r + 1][c];
  if (!horiz_walls[r][c] && manhattan_dist[r - 1][c] < min_val) min_val = manhattan_dist[r - 1][c];
  if (!vert_walls[r][c + 1] && manhattan_dist[r][c + 1] < min_val) min_val = manhattan_dist[r][c + 1];
  if (!vert_walls[r][c] && manhattan_dist[r][c - 1] < min_val) min_val = manhattan_dist[r][c - 1];
  return min_val;
}

bool isTarget(int r, int c, char target) {
  if (target == 'S') return (r == 0 && c == 0);
  if (target == 'C') {
    int mid = MAZE_SIZE / 2;
    return ((r == mid - 1 || r == mid) && (c == mid - 1 || c == mid));
  }
  return false;
}

void modifiedFloodfill(char target) {
  while (!isEmpty()) {
    Coordinates curr = pop();
    int r = curr.r; int c = curr.c;
    if (isTarget(r, c, target)) continue;
    int min_neighbor = getMinNeighbor(r, c);
    if (manhattan_dist[r][c] != min_neighbor + 1) {
      manhattan_dist[r][c] = min_neighbor + 1;
      if (!horiz_walls[r + 1][c]) push({(uint8_t)(r + 1), (uint8_t)c});
      if (!horiz_walls[r][c]) push({(uint8_t)(r - 1), (uint8_t)c});
      if (!vert_walls[r][c + 1]) push({(uint8_t)r, (uint8_t)(c + 1)});
      if (!vert_walls[r][c]) push({(uint8_t)r, (uint8_t)(c - 1)});
    }
  }
}

// ==========================================
// SETUP & LOOP PRINCIPAL
// ==========================================
void setup() {
  Serial.begin(115200);
  
  pinMode(SENSOR_LEFT, INPUT_PULLUP);
  pinMode(SENSOR_FRONT, INPUT_PULLUP);
  pinMode(SENSOR_RIGHT, INPUT_PULLUP);

  initSensor(sensorEsq);
  initSensor(sensorFrente);
  initSensor(sensorDir);

  inicializeMaze();
  floodfill_completo('C'); 
  
  Serial.println("====================================");
  Serial.println("CEREBRO DO MICRORATO INICIALIZADO!");
  Serial.println("Aguardando 3 segundos para dar a largada...");
  Serial.println("====================================");
  delay(3000); 
}

void loop() {
  if (finalizado) return; 

  if (fase == 0 && manhattan_dist[pos_r][pos_c] == 0) {
    API_log("Fase 0: Centro alcançado! Voltando pro inicio...");
    fase = 1; floodfill_completo('S');
  } else if (fase == 1 && pos_r == 0 && pos_c == 0) {
    API_log("Fase 1: Retorno concluído! Iniciando Speed Run!");
    fase = 2; floodfill_completo('C');
  } else if (fase == 2 && manhattan_dist[pos_r][pos_c] == 0) {
    API_log("VITÓRIA! Caminho ótimo concluído!");
    finalizado = true; return; 
  }

  bool wF = API_wallFront();
  bool wR = API_wallRight();
  bool wL = API_wallLeft();
  bool mapUpdated = false;

  Serial.printf("POS: (%d, %d) | DIR: %d | SENSORES -> Esq:%d Frente:%d Dir:%d\n", pos_r, pos_c, dir, wL, wF, wR);

  if (dir == 0) {
    if (wF && !horiz_walls[pos_r+1][pos_c]) { horiz_walls[pos_r+1][pos_c] = true; mapUpdated = true; push({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_r+1 < MAZE_SIZE) push({(uint8_t)(pos_r+1),(uint8_t)pos_c}); }
    if (wR && !vert_walls[pos_r][pos_c+1])  { vert_walls[pos_r][pos_c+1] = true; mapUpdated = true; push({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_c+1 < MAZE_SIZE) push({(uint8_t)pos_r,(uint8_t)(pos_c+1)}); }
    if (wL && !vert_walls[pos_r][pos_c])    { vert_walls[pos_r][pos_c] = true; mapUpdated = true; push({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_c-1 >= 0) push({(uint8_t)pos_r,(uint8_t)(pos_c-1)}); }
  } else if (dir == 1) {
    if (wF && !vert_walls[pos_r][pos_c+1])  { vert_walls[pos_r][pos_c+1] = true; mapUpdated = true; push({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_c+1 < MAZE_SIZE) push({(uint8_t)pos_r,(uint8_t)(pos_c+1)}); }
    if (wR && !horiz_walls[pos_r][pos_c])   { horiz_walls[pos_r][pos_c] = true; mapUpdated = true; push({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_r-1 >= 0) push({(uint8_t)(pos_r-1),(uint8_t)pos_c}); }
    if (wL && !horiz_walls[pos_r+1][pos_c]) { horiz_walls[pos_r+1][pos_c] = true; mapUpdated = true; push({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_r+1 < MAZE_SIZE) push({(uint8_t)(pos_r+1),(uint8_t)pos_c}); }
  } else if (dir == 2) {
    if (wF && !horiz_walls[pos_r][pos_c])   { horiz_walls[pos_r][pos_c] = true; mapUpdated = true; push({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_r-1 >= 0) push({(uint8_t)(pos_r-1),(uint8_t)pos_c}); }
    if (wR && !vert_walls[pos_r][pos_c])    { vert_walls[pos_r][pos_c] = true; mapUpdated = true; push({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_c-1 >= 0) push({(uint8_t)pos_r,(uint8_t)(pos_c-1)}); }
    if (wL && !vert_walls[pos_r][pos_c+1])  { vert_walls[pos_r][pos_c+1] = true; mapUpdated = true; push({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_c+1 < MAZE_SIZE) push({(uint8_t)pos_r,(uint8_t)(pos_c+1)}); }
  } else if (dir == 3) {
    if (wF && !vert_walls[pos_r][pos_c])    { vert_walls[pos_r][pos_c] = true; mapUpdated = true; push({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_c-1 >= 0) push({(uint8_t)pos_r,(uint8_t)(pos_c-1)}); }
    if (wR && !horiz_walls[pos_r+1][pos_c]) { horiz_walls[pos_r+1][pos_c] = true; mapUpdated = true; push({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_r+1 < MAZE_SIZE) push({(uint8_t)(pos_r+1),(uint8_t)pos_c}); }
    if (wL && !horiz_walls[pos_r][pos_c])   { horiz_walls[pos_r][pos_c] = true; mapUpdated = true; push({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_r-1 >= 0) push({(uint8_t)(pos_r-1),(uint8_t)pos_c}); }
  }

  if (mapUpdated) {
    if (fase == 1) modifiedFloodfill('S'); else modifiedFloodfill('C');
  }

  int min_dist = 999; int proxima_direcao = dir;
  if (!horiz_walls[pos_r+1][pos_c] && manhattan_dist[pos_r+1][pos_c] < min_dist) { min_dist = manhattan_dist[pos_r+1][pos_c]; proxima_direcao = 0; }
  if (!vert_walls[pos_r][pos_c+1] && manhattan_dist[pos_r][pos_c+1] < min_dist)  { min_dist = manhattan_dist[pos_r][pos_c+1]; proxima_direcao = 1; }
  if (!horiz_walls[pos_r][pos_c] && manhattan_dist[pos_r-1][pos_c] < min_dist)   { min_dist = manhattan_dist[pos_r-1][pos_c]; proxima_direcao = 2; }
  if (!vert_walls[pos_r][pos_c] && manhattan_dist[pos_r][pos_c-1] < min_dist)    { min_dist = manhattan_dist[pos_r][pos_c-1]; proxima_direcao = 3; }

  // ---------------------------------------------------------
  // TRAVA DE SEGURANÇA (FAIL-SAFE)
  // ---------------------------------------------------------
  if (min_dist == 999) {
    Serial.println(">>> ERRO CRITICO: Rato encurralado! Motores travados por segurança.");
    finalizado = true; 
    return; 
  }

  if (proxima_direcao == (dir + 1) % 4) { API_turnRight(); dir = proxima_direcao; } 
  else if (proxima_direcao == (dir + 3) % 4) { API_turnLeft(); dir = proxima_direcao; } 
  else if (proxima_direcao != dir) { API_turnRight(); API_turnRight(); dir = proxima_direcao; }

  // ---------------------------------------------------------
  // TRAVA DE FRONTEIRA DA MATRIZ
  // ---------------------------------------------------------
  bool movimento_seguro = false;
  if (dir == 0 && pos_r + 1 < MAZE_SIZE) movimento_seguro = true;
  if (dir == 1 && pos_c + 1 < MAZE_SIZE) movimento_seguro = true;
  if (dir == 2 && pos_r - 1 >= 0) movimento_seguro = true;
  if (dir == 3 && pos_c - 1 >= 0) movimento_seguro = true;

  if (movimento_seguro) {
    API_moveForward();
    if (dir == 0) pos_r++; 
    else if (dir == 1) pos_c++; 
    else if (dir == 2) pos_r--; 
    else if (dir == 3) pos_c--;
  } else {
    Serial.println(">>> ERRO CRITICO: Tentativa de sair dos limites do labirinto! Motores travados.");
    finalizado = true;
  }
}