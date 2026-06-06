#include <iostream>
#include <string>
#include <cstring>
#include <cstdint>

const int MAX_MAZE_SIZE = 16;
int MAZE_SIZE = 16; // Agora é variável
const int BLANK = 255;

struct Coordinates { int r; int c; };

// int manhattan_dist[MAZE_SIZE][MAZE_SIZE]; // Código original comentado para otimização de memória
uint8_t manhattan_dist[MAX_MAZE_SIZE][MAX_MAZE_SIZE]; // Otimizado: 1 byte por célula permite o uso do memset com 255 e economiza RAM
bool horiz_walls[MAX_MAZE_SIZE + 1][MAX_MAZE_SIZE];
bool vert_walls[MAX_MAZE_SIZE][MAX_MAZE_SIZE + 1];

Coordinates fila[2048]; // Aumentado para 2048 para suportar múltiplas propagações de onda no Modified Floodfill
int head = 0; int tail = 0;

void push(Coordinates cell) { fila[tail] = cell; tail = (tail + 1) % 2048; }
Coordinates pop() { Coordinates cell = fila[head]; head = (head + 1) % 2048; return cell; }
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
int API_mazeWidth() { std::cout << "mazeWidth" << std::endl; std::string response; std::cin >> response; return std::stoi(response); }

void inicializeMaze() {
    /* // Código original comentado para otimização com memset
    for (int r = 0; r < MAZE_SIZE; r++) {
        for (int c = 0; c < MAZE_SIZE; c++) {
            manhattan_dist[r][c] = BLANK; horiz_walls[r][c] = false; vert_walls[r][c] = false;
        }
    }
    */
    memset(manhattan_dist, BLANK, sizeof(manhattan_dist));
    memset(horiz_walls, 0, sizeof(horiz_walls));
    memset(vert_walls, 0, sizeof(vert_walls));

    for (int i = 0; i < MAZE_SIZE; i++) {
        horiz_walls[0][i] = true; horiz_walls[MAZE_SIZE][i] = true;
        vert_walls[i][0] = true; vert_walls[i][MAZE_SIZE] = true;
    }
}

// Recalcula todo o labirinto (O(N^2)) - Usado apenas quando muda de alvo/Fase!
void floodfill_completo(char target) {
    /* // Código original comentado para otimização com memset
    for(int r = 0; r < MAZE_SIZE; r++) {
        for(int c = 0; c < MAZE_SIZE; c++) {
            manhattan_dist[r][c] = BLANK;
        }
    }
    */
    memset(manhattan_dist, BLANK, sizeof(manhattan_dist));

    head = 0; tail = 0;

    if (target == 'C') { // Configura o objetivo para o Centro do labirinto
        int mid = MAZE_SIZE / 2;
        manhattan_dist[mid - 1][mid - 1] = 0; push({mid - 1, mid - 1});
        manhattan_dist[mid - 1][mid] = 0; push({mid - 1, mid});
        manhattan_dist[mid][mid - 1] = 0; push({mid, mid - 1});
        manhattan_dist[mid][mid] = 0; push({mid, mid});
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

// Flood Fill Modificado (O(K) Atualização Parcial e Diferencial)
void modifiedFloodfill(char target) {
    // Ao chamar, a fila 'fila' recebe as células diretamente afetadas (as fronteiras das novas paredes)
    while (!isEmpty()) {
        Coordinates curr = pop();
        int r = curr.r; int c = curr.c;

        // Se for o alvo, não alteramos sua distância (que é 0)
        if (isTarget(r, c, target)) continue;

        int min_neighbor = getMinNeighbor(r, c);
        
        // Se a distância atual for inconsistente
        if (manhattan_dist[r][c] != min_neighbor + 1) {
            manhattan_dist[r][c] = min_neighbor + 1;
            
            // Desenho diferencial (Atualiza O simulador APENAS para as poucas células reavaliadas)
            API_setText(c, r, std::to_string(manhattan_dist[r][c]));

            // Propaga a onda de verificação pros vizinhos acessíveis
            if (!horiz_walls[r + 1][c]) push({r + 1, c});
            if (!horiz_walls[r][c]) push({r - 1, c});
            if (!vert_walls[r][c + 1]) push({r, c + 1});
            if (!vert_walls[r][c]) push({r, c - 1});
        }
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
    MAZE_SIZE = API_mazeWidth(); // Descobre o tamanho do labirinto chamando a API do MMS!
    
    inicializeMaze();
    floodfill_completo('C'); // Começa com o objetivo de ir para o centro
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
            floodfill_completo('S'); // Muda as aguas para fluirem para o (0,0)
            desenharDistanciasNoSimulador();
            // A API do MMS exige dar um tempo ou logar para não bugar a visualização
        } 
        else if (fase == 1 && r == 0 && c == 0) {
            API_log("Fase 1: Retorno concluido! Iniciandoa caminho ótimo");
            fase = 2;
            floodfill_completo('C'); // Muda as aguas para fluirem pro centro novamente
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

        // 2. ATUALIZAR MATRIZ DE PAREDES E EM Fila CASOS AFETADOS
        if (dir == 0) { // Norte
            if (wF && !horiz_walls[r+1][c]) { horiz_walls[r+1][c] = true; mapUpdated = true; push({r,c}); if (r+1 < MAZE_SIZE) push({r+1,c}); }
            if (wR && !vert_walls[r][c+1])  { vert_walls[r][c+1] = true; mapUpdated = true; push({r,c}); if (c+1 < MAZE_SIZE) push({r,c+1}); }
            if (wL && !vert_walls[r][c])    { vert_walls[r][c] = true; mapUpdated = true; push({r,c}); if (c-1 >= 0) push({r,c-1}); }
        } else if (dir == 1) { // Leste
            if (wF && !vert_walls[r][c+1])  { vert_walls[r][c+1] = true; mapUpdated = true; push({r,c}); if (c+1 < MAZE_SIZE) push({r,c+1}); }
            if (wR && !horiz_walls[r][c])   { horiz_walls[r][c] = true; mapUpdated = true; push({r,c}); if (r-1 >= 0) push({r-1,c}); }
            if (wL && !horiz_walls[r+1][c]) { horiz_walls[r+1][c] = true; mapUpdated = true; push({r,c}); if (r+1 < MAZE_SIZE) push({r+1,c}); }
        } else if (dir == 2) { // Sul
            if (wF && !horiz_walls[r][c])   { horiz_walls[r][c] = true; mapUpdated = true; push({r,c}); if (r-1 >= 0) push({r-1,c}); }
            if (wR && !vert_walls[r][c])    { vert_walls[r][c] = true; mapUpdated = true; push({r,c}); if (c-1 >= 0) push({r,c-1}); }
            if (wL && !vert_walls[r][c+1])  { vert_walls[r][c+1] = true; mapUpdated = true; push({r,c}); if (c+1 < MAZE_SIZE) push({r,c+1}); }
        } else if (dir == 3) { // Oeste
            if (wF && !vert_walls[r][c])    { vert_walls[r][c] = true; mapUpdated = true; push({r,c}); if (c-1 >= 0) push({r,c-1}); }
            if (wR && !horiz_walls[r+1][c]) { horiz_walls[r+1][c] = true; mapUpdated = true; push({r,c}); if (r+1 < MAZE_SIZE) push({r+1,c}); }
            if (wL && !horiz_walls[r][c])   { horiz_walls[r][c] = true; mapUpdated = true; push({r,c}); if (r-1 >= 0) push({r-1,c}); }
        }

        // 3. RECALCULAR O MAPA SE ACHOU PAREDE NOVA
        if (mapUpdated) {
            // Usa o Flood Fill Modificado (Atualização Rápida no máximo O(K) ao invés de N^2)
            if (fase == 1) modifiedFloodfill('S'); 
            else modifiedFloodfill('C');
            // A chamada desenharDistanciasNoSimulador() não é mais necessária aqui,
            // A tela é atualizada pelas variáveis que o modifiedFloodfill avisa iterativamente.
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