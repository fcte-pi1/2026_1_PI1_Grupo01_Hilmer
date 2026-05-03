#include <Arduino.h>
#include <queue>

const int MAZE_SIZE = 16; //Trocar valor para o tamanho do labirinto

//Bitwise utiliza bits para representar as paredes do labirinto
const uint8_t WALL_NORTH = 1; //0001
const uint8_t WALL_EAST = 2;  //0010
const uint8_t WALL_SOUTH = 4; //0100
const uint8_t WALL_WEST = 8;  //1000

int distances[MAZE_SIZE][MAZE_SIZE]; //Matriz para armazenar as distâncias do labirinto
uint8_t wall[MAZE_SIZE][MAZE_SIZE]; //Matriz para armazenar as paredes do labirinto

struct Cell {
    int x;
    int y;
};

void inicializarLabirinto() {
    // Inicializa as paredes do labirinto
    for (int i = 0; i < MAZE_SIZE; i++) {
        for (int j = 0; j < MAZE_SIZE; j++) {
            wall[i][j] = 0; // Sem paredes
            
            // Adiciona as paredes das bordas do labirinto
            if (i == 0) wall[i][j] |= WALL_WEST;
            if (i == MAZE_SIZE - 1) wall[i][j] |= WALL_EAST;
            if (j == 0) wall[i][j] |= WALL_NORTH; 
            if (j == MAZE_SIZE - 1) wall[i][j] |= WALL_SOUTH;
        }
    }
}