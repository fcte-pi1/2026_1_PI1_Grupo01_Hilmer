#include "navigation/floodfill.h"
#include "memory/maze_memory.h"
#include "memory/sensor_memory.h"
#include "motors/motors.h"
#include <Arduino.h>
#include <string.h>

// ==========================================
// ESTRUTURAS DO FLOODFILL
// ==========================================
struct Coordinates { uint8_t r; uint8_t c; }; 

#define QUEUE_MAX 256
static uint32_t inQueue[MAZE_SIZE];
static Coordinates fila[QUEUE_MAX];
static int queue_head = 0; 
static int queue_tail = 0;

// ==========================================
// VARIÁVEIS DE ESTADO GLOBAIS DO RATO
// ==========================================
static int pos_r = 0;
static int pos_c = 0;
static int dir = 0;   // 0 = Norte, 1 = Leste, 2 = Sul, 3 = Oeste
static int fase = 0;  // 0 = Indo pro centro, 1 = Voltando pro inicio, 2 = Speed Run
static bool finalizado = false;

// ==========================================
// LÓGICA MATEMÁTICA DO FLOODFILL
// ==========================================
static void resetQueue() { 
    queue_head = 0; queue_tail = 0; memset(inQueue, 0, sizeof(inQueue)); 
}

static void pushUnique(Coordinates cell) {
  if (inQueue[cell.r] & (1u << cell.c)) return;
  if (((queue_tail + 1) & (QUEUE_MAX - 1)) == queue_head) return;
  inQueue[cell.r] |= (1u << cell.c);
  fila[queue_tail] = cell;
  queue_tail = (queue_tail + 1) & (QUEUE_MAX - 1);
}

static Coordinates popQueue() {
  Coordinates cell = fila[queue_head];
  inQueue[cell.r] &= ~(1u << cell.c);
  queue_head = (queue_head + 1) & (QUEUE_MAX - 1);
  return cell;
}

static bool isEmpty() { return queue_head == queue_tail; }

static void floodfill_completo(char target) {
  memset(manhattan_dist, BLANK, sizeof(manhattan_dist));
  resetQueue();
  if (target == 'C') { 
    int mid = MAZE_SIZE / 2;
    manhattan_dist[mid - 1][mid - 1] = 0; pushUnique({(uint8_t)(mid - 1), (uint8_t)(mid - 1)});
    manhattan_dist[mid - 1][mid] = 0; pushUnique({(uint8_t)(mid - 1), (uint8_t)mid});
    manhattan_dist[mid][mid - 1] = 0; pushUnique({(uint8_t)mid, (uint8_t)(mid - 1)});
    manhattan_dist[mid][mid] = 0; pushUnique({(uint8_t)mid, (uint8_t)mid});
  } else if (target == 'S') { 
    manhattan_dist[0][0] = 0; pushUnique({0, 0});
  }
  while (!isEmpty()) {
    Coordinates current = popQueue();
    int r = current.r; int c = current.c; int current_val = manhattan_dist[r][c];
    if (!horiz_walls[r + 1][c] && manhattan_dist[r + 1][c] == BLANK) { manhattan_dist[r + 1][c] = current_val + 1; pushUnique({(uint8_t)(r + 1), (uint8_t)c}); }
    if (!horiz_walls[r][c] && manhattan_dist[r - 1][c] == BLANK) { manhattan_dist[r - 1][c] = current_val + 1; pushUnique({(uint8_t)(r - 1), (uint8_t)c}); }
    if (!vert_walls[r][c + 1] && manhattan_dist[r][c + 1] == BLANK) { manhattan_dist[r][c + 1] = current_val + 1; pushUnique({(uint8_t)r, (uint8_t)(c + 1)}); }
    if (!vert_walls[r][c] && manhattan_dist[r][c - 1] == BLANK) { manhattan_dist[r][c - 1] = current_val + 1; pushUnique({(uint8_t)r, (uint8_t)(c - 1)}); }
  }
}

static int getMinNeighbor(int r, int c) {
  int min_val = 255;
  if (!horiz_walls[r + 1][c] && manhattan_dist[r + 1][c] < min_val) min_val = manhattan_dist[r + 1][c];
  if (!horiz_walls[r][c] && manhattan_dist[r - 1][c] < min_val) min_val = manhattan_dist[r - 1][c];
  if (!vert_walls[r][c + 1] && manhattan_dist[r][c + 1] < min_val) min_val = manhattan_dist[r][c + 1];
  if (!vert_walls[r][c] && manhattan_dist[r][c - 1] < min_val) min_val = manhattan_dist[r][c - 1];
  return min_val;
}

static bool isTarget(int r, int c, char target) {
  if (target == 'S') return (r == 0 && c == 0);
  if (target == 'C') {
    int mid = MAZE_SIZE / 2;
    return ((r == mid - 1 || r == mid) && (c == mid - 1 || c == mid));
  }
  return false;
}

static void modifiedFloodfill(char target) {
  while (!isEmpty()) {
    Coordinates curr = popQueue();
    int r = curr.r; int c = curr.c;
    if (isTarget(r, c, target)) continue;
    int min_neighbor = getMinNeighbor(r, c);
    if (manhattan_dist[r][c] != min_neighbor + 1) {
      manhattan_dist[r][c] = min_neighbor + 1;
      if (!horiz_walls[r + 1][c]) pushUnique({(uint8_t)(r + 1), (uint8_t)c});
      if (!horiz_walls[r][c]) pushUnique({(uint8_t)(r - 1), (uint8_t)c});
      if (!vert_walls[r][c + 1]) pushUnique({(uint8_t)r, (uint8_t)(c + 1)});
      if (!vert_walls[r][c]) pushUnique({(uint8_t)r, (uint8_t)(c - 1)});
    }
  }
}

// ==========================================
// ADAPTAÇÃO DA API PARA O ESP32
// ==========================================
void API_moveForward() { 
  motors_set_left(1.0);
  motors_set_right(1.0);
  // Movimento de teste/abstrato
  vTaskDelay(pdMS_TO_TICKS(1500)); 
  motors_stop();
}

void API_turnRight() { 
  motors_set_left(1.0);
  motors_set_right(-1.0);
  vTaskDelay(pdMS_TO_TICKS(800)); 
  motors_stop();
}

void API_turnLeft() { 
  motors_set_left(-1.0);
  motors_set_right(1.0);
  vTaskDelay(pdMS_TO_TICKS(800)); 
  motors_stop();
}

void floodfill_init() {
  maze_memory_init();
  floodfill_completo('C');
}

void floodfill_step() {
  if (finalizado) return; 

  if (fase == 0 && manhattan_dist[pos_r][pos_c] == 0) {
    fase = 1; floodfill_completo('S');
  } else if (fase == 1 && pos_r == 0 && pos_c == 0) {
    fase = 2; floodfill_completo('C');
  } else if (fase == 2 && manhattan_dist[pos_r][pos_c] == 0) {
    finalizado = true; 
    motors_stop();
    return; 
  }

  // Sensor threshold 
  bool wF = sensors.front > 0.5;
  bool wR = sensors.right > 0.5;
  bool wL = sensors.left > 0.5;
  bool mapUpdated = false;

  if (dir == 0) {
    if (wF && !horiz_walls[pos_r+1][pos_c]) { horiz_walls[pos_r+1][pos_c] = true; mapUpdated = true; pushUnique({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_r+1 < MAZE_SIZE) pushUnique({(uint8_t)(pos_r+1),(uint8_t)pos_c}); }
    if (wR && !vert_walls[pos_r][pos_c+1])  { vert_walls[pos_r][pos_c+1] = true; mapUpdated = true; pushUnique({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_c+1 < MAZE_SIZE) pushUnique({(uint8_t)pos_r,(uint8_t)(pos_c+1)}); }
    if (wL && !vert_walls[pos_r][pos_c])    { vert_walls[pos_r][pos_c] = true; mapUpdated = true; pushUnique({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_c-1 >= 0) pushUnique({(uint8_t)pos_r,(uint8_t)(pos_c-1)}); }
  } else if (dir == 1) {
    if (wF && !vert_walls[pos_r][pos_c+1])  { vert_walls[pos_r][pos_c+1] = true; mapUpdated = true; pushUnique({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_c+1 < MAZE_SIZE) pushUnique({(uint8_t)pos_r,(uint8_t)(pos_c+1)}); }
    if (wR && !horiz_walls[pos_r][pos_c])   { horiz_walls[pos_r][pos_c] = true; mapUpdated = true; pushUnique({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_r-1 >= 0) pushUnique({(uint8_t)(pos_r-1),(uint8_t)pos_c}); }
    if (wL && !horiz_walls[pos_r+1][pos_c]) { horiz_walls[pos_r+1][pos_c] = true; mapUpdated = true; pushUnique({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_r+1 < MAZE_SIZE) pushUnique({(uint8_t)(pos_r+1),(uint8_t)pos_c}); }
  } else if (dir == 2) {
    if (wF && !horiz_walls[pos_r][pos_c])   { horiz_walls[pos_r][pos_c] = true; mapUpdated = true; pushUnique({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_r-1 >= 0) pushUnique({(uint8_t)(pos_r-1),(uint8_t)pos_c}); }
    if (wR && !vert_walls[pos_r][pos_c])    { vert_walls[pos_r][pos_c] = true; mapUpdated = true; pushUnique({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_c-1 >= 0) pushUnique({(uint8_t)pos_r,(uint8_t)(pos_c-1)}); }
    if (wL && !vert_walls[pos_r][pos_c+1])  { vert_walls[pos_r][pos_c+1] = true; mapUpdated = true; pushUnique({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_c+1 < MAZE_SIZE) pushUnique({(uint8_t)pos_r,(uint8_t)(pos_c+1)}); }
  } else if (dir == 3) {
    if (wF && !vert_walls[pos_r][pos_c])    { vert_walls[pos_r][pos_c] = true; mapUpdated = true; pushUnique({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_c-1 >= 0) pushUnique({(uint8_t)pos_r,(uint8_t)(pos_c-1)}); }
    if (wR && !horiz_walls[pos_r+1][pos_c]) { horiz_walls[pos_r+1][pos_c] = true; mapUpdated = true; pushUnique({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_r+1 < MAZE_SIZE) pushUnique({(uint8_t)(pos_r+1),(uint8_t)pos_c}); }
    if (wL && !horiz_walls[pos_r][pos_c])   { horiz_walls[pos_r][pos_c] = true; mapUpdated = true; pushUnique({(uint8_t)pos_r,(uint8_t)pos_c}); if (pos_r-1 >= 0) pushUnique({(uint8_t)(pos_r-1),(uint8_t)pos_c}); }
  }

  if (mapUpdated) {
    if (fase == 1) modifiedFloodfill('S'); else modifiedFloodfill('C');
  }

  int min_dist = 999; int proxima_direcao = dir;
  if (!horiz_walls[pos_r+1][pos_c] && manhattan_dist[pos_r+1][pos_c] < min_dist) { min_dist = manhattan_dist[pos_r+1][pos_c]; proxima_direcao = 0; }
  if (!vert_walls[pos_r][pos_c+1] && manhattan_dist[pos_r][pos_c+1] < min_dist)  { min_dist = manhattan_dist[pos_r][pos_c+1]; proxima_direcao = 1; }
  if (!horiz_walls[pos_r][pos_c] && manhattan_dist[pos_r-1][pos_c] < min_dist)   { min_dist = manhattan_dist[pos_r-1][pos_c]; proxima_direcao = 2; }
  if (!vert_walls[pos_r][pos_c] && manhattan_dist[pos_r][pos_c-1] < min_dist)    { min_dist = manhattan_dist[pos_r][pos_c-1]; proxima_direcao = 3; }

  if (min_dist == 999) {
    finalizado = true; 
    motors_stop();
    return; 
  }

  if (proxima_direcao == (dir + 1) % 4) { API_turnRight(); dir = proxima_direcao; } 
  else if (proxima_direcao == (dir + 3) % 4) { API_turnLeft(); dir = proxima_direcao; } 
  else if (proxima_direcao != dir) { API_turnRight(); API_turnRight(); dir = proxima_direcao; }

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
    finalizado = true;
    motors_stop();
  }
}