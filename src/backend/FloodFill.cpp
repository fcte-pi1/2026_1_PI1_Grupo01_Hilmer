#include <Arduino.h>

const int MAZE_SIZE = 16; //Trocar valor para o tamanho do labirinto
const int BLANK = 255; //Valor para representar células não visitadas no labirinto

struct Coordinates {
    int r; // Row
    int c; // Column
};

int manhattan_dist[MAZE_SIZE][MAZE_SIZE]; // Matriz para armazenar as distâncias de Manhattan
bool horiz_walls[MAZE_SIZE + 1][MAZE_SIZE]; // Matriz para armazenar as paredes horizontais
bool vert_walls[MAZE_SIZE][MAZE_SIZE + 1]; // Matriz para armazenar as paredes verticais

Coordinates fila[512];
int head = 0;
int tail = 0;

void push(Coordinates cell) {
    fila[tail] = cell;
    tail = (tail + 1) % 512; // Volta para 0 se passar de 511 (circular)
}

Coordinates pop() {
    Coordinates cell = fila[head];
    head = (head + 1) % 512;
    return cell;
}

bool isEmpty() {
    return head == tail;
}

void inicializeMaze() {
    for (int r = 0; r < MAZE_SIZE; r++) {
        for (int c = 0; c < MAZE_SIZE; c++) {
            manhattan_dist[r][c] = BLANK;
            horiz_walls[r][c] = false; // Inicialmente, sem paredes
            vert_walls[r][c] = false;
        }
    }


    // Config do perimetro externo do labirinto (Rato nao pode sair do labirinto)
    for (int i = 0; i < MAZE_SIZE; i++) {
        horiz_walls[0][i] = true; // fechando Sul (linha inferior)
        horiz_walls[MAZE_SIZE][i] = true; // fechando Norte (linha superior)
        vert_walls[i][0] = true; // fechando Oeste (coluna esquerda)
        vert_walls[i][MAZE_SIZE] = true; // fechando Leste (coluna direita)
    }
}

void floodfill () {

}


void setup() {
    Serial.begin(115200);
    inicializeMaze();
    floodfill();
    Serial.println("Labirinto inicializado e floodfill calculado.");
    // Configurações adicionais, como sensores, motores, etc.
}

void loop() {
    // Loop do rato será algo como
        // 1. Andar para a prox celula
        // 2. Parar no centro da celula
        // 3. Ler o Sensor
        // 4. Se descobriu parede nova -> recalculateFloodFill
        // 5. Repetir ate chegar no centro do labirinto

    delay(1000); // Apenas para evitar um loop muito rápido, pode ser removido
}