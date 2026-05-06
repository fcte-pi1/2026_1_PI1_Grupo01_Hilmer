#include <iostream>
#include <string>

const int MAZE_SIZE = 16;
const int BLANK = 255;

struct Coordinates { int r; int c; };

int manhattan_dist[MAZE_SIZE][MAZE_SIZE];
bool horiz_walls[MAZE_SIZE + 1][MAZE_SIZE];
bool vert_walls[MAZE_SIZE][MAZE_SIZE + 1];

Coordinates fila[512];
int head = 0; int tail = 0;

void push(Coordinates cell) { fila[tail] = cell; tail = (tail + 1) % 512; }
Coordinates pop() { Coordinates cell = fila[head]; head = (head + 1) % 512; return cell; }
bool isEmpty() { return head == tail; }


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
            manhattan_dist[r][c] = BLANK; horiz_walls[r][c] = false; vert_walls[r][c] = false;
        }
    }
    for (int i = 0; i < MAZE_SIZE; i++) {
        horiz_walls[0][i] = true; horiz_walls[MAZE_SIZE][i] = true;
        vert_walls[i][0] = true; vert_walls[i][MAZE_SIZE] = true;
    }
}

// Agora o floodfill aceita um alvo: 'C' (Center) ou 'S' (Start)
void floodfill(char target) {
    for(int r = 0; r < MAZE_SIZE; r++) {
        for(int c = 0; c < MAZE_SIZE; c++) {
            manhattan_dist[r][c] = BLANK;
        }
    }
    head = 0; tail = 0;

    if (target == 'C') { // Configura o objetivo para o Centro do labirinto
        manhattan_dist[7][7] = 0; push({7, 7});
        manhattan_dist[7][8] = 0; push({7, 8});
        manhattan_dist[8][7] = 0; push({8, 7});
        manhattan_dist[8][8] = 0; push({8, 8});
    } else if (target == 'S') { // Configura o objetivo para o Início (0,0)
        manhattan_dist[0][0] = 0; push({0, 0});
    }

    while (!isEmpty()) {
        Coordinates current = pop();
        int r = current.r; int c = current.c; int current_val = manhattan_dist[r][c];

        if (!horiz_walls[r + 1][c] && manhattan_dist[r + 1][c] == BLANK) { manhattan_dist[r + 1][c] = current_val + 1; push({r + 1, c}); }
        if (!horiz_walls[r][c] && manhattan_dist[r - 1][c] == BLANK) { manhattan_dist[r - 1][c] = current_val + 1; push({r - 1, c}); }
        if (!vert_walls[r][c + 1] && manhattan_dist[r][c + 1] == BLANK) { manhattan_dist[r][c + 1] = current_val + 1; push({r, c + 1}); }
        if (!vert_walls[r][c] && manhattan_dist[r][c - 1] == BLANK) { manhattan_dist[r][c - 1] = current_val + 1; push({r, c - 1}); }
    }
}

void desenharDistanciasNoSimulador() {
    for (int r = 0; r < MAZE_SIZE; r++) {
        for (int c = 0; c < MAZE_SIZE; c++) {
            API_setText(c, r, std::to_string(manhattan_dist[r][c]));
        }
    }
}

int main() {
    inicializeMaze();
    floodfill('C'); // Começa com o objetivo de ir para o centro
    desenharDistanciasNoSimulador();

    int r = 0;
    int c = 0;
    int dir = 0; // 0 = Norte, 1 = Leste, 2 = Sul, 3 = Oeste
    
    // Fases da missão do robô
    int fase = 0; // 0 = Indo pro centro, 1 = Voltando pro inicio, 2 = Speed Run

    while (true) {
        
        // VERIFICAR SE CHEGOU NO OBJETIVO DA FASE ATUAL
        if (fase == 0 && manhattan_dist[r][c] == 0) {
            API_log("Fase 0: Centro alcancado! Iniciando exploracao de retorno...");
            fase = 1;
            floodfill('S'); // Muda as aguas para fluirem para o (0,0)
            desenharDistanciasNoSimulador();
            // A API do MMS exige dar um tempo ou logar para não bugar a visualização
        } 
        else if (fase == 1 && r == 0 && c == 0) {
            API_log("Fase 1: Retorno concluido! Iniciandoa caminho ótimo");
            fase = 2;
            floodfill('C'); // Muda as aguas para fluirem pro centro novamente
            desenharDistanciasNoSimulador();
        }
        else if (fase == 2 && manhattan_dist[r][c] == 0) {
            API_log("Fase 2: VITORIA! Caminho ótimo concluido!");
            break; // Termina o programa com sucesso!
        }

        // 1. LER SENSORES
        bool wF = API_wallFront();
        bool wR = API_wallRight();
        bool wL = API_wallLeft();
        bool mapUpdated = false;

        // 2. ATUALIZAR MATRIZ DE PAREDES
        if (dir == 0) { // Norte
            if (wF && !horiz_walls[r+1][c]) { horiz_walls[r+1][c] = true; mapUpdated = true; }
            if (wR && !vert_walls[r][c+1])  { vert_walls[r][c+1] = true; mapUpdated = true; }
            if (wL && !vert_walls[r][c])    { vert_walls[r][c] = true; mapUpdated = true; }
        } else if (dir == 1) { // Leste
            if (wF && !vert_walls[r][c+1])  { vert_walls[r][c+1] = true; mapUpdated = true; }
            if (wR && !horiz_walls[r][c])   { horiz_walls[r][c] = true; mapUpdated = true; }
            if (wL && !horiz_walls[r+1][c]) { horiz_walls[r+1][c] = true; mapUpdated = true; }
        } else if (dir == 2) { // Sul
            if (wF && !horiz_walls[r][c])   { horiz_walls[r][c] = true; mapUpdated = true; }
            if (wR && !vert_walls[r][c])    { vert_walls[r][c] = true; mapUpdated = true; }
            if (wL && !vert_walls[r][c+1])  { vert_walls[r][c+1] = true; mapUpdated = true; }
        } else if (dir == 3) { // Oeste
            if (wF && !vert_walls[r][c])    { vert_walls[r][c] = true; mapUpdated = true; }
            if (wR && !horiz_walls[r+1][c]) { horiz_walls[r+1][c] = true; mapUpdated = true; }
            if (wL && !horiz_walls[r][c])   { horiz_walls[r][c] = true; mapUpdated = true; }
        }

        // 3. RECALCULAR O MAPA SE ACHOU PAREDE NOVA
        if (mapUpdated) {
            // Recalcula pro alvo certo dependendo da fase
            if (fase == 1) floodfill('S'); 
            else floodfill('C');
            desenharDistanciasNoSimulador();
        }

        // 4. ESCOLHER O VIZINHO COM O MENOR NÚMERO
        int min_dist = 999;
        int proxima_direcao = dir;

        if (!horiz_walls[r+1][c] && manhattan_dist[r+1][c] < min_dist) { min_dist = manhattan_dist[r+1][c]; proxima_direcao = 0; } // Norte
        if (!vert_walls[r][c+1] && manhattan_dist[r][c+1] < min_dist) { min_dist = manhattan_dist[r][c+1]; proxima_direcao = 1; } // Leste
        if (!horiz_walls[r][c] && manhattan_dist[r-1][c] < min_dist)   { min_dist = manhattan_dist[r-1][c]; proxima_direcao = 2; } // Sul
        if (!vert_walls[r][c] && manhattan_dist[r][c-1] < min_dist)   { min_dist = manhattan_dist[r][c-1]; proxima_direcao = 3; } // Oeste

        // 5. VIRAR O RATO PARA A DIREÇÃO ESCOLHIDA
        if (proxima_direcao == dir) {
            // Nao faz nada
        } else if (proxima_direcao == (dir + 1) % 4) {
            API_turnRight(); dir = proxima_direcao;
        } else if (proxima_direcao == (dir + 3) % 4) { 
            API_turnLeft(); dir = proxima_direcao;
        } else { 
            API_turnRight(); API_turnRight(); dir = proxima_direcao;
        }

        // 6. ANDAR
        API_moveForward();

        // 7. ATUALIZAR AS COORDENADAS INTERNAS
        if (dir == 0) r++;
        else if (dir == 1) c++;
        else if (dir == 2) r--;
        else if (dir == 3) c--;
    }

    return 0;
}
