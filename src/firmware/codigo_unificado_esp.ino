// =============================================================================
// MICROMOUSE UNIFICADO — ESP32
// Flood Fill + Sensores filtrados + PWM separado por motor
// Versão ajustada:
// - Limpa filtro dos sensores após curvas
// - Não anda automaticamente depois de virar
// - Corrige giro de 180 graus
// - Aumenta PWM de frente para evitar andar fraco
// =============================================================================

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <stdint.h>

// =============================================================================
// PINOS
// =============================================================================
#define SENSOR_LEFT   34
#define SENSOR_FRONT  34
#define SENSOR_RIGHT  36

// Motores
// MOTOR_IN1 = motor esquerdo para trás
// MOTOR_IN2 = motor esquerdo para frente
// MOTOR_IN3 = motor direito para trás
// MOTOR_IN4 = motor direito para frente
#define MOTOR_IN1 25
#define MOTOR_IN2 26
#define MOTOR_IN3 32
#define MOTOR_IN4 33

// RGB desativado por enquanto
#define USE_RGB 0
#define SENSOR_RGB_PIN 0

// Botão não será usado para iniciar automaticamente
#define BUTTON_PIN 0

// =============================================================================
// CONFIGURAÇÕES
// =============================================================================
#define MAZE_SIZE    16
#define INF          255
#define FILTER_SIZE  5

// =============================================================================
// PWM DOS MOTORES
// =============================================================================
// Se puxar para a direita, reduza PWM_LEFT ou aumente PWM_RIGHT.
// Se puxar para a esquerda, aumente PWM_LEFT ou reduza PWM_RIGHT.
#define PWM_LEFT 88
#define PWM_RIGHT 75

// PWM usado nas curvas
#define TURN_PWM_LEFT 110
#define TURN_PWM_RIGHT 110

// Tempos de movimento
#define TURN_TIME_90        445
#define TURN_TIME_180       950
#define CELL_FORWARD_TIME   500
#define STOP_TIME           400

// Atualização dos sensores
#define SENSOR_UPDATE_TIME  20
#define PRINT_SENSOR_TIME   150

// Cor de chegada no sensor RGB
#define FINISH_COLOR  0xFFFFFF

// =============================================================================
// COMUNICAÇÃO WEB
// =============================================================================
const char *ssid = "Micromouse_Telemetry";
const char *password = "12345678";

WebSocketsServer webSocket = WebSocketsServer(81);
unsigned long lastTelemetryMillis = 0;

constexpr uint8_t MAP_SIZE = 33;
constexpr uint8_t PATH_CAPACITY = MAZE_SIZE * MAZE_SIZE;

// =============================================================================
// ENUMS
// =============================================================================
enum RobotState : uint8_t {
    DESLIGADO,
    AGUARDANDO_INICIO,
    MOVIMENTO_FRENTE,
    PARADO,
    GIRAR_DIREITA,
    GIRAR_ESQUERDA,
    GIRO_180,
    FINALIZADO
};

enum Direction : uint8_t {
    NORTH = 0,
    EAST  = 1,
    SOUTH = 2,
    WEST  = 3
};

enum MoveCommand : uint8_t {
    MOVE_FORWARD,
    TURN_LEFT_CMD,
    TURN_RIGHT_CMD,
    TURN_BACK_CMD,
    STOP_CMD
};

enum AbsoluteWall : uint8_t {
    WALL_NORTH = 0,
    WALL_EAST  = 1,
    WALL_SOUTH = 2,
    WALL_WEST  = 3
};

// =============================================================================
// STRUCTS
// =============================================================================
struct Cell {
    uint8_t dist;
    uint8_t visited   : 1;
    uint8_t northWall : 1;
    uint8_t southWall : 1;
    uint8_t eastWall  : 1;
    uint8_t westWall  : 1;
};

struct Position {
    int8_t r;
    int8_t c;
};

struct TelemetryPoint {
    int8_t row;
    int8_t col;
};

uint8_t mapaLabirinto[MAP_SIZE][MAP_SIZE];
TelemetryPoint caminhoPercorrido[PATH_CAPACITY];
uint16_t caminhoPercorridoSize = 0;
uint32_t numeroTentativa = 0;
unsigned long inicioCorrida = 0;
bool desafioCumprido = false;
String tempoConclusaoISO;

const int8_t deltaLinha[4] = {1, 0, -1, 0};
const int8_t deltaColuna[4] = {0, 1, 0, -1};
const char *nomesDirecao[4] = {"NORTE", "LESTE", "SUL", "OESTE"};

// =============================================================================
// SENSOR DE PAREDE COM FILTRO
// =============================================================================
struct SensorParede {
    int  pin;
    int  readings[FILTER_SIZE];
    int  index;
    bool wallDetected;
};

void initSensorParede(SensorParede &s) {
    s.index = 0;
    s.wallDetected = false;

    for (int i = 0; i < FILTER_SIZE; i++) {
        s.readings[i] = HIGH;
    }

    // GPIO 34, 35 e 36 são somente entrada.
    // Use INPUT, não INPUT_PULLUP.
    pinMode(s.pin, INPUT);
}

// LOW = parede
bool lerSensorFiltrado(SensorParede &s) {
    s.readings[s.index] = digitalRead(s.pin);
    s.index = (s.index + 1) % FILTER_SIZE;

    int leiturasLow = 0;

    for (int i = 0; i < FILTER_SIZE; i++) {
        if (s.readings[i] == LOW) {
            leiturasLow++;
        }
    }

    // Com 5 leituras, 3 LOW já contam como parede
    return leiturasLow >= 3;
}

bool dentroDoLabirinto(int8_t linha, int8_t coluna) {
    return linha >= 0 && linha < MAZE_SIZE && coluna >= 0 && coluna < MAZE_SIZE;
}

void inicializarMapaWeb() {
    for (uint8_t i = 0; i < MAP_SIZE; i++) {
        for (uint8_t j = 0; j < MAP_SIZE; j++) {
            if (i == 0 || i == MAP_SIZE - 1 || j == 0 || j == MAP_SIZE - 1) {
                mapaLabirinto[i][j] = 1;
            } else {
                mapaLabirinto[i][j] = 2;
            }
        }
    }
}

void celulaParaMapa(int8_t linha, int8_t coluna, uint8_t &mapaLinha, uint8_t &mapaColuna) {
    mapaLinha = (uint8_t)(linha * 2 + 1);
    mapaColuna = (uint8_t)(coluna * 2 + 1);
}

void marcarCelulaVisitada(int8_t linha, int8_t coluna) {
    if (!dentroDoLabirinto(linha, coluna)) {
        return;
    }

    uint8_t mapaLinha = 0;
    uint8_t mapaColuna = 0;
    celulaParaMapa(linha, coluna, mapaLinha, mapaColuna);
    mapaLabirinto[mapaLinha][mapaColuna] = 0;
}

void marcarSegmentoVisitado(int8_t linhaAnterior, int8_t colunaAnterior, int8_t linhaAtualWeb, int8_t colunaAtualWeb) {
    if (!dentroDoLabirinto(linhaAnterior, colunaAnterior) || !dentroDoLabirinto(linhaAtualWeb, colunaAtualWeb)) {
        return;
    }

    uint8_t linhaA = 0;
    uint8_t colunaA = 0;
    uint8_t linhaB = 0;
    uint8_t colunaB = 0;

    celulaParaMapa(linhaAnterior, colunaAnterior, linhaA, colunaA);
    celulaParaMapa(linhaAtualWeb, colunaAtualWeb, linhaB, colunaB);

    mapaLabirinto[(linhaA + linhaB) / 2][(colunaA + colunaB) / 2] = 0;
}

void registrarCaminho(int8_t linha, int8_t coluna) {
    if (caminhoPercorridoSize >= PATH_CAPACITY) {
        return;
    }

    caminhoPercorrido[caminhoPercorridoSize++] = {linha, coluna};
}

String timestampISO(unsigned long elapsedMillis) {
    unsigned long totalSeconds = elapsedMillis / 1000UL;
    unsigned int seconds = totalSeconds % 60UL;
    unsigned int minutes = (totalSeconds / 60UL) % 60UL;
    unsigned int hours = (totalSeconds / 3600UL) % 24UL;

    char buffer[25];
    snprintf(buffer, sizeof(buffer), "2026-06-27T%02u:%02u:%02uZ", hours, minutes, seconds);
    return String(buffer);
}

String direcaoParaTexto(uint8_t direcao) {
    if (direcao < 4) {
        return String(nomesDirecao[direcao]);
    }

    return String("NORTE");
}

bool atCenter(Position pos) {
    const int8_t mid = MAZE_SIZE / 2;
    return (pos.r == mid - 1 || pos.r == mid) && (pos.c == mid - 1 || pos.c == mid);
}

uint8_t posicaoParaDirecaoAtual(Direction dir) {
    switch (dir) {
        case NORTH: return 0;
        case EAST:  return 1;
        case SOUTH: return 2;
        case WEST:  return 3;
    }

    return 0;
}

void atualizarMapaMovimento(Position anterior, Position atual) {
    if (dentroDoLabirinto(anterior.r, anterior.c)) {
        marcarCelulaVisitada(anterior.r, anterior.c);
    }

    if (dentroDoLabirinto(atual.r, atual.c)) {
        marcarCelulaVisitada(atual.r, atual.c);
        marcarSegmentoVisitado(anterior.r, anterior.c, atual.r, atual.c);
        registrarCaminho(atual.r, atual.c);
    }
}

void preencherMatriz(JsonDocument &doc, const char *chave) {
    JsonArray matriz = doc[chave].to<JsonArray>();

    for (uint8_t i = 0; i < MAP_SIZE; i++) {
        JsonArray linha = matriz.add<JsonArray>();
        for (uint8_t j = 0; j < MAP_SIZE; j++) {
            linha.add(mapaLabirinto[i][j]);
        }
    }
}

void preencherPayloadWeb(JsonDocument &doc) {
    uint8_t posMapaLinha = 0;
    uint8_t posMapaColuna = 0;
    uint8_t startMapaLinha = 0;
    uint8_t startMapaColuna = 0;
    uint8_t goalMapaLinha = 0;
    uint8_t goalMapaColuna = 0;

    celulaParaMapa(currentPos.r, currentPos.c, posMapaLinha, posMapaColuna);
    celulaParaMapa(0, 0, startMapaLinha, startMapaColuna);
    celulaParaMapa((int8_t)(MAZE_SIZE / 2 - 1), (int8_t)(MAZE_SIZE / 2 - 1), goalMapaLinha, goalMapaColuna);

    doc["numTentativa"] = numeroTentativa;
    doc["tempoColeta"] = timestampISO(millis() - inicioCorrida);
    doc["tensaoRecente"] = 7.4;
    doc["correnteRecente"] = 1.0;
    doc["posHRecente"] = currentPos.c;
    doc["posVRecente"] = currentPos.r;
    doc["velocidadeAtual"] = 0.55;
    doc["bateriaAtual"] = 100.0;
    doc["tensaoAtual"] = 7.4;
    doc["sensorCor"] = "#000000";
    doc["sensorEsquerda"] = sensorEsq.wallDetected ? 1 : 0;
    doc["sensorDireita"] = sensorDir.wallDetected ? 1 : 0;
    doc["sensorFrontal"] = sensorFrente.wallDetected ? 1 : 0;
    doc["tipoLabirinto"] = "16x16";
    doc["desafioCumprido"] = desafioCumprido ? "SIM" : "NAO";
    doc["status"] = desafioCumprido ? "success" : "running";
    doc["elapsedSeconds"] = millis() / 1000.0;
    doc["batteryPercent"] = 100.0;
    doc["speedMps"] = 0.55;

    JsonArray position = doc["position"].to<JsonArray>();
    position.add(posMapaLinha);
    position.add(posMapaColuna);

    JsonArray start = doc["start"].to<JsonArray>();
    start.add(startMapaLinha);
    start.add(startMapaColuna);

    JsonArray goal = doc["goal"].to<JsonArray>();
    goal.add(goalMapaLinha);
    goal.add(goalMapaColuna);

    JsonArray path = doc["visitedPath"].to<JsonArray>();
    for (uint16_t i = 0; i < caminhoPercorridoSize; i++) {
        uint8_t mapaLinha = 0;
        uint8_t mapaColuna = 0;
        celulaParaMapa(caminhoPercorrido[i].row, caminhoPercorrido[i].col, mapaLinha, mapaColuna);

        JsonArray point = path.add<JsonArray>();
        point.add(mapaLinha);
        point.add(mapaColuna);
    }

    JsonObject trajetoriaAtual = doc["trajetoAtual"].to<JsonObject>();
    trajetoriaAtual["numTentativa"] = numeroTentativa;
    trajetoriaAtual["passo"] = caminhoPercorridoSize == 0 ? 1 : caminhoPercorridoSize;
    trajetoriaAtual["pos_h"] = currentPos.c;
    trajetoriaAtual["pos_v"] = currentPos.r;
    trajetoriaAtual["direcao"] = direcaoParaTexto(posicaoParaDirecaoAtual(currentDir));

    if (desafioCumprido) {
        doc["tempoConclusao"] = timestampISO(millis() - inicioCorrida);
    }

    preencherMatriz(doc, "mapa");

    JsonObject historico = doc["historico"].to<JsonObject>();
    historico["percentualBateria"] = 100.0;
    historico["velocidadeMedia"] = 0.55;
    historico["tempoConclusao"] = desafioCumprido ? timestampISO(millis() - inicioCorrida) : "";
    historico["desafioCumprido"] = desafioCumprido ? "SIM" : "NAO";
    historico["correnteEletrica"] = 1.0;
    historico["tensaoEletrica"] = 7.4;
    historico["tipoLabirinto"] = "16x16";
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%d] Desconectado!\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%d] Conectado de %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
            break;
        }
        case WStype_TEXT:
            Serial.printf("[%d] Comando recebido: %s\n", num, payload);
            break;
        default:
            break;
    }
}

// Sensores globais
SensorParede sensorEsq    = {SENSOR_LEFT};
SensorParede sensorFrente = {SENSOR_FRONT};
SensorParede sensorDir    = {SENSOR_RIGHT};

void atualizarSensores() {
    sensorEsq.wallDetected    = lerSensorFiltrado(sensorEsq);
    sensorFrente.wallDetected = lerSensorFiltrado(sensorFrente);
    sensorDir.wallDetected    = lerSensorFiltrado(sensorDir);
}

// Limpa um sensor individual
void limparFiltroSensor(SensorParede &s) {
    s.index = 0;
    s.wallDetected = false;

    for (int i = 0; i < FILTER_SIZE; i++) {
        s.readings[i] = HIGH;
    }
}

// Limpa todos os filtros e força novas leituras
void limparFiltrosSensores() {
    limparFiltroSensor(sensorEsq);
    limparFiltroSensor(sensorFrente);
    limparFiltroSensor(sensorDir);

    for (int i = 0; i < FILTER_SIZE; i++) {
        atualizarSensores();
        delay(SENSOR_UPDATE_TIME);
    }
}

// =============================================================================
// SENSOR RGB
// =============================================================================
uint32_t lerRGB() {
#if USE_RGB
    // TODO: implementar leitura real do sensor RGB
    return 0x000000;
#else
    return 0x000000;
#endif
}

// =============================================================================
// FILA ESTÁTICA PARA FLOOD FILL
// =============================================================================
class PositionQueue {
private:
    Position data[MAZE_SIZE * MAZE_SIZE];
    uint16_t head = 0;
    uint16_t tail = 0;

public:
    void clear() {
        head = 0;
        tail = 0;
    }

    bool empty() const {
        return head == tail;
    }

    void push(Position p) {
        if (tail < MAZE_SIZE * MAZE_SIZE) {
            data[tail++] = p;
        }
    }

    Position pop() {
        return data[head++];
    }
};

// =============================================================================
// MAPA DO LABIRINTO
// =============================================================================
class MazeMap {
private:
    Cell cells[MAZE_SIZE][MAZE_SIZE];

public:
    void begin() {
        for (uint8_t r = 0; r < MAZE_SIZE; r++) {
            for (uint8_t c = 0; c < MAZE_SIZE; c++) {
                cells[r][c] = {INF, 0, 0, 0, 0, 0};
            }
        }

        // Paredes externas
        for (uint8_t i = 0; i < MAZE_SIZE; i++) {
            cells[0][i].southWall = 1;
            cells[MAZE_SIZE - 1][i].northWall = 1;
            cells[i][0].westWall = 1;
            cells[i][MAZE_SIZE - 1].eastWall = 1;
        }
    }

    inline Cell& get(int8_t r, int8_t c) {
        return cells[r][c];
    }

    inline bool valid(int8_t r, int8_t c) const {
        return r >= 0 && r < MAZE_SIZE && c >= 0 && c < MAZE_SIZE;
    }

    void setWall(Position pos, AbsoluteWall dir, bool wall) {
        if (!valid(pos.r, pos.c)) {
            return;
        }

        Cell &cur = get(pos.r, pos.c);

        switch (dir) {
            case WALL_NORTH:
                cur.northWall = wall;
                if (valid(pos.r + 1, pos.c)) {
                    get(pos.r + 1, pos.c).southWall = wall;
                }
                break;

            case WALL_SOUTH:
                cur.southWall = wall;
                if (valid(pos.r - 1, pos.c)) {
                    get(pos.r - 1, pos.c).northWall = wall;
                }
                break;

            case WALL_EAST:
                cur.eastWall = wall;
                if (valid(pos.r, pos.c + 1)) {
                    get(pos.r, pos.c + 1).westWall = wall;
                }
                break;

            case WALL_WEST:
                cur.westWall = wall;
                if (valid(pos.r, pos.c - 1)) {
                    get(pos.r, pos.c - 1).eastWall = wall;
                }
                break;
        }
    }
};

// =============================================================================
// FLOOD FILL
// =============================================================================
class FloodFillNavigator {
private:
    MazeMap *maze;
    PositionQueue queue;

    void processNeighbor(int8_t nr, int8_t nc, bool wall, uint8_t base) {
        if (wall || !maze->valid(nr, nc)) {
            return;
        }

        Cell &nbr = maze->get(nr, nc);

        if (nbr.dist != INF) {
            return;
        }

        nbr.dist = base + 1;
        queue.push({nr, nc});
    }

public:
    FloodFillNavigator(MazeMap *m) : maze(m) {}

    void computeMulti(Position goals[], uint8_t count) {
        for (uint8_t r = 0; r < MAZE_SIZE; r++) {
            for (uint8_t c = 0; c < MAZE_SIZE; c++) {
                maze->get(r, c).dist = INF;
            }
        }

        queue.clear();

        for (uint8_t i = 0; i < count; i++) {
            if (!maze->valid(goals[i].r, goals[i].c)) {
                continue;
            }

            maze->get(goals[i].r, goals[i].c).dist = 0;
            queue.push(goals[i]);
        }

        while (!queue.empty()) {
            Position p = queue.pop();
            Cell &cur = maze->get(p.r, p.c);
            uint8_t base = cur.dist;

            processNeighbor(p.r + 1, p.c, cur.northWall, base);
            processNeighbor(p.r - 1, p.c, cur.southWall, base);
            processNeighbor(p.r, p.c + 1, cur.eastWall, base);
            processNeighbor(p.r, p.c - 1, cur.westWall, base);
        }
    }
};

// =============================================================================
// CONTROLADOR DE MOTORES
// =============================================================================
class MotorController {
public:
    void begin() {
        pinMode(MOTOR_IN1, OUTPUT);
        pinMode(MOTOR_IN2, OUTPUT);
        pinMode(MOTOR_IN3, OUTPUT);
        pinMode(MOTOR_IN4, OUTPUT);

        stop();
    }

    void stop() {
        analogWrite(MOTOR_IN1, 0);
        analogWrite(MOTOR_IN2, 0);
        analogWrite(MOTOR_IN3, 0);
        analogWrite(MOTOR_IN4, 0);

        digitalWrite(MOTOR_IN1, LOW);
        digitalWrite(MOTOR_IN2, LOW);
        digitalWrite(MOTOR_IN3, LOW);
        digitalWrite(MOTOR_IN4, LOW);
    }

    // Motor esquerdo
    void motorEsquerdoFrente(int pwm) {
        analogWrite(MOTOR_IN1, 0);
        analogWrite(MOTOR_IN2, pwm);
    }

    void motorEsquerdoTras(int pwm) {
        analogWrite(MOTOR_IN1, pwm);
        analogWrite(MOTOR_IN2, 0);
    }

    // Motor direito
    void motorDireitoFrente(int pwm) {
        analogWrite(MOTOR_IN3, 0);
        analogWrite(MOTOR_IN4, pwm);
    }

    void motorDireitoTras(int pwm) {
        analogWrite(MOTOR_IN3, pwm);
        analogWrite(MOTOR_IN4, 0);
    }

    void forward() {
        motorEsquerdoFrente(PWM_LEFT);
        motorDireitoFrente(PWM_RIGHT);
    }

    void turnRight() {
        // Movimento físico para virar para a direita
        motorEsquerdoTras(TURN_PWM_LEFT);
        motorDireitoFrente(TURN_PWM_RIGHT);
    }

    void turnLeft() {
        // Movimento físico para virar para a esquerda
        motorEsquerdoFrente(TURN_PWM_LEFT);
        motorDireitoTras(TURN_PWM_RIGHT);
    }

    void turnBack() {
        // Giro de 180 usando o mesmo sentido da direita
        motorEsquerdoTras(TURN_PWM_LEFT);
        motorDireitoFrente(TURN_PWM_RIGHT);
    }

    void pararComLeitura(int tempoMs) {
        unsigned long inicio = millis();

        stop();

        while (millis() - inicio < (unsigned long)tempoMs) {
            atualizarSensores();
            delay(SENSOR_UPDATE_TIME);
        }
    }

    bool andarUmaCelula() {
        // Garante que o sensor frontal não está usando leitura antiga
        limparFiltrosSensores();

        if (sensorFrente.wallDetected) {
            stop();
            delay(STOP_TIME);
            return false;
        }

        unsigned long inicio = millis();

        forward();

        while (millis() - inicio < CELL_FORWARD_TIME) {
            atualizarSensores();

            // Se detectar parede na frente durante o movimento, para na hora.
            if (sensorFrente.wallDetected) {
                stop();
                delay(STOP_TIME);
                return false;
            }

            delay(SENSOR_UPDATE_TIME);
        }

        pararComLeitura(STOP_TIME);
        return true;
    }

    void virarDireita90() {
        pararComLeitura(STOP_TIME);

        unsigned long inicio = millis();

        turnRight();

        while (millis() - inicio < TURN_TIME_90) {
            atualizarSensores();
            delay(SENSOR_UPDATE_TIME);
        }

        stop();
        delay(100);

        limparFiltrosSensores();

        pararComLeitura(STOP_TIME);
    }

    void virarEsquerda90() {
        pararComLeitura(STOP_TIME);

        unsigned long inicio = millis();

        turnLeft();

        while (millis() - inicio < TURN_TIME_90) {
            atualizarSensores();
            delay(SENSOR_UPDATE_TIME);
        }

        stop();
        delay(100);

        limparFiltrosSensores();

        pararComLeitura(STOP_TIME);
    }

    void virar180() {
        pararComLeitura(STOP_TIME);

        unsigned long inicio = millis();

        turnBack();

        while (millis() - inicio < TURN_TIME_180) {
            atualizarSensores();
            delay(SENSOR_UPDATE_TIME);
        }

        stop();
        delay(100);

        limparFiltrosSensores();

        pararComLeitura(STOP_TIME);
    }

    bool execute(MoveCommand cmd) {
        switch (cmd) {
            case MOVE_FORWARD:
                return andarUmaCelula();

            case TURN_RIGHT_CMD:
                virarDireita90();
                return false;

            case TURN_LEFT_CMD:
                virarEsquerda90();
                return false;

            case TURN_BACK_CMD:
                virar180();
                return false;

            case STOP_CMD:
                stop();
                return false;
        }

        return false;
    }
};

// =============================================================================
// CONTROLADOR DE NAVEGAÇÃO
// =============================================================================
class NavigationController {
private:
    MazeMap *maze;

    bool blocked(Position from, Position to) {
        if (!maze->valid(to.r, to.c)) {
            return true;
        }

        Cell &cur = maze->get(from.r, from.c);

        if (to.r == from.r + 1) return cur.northWall;
        if (to.r == from.r - 1) return cur.southWall;
        if (to.c == from.c + 1) return cur.eastWall;
        if (to.c == from.c - 1) return cur.westWall;

        return true;
    }

public:
    NavigationController(MazeMap *m) : maze(m) {}

    MoveCommand decide(Position pos, Direction dir) {
        Position rel[4];

        switch (dir) {
            case NORTH:
                rel[0] = {pos.r + 1, pos.c}; // frente
                rel[1] = {pos.r, pos.c + 1}; // direita
                rel[2] = {pos.r, pos.c - 1}; // esquerda
                rel[3] = {pos.r - 1, pos.c}; // trás
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

            Cell &nbr = maze->get(rel[i].r, rel[i].c);

            if (nbr.dist < bestDist) {
                bestDist = nbr.dist;
                bestIdx = i;
            }
        }

        switch (bestIdx) {
            case 0:
                return MOVE_FORWARD;

            case 1:
                return TURN_RIGHT_CMD;

            case 2:
                return TURN_LEFT_CMD;

            default:
                return TURN_BACK_CMD;
        }
    }
};

// =============================================================================
// OBJETOS GLOBAIS
// =============================================================================
MazeMap maze;
FloodFillNavigator floodFill(&maze);
NavigationController navigator(&maze);
MotorController motors;

RobotState estadoAtual = DESLIGADO;
Position currentPos = {0, 0};
Direction currentDir = NORTH;

const uint8_t MID = MAZE_SIZE / 2;

Position centroGoals[4] = {
    {(int8_t)(MID - 1), (int8_t)(MID - 1)},
    {(int8_t)(MID - 1), (int8_t)MID},
    {(int8_t)MID,       (int8_t)(MID - 1)},
    {(int8_t)MID,       (int8_t)MID}
};

// =============================================================================
// FUNÇÕES AUXILIARES
// =============================================================================
void imprimirStatus() {
    Serial.print("POS: ");
    Serial.print(currentPos.r);
    Serial.print(",");
    Serial.print(currentPos.c);

    Serial.print(" | DIR: ");
    Serial.print(currentDir);

    Serial.print(" | ESQ: ");
    Serial.print(sensorEsq.wallDetected ? "PAREDE" : "LIVRE");

    Serial.print(" | FRENTE: ");
    Serial.print(sensorFrente.wallDetected ? "PAREDE" : "LIVRE");

    Serial.print(" | DIR_SENSOR: ");
    Serial.println(sensorDir.wallDetected ? "PAREDE" : "LIVRE");
}

// =============================================================================
// ATUALIZAÇÃO DE PAREDES
// =============================================================================
void updateWalls() {
    bool front = sensorFrente.wallDetected;
    bool right = sensorDir.wallDetected;
    bool left  = sensorEsq.wallDetected;

    switch (currentDir) {
        case NORTH:
            maze.setWall(currentPos, WALL_NORTH, front);
            maze.setWall(currentPos, WALL_EAST, right);
            maze.setWall(currentPos, WALL_WEST, left);
            break;

        case EAST:
            maze.setWall(currentPos, WALL_EAST, front);
            maze.setWall(currentPos, WALL_SOUTH, right);
            maze.setWall(currentPos, WALL_NORTH, left);
            break;

        case SOUTH:
            maze.setWall(currentPos, WALL_SOUTH, front);
            maze.setWall(currentPos, WALL_WEST, right);
            maze.setWall(currentPos, WALL_EAST, left);
            break;

        case WEST:
            maze.setWall(currentPos, WALL_WEST, front);
            maze.setWall(currentPos, WALL_NORTH, right);
            maze.setWall(currentPos, WALL_SOUTH, left);
            break;
    }
}

// =============================================================================
// ATUALIZAÇÃO DE POSIÇÃO
// =============================================================================
void updatePosition(MoveCommand cmd, bool avancou) {
    // Primeiro atualiza a direção, mesmo quando não avançou.
    // Isso é importante porque agora o robô vira em um loop
    // e só anda no próximo loop.
    switch (cmd) {
        case TURN_LEFT_CMD:
            currentDir = (Direction)((currentDir + 3) & 0x03);
            break;

        case TURN_RIGHT_CMD:
            currentDir = (Direction)((currentDir + 1) & 0x03);
            break;

        case TURN_BACK_CMD:
            currentDir = (Direction)((currentDir + 2) & 0x03);
            break;

        default:
            break;
    }

    if (!avancou || cmd == STOP_CMD) {
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

    currentPos.r = constrain(currentPos.r, 0, MAZE_SIZE - 1);
    currentPos.c = constrain(currentPos.c, 0, MAZE_SIZE - 1);
}

void publicarTelemetria() {
    JsonDocument doc;
    preencherPayloadWeb(doc);

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    webSocket.broadcastTXT(jsonPayload);
}

// =============================================================================
// VERIFICAÇÃO DE CHEGADA
// =============================================================================
bool atCenter() {
    return (currentPos.r == MID - 1 || currentPos.r == MID) &&
           (currentPos.c == MID - 1 || currentPos.c == MID);
}

// =============================================================================
// SETUP
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    randomSeed((uint32_t)millis());

    initSensorParede(sensorEsq);
    initSensorParede(sensorFrente);
    initSensorParede(sensorDir);

#if USE_RGB
    pinMode(SENSOR_RGB_PIN, INPUT);
#endif

    // Pode deixar configurado, mas não será usado para iniciar.
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    motors.begin();
    maze.begin();
    inicializarMapaWeb();

    numeroTentativa = random(1000, 9999);
    inicioCorrida = millis();
    caminhoPercorridoSize = 0;
    desafioCumprido = false;
    tempoConclusaoISO = "";

    registrarCaminho(currentPos.r, currentPos.c);
    marcarCelulaVisitada(currentPos.r, currentPos.c);

    Serial.println("Configurando Access Point...");
    WiFi.softAP(ssid, password);
    Serial.print("IP da ESP32: ");
    Serial.println(WiFi.softAPIP());

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    Serial.println("[MICROMOUSE] Hardware inicializado.");
    Serial.println("[MICROMOUSE] Iniciando automaticamente...");

    limparFiltrosSensores();

    floodFill.computeMulti(centroGoals, 4);

    delay(1000);

    estadoAtual = MOVIMENTO_FRENTE;

    Serial.println("[MICROMOUSE] Iniciando exploração...");
}

// =============================================================================
// LOOP PRINCIPAL
// =============================================================================
void loop() {
    if (estadoAtual == FINALIZADO) {
        webSocket.loop();
        return;
    }

    webSocket.loop();
    atualizarSensores();
    imprimirStatus();

    uint32_t corRGB = lerRGB();

    updateWalls();
    floodFill.computeMulti(centroGoals, 4);

    if (atCenter() || corRGB == FINISH_COLOR) {
        desafioCumprido = true;
        tempoConclusaoISO = timestampISO(millis() - inicioCorrida);
        estadoAtual = FINALIZADO;
        motors.stop();
        Serial.println("[MICROMOUSE] DESTINO ALCANCADO! Encerrando...");
    }

    if (estadoAtual != FINALIZADO) {
        MoveCommand cmd = navigator.decide(currentPos, currentDir);

        // Segurança: se o Flood Fill mandar ir para frente, mas tem parede,
        // escolhe uma alternativa simples.
        if (cmd == MOVE_FORWARD && sensorFrente.wallDetected) {
            if (!sensorDir.wallDetected) {
                cmd = TURN_RIGHT_CMD;
            } else if (!sensorEsq.wallDetected) {
                cmd = TURN_LEFT_CMD;
            } else {
                cmd = TURN_BACK_CMD;
            }
        }

        Position posicaoAnterior = currentPos;
        bool avancou = motors.execute(cmd);

        updatePosition(cmd, avancou);

        if (avancou) {
            atualizarMapaMovimento(posicaoAnterior, currentPos);
        }

        if (maze.valid(currentPos.r, currentPos.c)) {
            maze.get(currentPos.r, currentPos.c).visited = 1;
        }
    }

    publicarTelemetria();
    delay(20);
}