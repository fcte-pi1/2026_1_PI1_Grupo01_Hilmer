#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <stdint.h>
#include <math.h>
#include <Wire.h>
#include <INA226.h>

#include <VL53L0X.h>
#include <Adafruit_APDS9960.h>

struct Position;
struct Cell;
struct TelemetryPoint;
struct FiltroMediaMovel;
struct SensorLateral;
struct SensorFrontal;
struct ClassificacaoLateralDecisao;
enum Direction : uint8_t;
enum MoveCommand : uint8_t;
enum AbsoluteWall : uint8_t;
enum RobotState : uint8_t;

enum ModoOperacao : uint8_t { MODO_MAPEAMENTO, MODO_CORRIDA };
class MazeMap;
class PositionQueue;
class FloodFillNavigator;
class MotorController;
class NavigationController;

#define PWM_LEFT             70     // PWM motor esquerdo andando reto
#define PWM_RIGHT            72     // PWM motor direito andando reto
#define TURN_PWM_LEFT         69    // PWM motor esquerdo nas curvas de 90/180
#define TURN_PWM_RIGHT         69    // PWM motor direito nas curvas de 90/180

#define TURN_TIME_90          320    // ms para girar 90 graus
#define TURN_TIME_180         810    // ms para girar 180 graus

#define TURN_TIME_45          160   // ms para girar 45 graus (curva de desempaque)


#define REVERSE_PULSE_PWM       70    // PWM do pulso de ré (mesmo para os 2 motores) — AUMENTADO (10 era baixo demais pra vencer o atrito estático do motor, ré não movia de verdade)


#define AUTO_INICIAR_SEM_SITE  true   // true = liga e já começa a mapear sozinho (teste standalone) / false = espera START do site

#define REVERSE_PULSE_FRACTION    15   // re curta antes de curva: 1/10 de celula (antes era 1/2 e tirava o rato do centro)
#define REVERSE_PULSE_TICKS      (TICKS_PER_CELL / REVERSE_PULSE_FRACTION)
#define REVERSE_PULSE_TIMEOUT_MS 220   // tempo MAXIMO de seguranca da re, caso o encoder falhe

#define MICRO_RE_PWM              70    // PWM da re pequena de desempacar — AUMENTADO (40 podia nao ser suficiente pra vencer o atrito estatico)
#define MICRO_RE_FRACTION         8    // re pequena de desempacar: 1/8 de celula
#define MICRO_RE_TICKS            (TICKS_PER_CELL / MICRO_RE_FRACTION)
#define MICRO_RE_TIMEOUT_MS       280   // tempo MAXIMO de seguranca dessa re pequena — AUMENTADO (120 podia ser curto demais pra sair da inercia)

#define MICRO_RE_TICKS_BASE       (TICKS_PER_CELL / 8)    // 1a tentativa: ~1/8 de celula
#define MICRO_RE_TICKS_STEP       (TICKS_PER_CELL / 12)   // cresce ~1/12 por tentativa
#define MICRO_RE_TICKS_MAX        (TICKS_PER_CELL / 2)    // teto: metade de uma celula
#define MICRO_RE_TIMEOUT_PROG_MS  450   // timeout de seguranca maior (a re pode ser maior agora)

#define TEMPO_ANALISE_ARREDORES_MS   1600  // mapeamento: parado 1.5s lendo os sensores com calma (pedido)
#define TEMPO_RELEITURA_CURVA_MS      250  // releitura parada antes de decidir curva com parede frontal
#define TEMPO_RELEITURA_APOS_GIRO_MS  180  // releitura curta depois de virar antes de tentar andar
#define MAX_GIROS_SEM_AVANCO_MESMA_CELULA 2  // trava anti-loop: nao deixa ficar girando no mesmo quadrado

#define I2C_SDA_PIN        21        // SDA da INA226 (confirmado no esquematico)
#define I2C_SCL_PIN        22        // SCL da INA226 — CONFIRME na sua fiacao
#define INA226_ENDERECO    0x40      // endereco I2C padrao do modulo (mude se o scanner apontar outro)
#define INA226_SHUNT_OHMS  0.1       // resistor shunt do modulo (tipico 0.1 ohm)
#define INA226_CORRENTE_MAX_A  2.0   // corrente maxima esperada (para calibracao)
#define BAT_TENSAO_MAX     8.4       // 2S Li-ion/LiPo carregada (~8.4V) = 100%
#define BAT_TENSAO_MIN     6.6       // corte de descarga aproximado (~6.0V) = 0%
#define BAT_UPDATE_MS      500       // intervalo de leitura da bateria (ms)

#define CELL_SIZE_CM            18.0   // tamanho de uma célula do labirinto (cm) — NÃO MEXER (fixo pela regra)
#define WHEEL_DIAMETER_CM        4.2   // diâmetro da roda em cm — AJUSTAR conforme medição real
#define ENCODER_PPR               15   // pulsos por volta do encoder — AJUSTAR conforme datasheet/teste

#define TICKS_PER_CELL 305

#define CELL_FORWARD_TIMEOUT_MS 1100  // tempo MAXIMO de seguranca por celula no mapeamento/correcao

#define GANHO_CORRECAO_RETA       1.4  // ajuste de PWM por tick de diferenca entre encoders
#define PWM_CORRECAO_MAX         22    // limite da correcao por encoders

#define KP_CENTRALIZACAO             0.25  // ganho proporcional da centralizacao (PWM por mm de erro)
#define ZONA_MORTA_CENTRALIZACAO_MM  5     // erro (mm) abaixo disso e ignorado
#define PWM_CORRECAO_LATERAL_MAX     22    // ajuste maximo da centralizacao lateral
#define PWM_CORRECAO_TOTAL_MAX       32    // trava a SOMA (reta + lateral) por motor

#define PWM_MOTOR_MIN            40    // piso de PWM em movimento reto (não deixa motor "morrer")
#define PWM_MOTOR_MAX           180    // teto de PWM em movimento reto
#define RAMPA_PWM_MAX_PASSO       6    // variação máxima de PWM por ciclo (~10ms) — evita trancos

#define STOP_TIME_SETTLE        100    // ms — assentamento mecânico antes de iniciar um giro

#define TENTATIVAS_PARA_RE_DESTRAVAR   3    // tentativas antes de tentar a re pequena de desempacar
#define MIN_TENTATIVAS_GIRO           10    // tentativas totais antes de avisar travamento (era 8)

#define FILTER_SIZE_DIST         5    // amostras na média móvel de cada sensor (maior = mais estável, porém mais lento p/ reagir)
#define SENSOR_UPDATE_TIME      10    // ms entre leituras dos sensores
#define PRINT_SENSOR_TIME      150    // ms entre prints de status (debug)

#define DIST_LATERAL_MUITO_PROXIMA_MM   25    // abaixo disso: quase encostando na parede lateral (perigo)
#define DIST_LATERAL_PAREDE_MM         90    // abaixo disso: considera que HÁ parede lateral naquele lado
#define DIST_LATERAL_LIVRE_MM          110    // acima disso: considera o lado livre (sem parede)
#define DIST_LATERAL_IDEAL_MM           75    // distância "ideal" de cada parede lateral quando bem centralizado
#define DIST_SEM_LEITURA_MM           2000    // usado quando o VL53L0X "não vê nada" (timeout/sem init) = bem longe/livre

// V4: para decidir curva, o lado precisa estar livre de forma mais clara
// do que o limiar usado apenas para imprimir LIVRE/PAREDE. Isso evita
// virar para uma parede quando o VL53L0X da um pico alto por reflexao/angulo.
#define DIST_LATERAL_LIVRE_CURVA_MM      175
#define DIST_LATERAL_LIVRE_MAPA_MM       185
#define AMOSTRAS_DECISAO_CURVA             9
#define MIN_AMOSTRAS_VALIDAS_CURVA         6
#define MIN_AMOSTRAS_PAREDE_CURVA          3
#define MIN_AMOSTRAS_LIVRE_CURVA           6
#define INTERVALO_AMOSTRA_DECISAO_MS      12
#define MARGEM_MEDIA_ESCOLHA_DUVIDOSA_MM  45

// Leituras laterais so entram na centralizacao quando parecem parede real.
// Isso evita o rato puxar para dentro de uma abertura lateral ou corredor sem parede.
#define DIST_LATERAL_MIN_CONFIAVEL_MM   22
#define DIST_LATERAL_MAX_CORRECAO_MM   170
#define DIST_LATERAL_PERIGO_MM          28
#define TICKS_IGNORAR_CORR_INICIO      (TICKS_PER_CELL / 8)
#define TICKS_IGNORAR_CORR_FIM         (TICKS_PER_CELL / 8)
#define TICKS_CELULA_OK_MIN            (TICKS_PER_CELL - (TICKS_PER_CELL / 10))

// ---- Limites do sensor FRONTAL (APDS9960, proximidade crua 0-255) ----
// Atenção: aqui é o INVERSO dos laterais -> valor MAIOR = mais PERTO da parede.
#define PROX_FRONTAL_PAREDE_DETECTADA   20   // acima disso: ha parede na frente
#define PROX_FRONTAL_LIVRE              16    // histerese: abaixo disso libera a frente
#define PROX_FRONTAL_MUITO_PROXIMA      45   // acima disso: parede frontal muito perto (risco de colisao)

#define MAX_MAZE_SIZE 16  // memória máxima reservada; o tamanho real vem do site
#define MAX_MAP_SIZE  (MAX_MAZE_SIZE * 2 + 1)
#define MAX_PATH_CAPACITY (MAX_MAZE_SIZE * MAX_MAZE_SIZE)

uint8_t mazeSize = 16;  // valor padrão; pode virar 4, 8 ou 16 quando o site mandar START

uint8_t mapSizeWeb() {
    return mazeSize + 2;
}

uint16_t pathCapacityAtual() {
    return mazeSize * mazeSize;
}

bool mazeSizeValido(uint8_t value) {
    return value == 4 || value == 8 || value == 16;
}

#define PWM_LEFT_MAPEAMENTO     70    // PWM mais lento pro modo mapeamento (cauteloso)
#define PWM_RIGHT_MAPEAMENTO    70
#define PWM_LEFT_CORRIDA        PWM_LEFT   // modo corrida usa os PWM normais (mais rápidos)
#define PWM_RIGHT_CORRIDA       PWM_RIGHT

#define VL53L0X_DIR_XSHUT_PIN         27     // XSHUT do VL53L0X que FISICAMENTE tem XSHUT ligado
#define VL53L0X_ENDERECO_SEM_XSHUT  0x30     // endereço novo do sensor que fica ligado desde o início (sem XSHUT)
#define VL53L0X_ENDERECO_COM_XSHUT  0x29     // sensor com XSHUT fica no próprio endereço padrão (único sobrando)

#define INVERTER_SENSORES_LATERAIS  false

// Motores
// MOTOR_IN1 = motor esquerdo para trás
// MOTOR_IN2 = motor esquerdo para frente
// MOTOR_IN3 = motor direito para trás
// MOTOR_IN4 = motor direito para frente
#define MOTOR_IN1 33   // esquerdo trás  (canal {32,33} agora está na esquerda física)
#define MOTOR_IN2 32   // esquerdo frente
#define MOTOR_IN3 26   // direito trás   (canal {25,26} agora está na direita física)
#define MOTOR_IN4 25   // direito frente

#define ENCODER_LEFT_A   19   // encoder do motor que agora está na esquerda física
#define ENCODER_LEFT_B   18
#define ENCODER_RIGHT_A  17   // encoder do motor que agora está na direita física
#define ENCODER_RIGHT_B  16

// RGB desativado por enquanto
#define USE_RGB 0
#define SENSOR_RGB_PIN 0

// Botão não será usado para iniciar automaticamente
#define BUTTON_PIN 0

#define INF          255

// Cor de chegada no sensor RGB
#define FINISH_COLOR  0xFFFFFF

// =============================================================================
// COMUNICAÇÃO WEB
// =============================================================================
const char *ssid = "Micromouse_Telemetry";
const char *password = "12345678";

WebSocketsServer webSocket = WebSocketsServer(81);
unsigned long lastTelemetryMillis = 0;

INA226 ina(INA226_ENDERECO);
bool inaOk = false;
float batTensaoAtual   = 7.4;    // V   (bus voltage)
float batPercentAtual  = 100.0;  // %   (derivado da tensao)
float batCorrenteAtual = 0.0;    // mA
unsigned long ultimoUpdateBateria = 0;
enum RobotState : uint8_t {
    DESLIGADO,
    AGUARDANDO_INICIO,
    MOVIMENTO_FRENTE,
    PARADO,
    GIRAR_DIREITA,
    GIRAR_ESQUERDA,
    GIRO_180,
    FINALIZADO,
    TRAVADO, // ========  nova alt: novo estado de travamento  ========
    MAPEAMENTO_CONCLUIDO
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

uint8_t mapaLabirinto[MAX_MAP_SIZE][MAX_MAP_SIZE];
TelemetryPoint caminhoPercorrido[MAX_PATH_CAPACITY];
uint16_t caminhoPercorridoSize = 0;
uint32_t numeroTentativa = 0;
unsigned long inicioCorrida = 0;
bool desafioCumprido = false;
String tempoConclusaoISO;

const int8_t deltaLinha[4] = {1, 0, -1, 0};
const int8_t deltaColuna[4] = {0, 1, 0, -1};
const char *nomesDirecao[4] = {"NORTE", "LESTE", "SUL", "OESTE"};

volatile long encoderLeftCount = 0;
volatile long encoderRightCount = 0;

void IRAM_ATTR isrEncoderLeft() {
    if (digitalRead(ENCODER_LEFT_B) == HIGH) {
        encoderLeftCount++;
    } else {
        encoderLeftCount--;
    }
}

void IRAM_ATTR isrEncoderRight() {
    if (digitalRead(ENCODER_RIGHT_B) == HIGH) {
        encoderRightCount++;
    } else {
        encoderRightCount--;
    }
}

void initEncoders() {
    pinMode(ENCODER_LEFT_A, INPUT_PULLUP);
    pinMode(ENCODER_LEFT_B, INPUT_PULLUP);
    pinMode(ENCODER_RIGHT_A, INPUT_PULLUP);
    pinMode(ENCODER_RIGHT_B, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(ENCODER_LEFT_A), isrEncoderLeft, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCODER_RIGHT_A), isrEncoderRight, RISING);
}

void resetEncoders() {
    noInterrupts();
    encoderLeftCount = 0;
    encoderRightCount = 0;
    interrupts();
}

long lerMediaEncoders() {
    long left;
    long right;

    noInterrupts();
    left = encoderLeftCount;
    right = encoderRightCount;
    interrupts();

    return (labs(left) + labs(right)) / 2;
}


float tensaoParaPercentualBateria(float tensao) {

    float pct = (tensao - BAT_TENSAO_MIN) / (BAT_TENSAO_MAX - BAT_TENSAO_MIN) * 100.0;
    return constrain(pct, 0.0, 100.0);
}

void initBateriaINA226() {
    if (ina.begin()) {
        ina.setMaxCurrentShunt(INA226_CORRENTE_MAX_A, INA226_SHUNT_OHMS);
        inaOk = true;
        Serial.println("[MICROMOUSE] INA226 (bateria) inicializada com sucesso.");
    } else {
        inaOk = false;
        Serial.println("[MICROMOUSE] AVISO: INA226 nao encontrada no I2C.");
        Serial.println("             Bateria vai reportar os ultimos valores padrao.");
        Serial.println("             Confira SDA(21)/SCL(22), VCC/GND e endereco (0x40).");
    }
}

void atualizarBateria() {
    if (!inaOk) {
        return;
    }

    unsigned long agora = millis();
    if (agora - ultimoUpdateBateria < BAT_UPDATE_MS) {
        return;
    }
    ultimoUpdateBateria = agora;

    float tensao = ina.getBusVoltage();

    if (tensao > 1.0) {
        batTensaoAtual   = tensao;
        batPercentAtual  = tensaoParaPercentualBateria(tensao);
        batCorrenteAtual = ina.getCurrent_mA();
    }
}


struct FiltroMediaMovel {
    uint16_t leituras[FILTER_SIZE_DIST];
    uint8_t  indice;
    uint32_t soma;
};

void iniciarFiltro(FiltroMediaMovel &f, uint16_t valorInicial) {
    f.indice = 0;
    f.soma = (uint32_t)valorInicial * FILTER_SIZE_DIST;

    for (uint8_t i = 0; i < FILTER_SIZE_DIST; i++) {
        f.leituras[i] = valorInicial;
    }
}

uint16_t atualizarFiltro(FiltroMediaMovel &f, uint16_t novaLeitura) {
    f.soma -= f.leituras[f.indice];
    f.leituras[f.indice] = novaLeitura;
    f.soma += novaLeitura;
    f.indice = (f.indice + 1) % FILTER_SIZE_DIST;

    return (uint16_t)(f.soma / FILTER_SIZE_DIST);
}


struct SensorLateral {
    VL53L0X  vl;
    bool     ok;              // true se o sensor respondeu no I2C na inicialização
    FiltroMediaMovel filtro;
    uint16_t distanciaMM;      // última média filtrada
    bool     paredeDetectada;  // distanciaMM < DIST_LATERAL_PAREDE_MM
    bool     muitoProxima;     // distanciaMM < DIST_LATERAL_MUITO_PROXIMA_MM
};


struct SensorFrontal {
    Adafruit_APDS9960 apds;
    bool     ok;
    FiltroMediaMovel filtro;
    uint16_t proximidade;      // última média filtrada (0-255)
    bool     paredeDetectada;  // proximidade > PROX_FRONTAL_PAREDE_DETECTADA
    bool     muitoProxima;     // proximidade > PROX_FRONTAL_MUITO_PROXIMA
};


uint16_t lerDistanciaLateralMM(VL53L0X &vl, bool ok) {
    if (!ok) {
        return DIST_SEM_LEITURA_MM;
    }

    uint16_t mm = vl.readRangeContinuousMillimeters();

    if (vl.timeoutOccurred() || mm == 0 || mm > 1200) {
        return DIST_SEM_LEITURA_MM;
    }

    return mm;
}

uint16_t lerProximidadeFrontal(Adafruit_APDS9960 &apds, bool ok) {
    if (!ok) {
        return 0;
    }

    return apds.readProximity();
}

bool dentroDoLabirinto(int8_t linha, int8_t coluna) {
    return linha >= 0 && linha < mazeSize && coluna >= 0 && coluna < mazeSize;
}

void inicializarMapaWeb() {
    uint8_t tamanhoMapa = mapSizeWeb();

    for (uint8_t i = 0; i < tamanhoMapa; i++) {
        for (uint8_t j = 0; j < tamanhoMapa; j++) {
            if (i == 0 || i == tamanhoMapa - 1 || j == 0 || j == tamanhoMapa - 1) {
                mapaLabirinto[i][j] = 1;
            } else {
                mapaLabirinto[i][j] = 2;
            }
        }
    }
}

void celulaParaMapa(int8_t linha, int8_t coluna, uint8_t &mapaLinha, uint8_t &mapaColuna) {
    mapaLinha = (uint8_t)(linha + 1);
    mapaColuna = (uint8_t)(coluna + 1);
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
    (void)linhaAnterior;
    (void)colunaAnterior;
    (void)linhaAtualWeb;
    (void)colunaAtualWeb;
}

void registrarCaminho(int8_t linha, int8_t coluna) {
    if (caminhoPercorridoSize >= pathCapacityAtual()) {
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
    const int8_t mid = mazeSize / 2;
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
    uint8_t tamanhoMapa = mapSizeWeb();

    for (uint8_t i = 0; i < tamanhoMapa; i++) {
        JsonArray linha = matriz.add<JsonArray>();
        for (uint8_t j = 0; j < tamanhoMapa; j++) {
            linha.add(mapaLabirinto[i][j]);
        }
    }
}

// Forward declarations for globals used by telemetry
extern struct SensorLateral sensorEsq;
extern struct SensorLateral sensorDir;
extern struct SensorFrontal sensorFrente;
extern Position currentPos;
extern Direction currentDir;
extern RobotState estadoAtual;
extern ModoOperacao modoAtual;
extern bool robotTravado; // ========  nova alt: flag de travamento usada na telemetria  ========

void preencherPayloadWeb(JsonDocument &doc) {
    uint8_t posMapaLinha = 0;
    uint8_t posMapaColuna = 0;
    uint8_t startMapaLinha = 0;
    uint8_t startMapaColuna = 0;
    uint8_t goalMapaLinha = 0;
    uint8_t goalMapaColuna = 0;

    celulaParaMapa(currentPos.r, currentPos.c, posMapaLinha, posMapaColuna);
    celulaParaMapa(0, 0, startMapaLinha, startMapaColuna);
    celulaParaMapa((int8_t)(mazeSize / 2 - 1), (int8_t)(mazeSize / 2 - 1), goalMapaLinha, goalMapaColuna);

    float correnteA = batCorrenteAtual / 1000.0;

    doc["numTentativa"] = numeroTentativa;
    doc["tempoColeta"] = timestampISO(millis() - inicioCorrida);
    // ========  nova alt (bateria): valores reais da INA226  ========
    doc["tensaoRecente"] = batTensaoAtual;
    doc["correnteRecente"] = correnteA;
    doc["posHRecente"] = currentPos.c;
    doc["posVRecente"] = currentPos.r;
    doc["velocidadeAtual"] = 0.55;
    doc["bateriaAtual"] = batPercentAtual;   // ========  nova alt (bateria)  ========
    doc["tensaoAtual"] = batTensaoAtual;     // ========  nova alt (bateria)  ========
    doc["sensorCor"] = "#000000";
    doc["sensorEsquerda"] = sensorEsq.paredeDetectada ? 1 : 0;
    doc["sensorDireita"] = sensorDir.paredeDetectada ? 1 : 0;
    doc["sensorFrontal"] = sensorFrente.paredeDetectada ? 1 : 0;
    // ========  nova alt (sensores de distância): valores crus, úteis para debug/calibração  ========
    doc["sensorEsquerdaMM"] = sensorEsq.distanciaMM;
    doc["sensorDireitaMM"] = sensorDir.distanciaMM;
    doc["sensorFrontalProx"] = sensorFrente.proximidade;

    String tipoLabirinto = String(mazeSize) + "x" + String(mazeSize);
    doc["mazeSize"] = mazeSize;
    doc["mapSize"] = mapSizeWeb();
    doc["tipoLabirinto"] = tipoLabirinto;
    doc["estadoRobo"] = estadoAtual;
    doc["modoOperacao"] = modoAtual == MODO_MAPEAMENTO ? "MAPEAMENTO" : "CORRIDA";
    doc["aguardandoInicio"] = estadoAtual == AGUARDANDO_INICIO;
    doc["aguardandoCorrida"] = estadoAtual == MAPEAMENTO_CONCLUIDO;
    doc["travado"] = robotTravado;
    doc["desafioCumprido"] = desafioCumprido ? "SIM" : "NAO";
    if (desafioCumprido) {
        doc["status"] = "success";
    } else if (robotTravado) {
        doc["status"] = "stuck";
    } else if (estadoAtual == AGUARDANDO_INICIO) {
        doc["status"] = "waiting_start";
    } else if (estadoAtual == MAPEAMENTO_CONCLUIDO) {
        doc["status"] = "waiting_run";
    } else {
        doc["status"] = "running";
    }
    doc["elapsedSeconds"] = millis() / 1000.0;
    doc["batteryPercent"] = batPercentAtual;
    doc["speedMps"] = 0.55;
    doc["tensaoEletrica"] = batTensaoAtual;
    doc["correnteEletrica"] = correnteA;

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
    // ========  nova alt (bateria): valores reais da INA226 no historico  ========
    historico["percentualBateria"] = batPercentAtual;
    historico["velocidadeMedia"] = 0.55;
    historico["tempoConclusao"] = desafioCumprido ? timestampISO(millis() - inicioCorrida) : "";
    historico["desafioCumprido"] = desafioCumprido ? "SIM" : "NAO";
    historico["correnteEletrica"] = correnteA;
    historico["tensaoEletrica"] = batTensaoAtual;
    historico["tipoLabirinto"] = tipoLabirinto;
    historico["mazeSize"] = mazeSize;
}

bool comandoIniciarCorrida = false;
bool comandoIniciarMapeamento = false;
bool comandoPararRato = false;
uint8_t comandoMazeSize = 16;

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
        case WStype_TEXT: {
            Serial.printf("[%d] Comando recebido: %s\n", num, payload);

            JsonDocument cmdDoc;
            DeserializationError error = deserializeJson(cmdDoc, payload, length);

            if (!error) {
                String type = cmdDoc["type"] | "";
                type.toUpperCase();

                if (type == "START") {
                    uint8_t novoMazeSize = cmdDoc["mazeSize"] | mazeSize;

                    JsonDocument resposta;
                    resposta["type"] = "ACK";
                    resposta["command"] = "START";

                    if (!mazeSizeValido(novoMazeSize)) {
                        resposta["type"] = "ERROR";
                        resposta["message"] = "mazeSize invalido. Use 4, 8 ou 16.";
                        resposta["receivedMazeSize"] = novoMazeSize;
                    } else {
                        comandoMazeSize = novoMazeSize;
                        comandoIniciarMapeamento = true;
                        resposta["mazeSize"] = novoMazeSize;
                        resposta["status"] = "queued";
                    }

                    String out;
                    serializeJson(resposta, out);
                    webSocket.sendTXT(num, out);
                    break;
                }

                if (type == "START_RUN") {
                    comandoIniciarCorrida = true;

                    JsonDocument resposta;
                    resposta["type"] = "ACK";
                    resposta["command"] = "START_RUN";
                    resposta["status"] = "queued";

                    String out;
                    serializeJson(resposta, out);
                    webSocket.sendTXT(num, out);
                    break;
                }

                if (type == "STOP") {
                    comandoPararRato = true;

                    JsonDocument resposta;
                    resposta["type"] = "ACK";
                    resposta["command"] = "STOP";
                    resposta["status"] = "queued";

                    String out;
                    serializeJson(resposta, out);
                    webSocket.sendTXT(num, out);
                    break;
                }
            }

            // Compatibilidade com o protocolo antigo: texto puro
            // "INICIAR_CORRIDA".
            String mensagem = String((char *)payload);
            mensagem.toUpperCase();

            if (mensagem.indexOf("INICIAR_CORRIDA") >= 0) {
                comandoIniciarCorrida = true;
                Serial.println("[MICROMOUSE] Comando INICIAR_CORRIDA recebido via WebSocket.");
            }
            break;
        }
        default:
            break;
    }
}

SensorLateral  sensorEsq;
SensorLateral  sensorDir;
SensorFrontal  sensorFrente;

void iniciarSensoresDistancia() {
    SensorLateral &sensorSemXshut = INVERTER_SENSORES_LATERAIS ? sensorDir : sensorEsq;
    SensorLateral &sensorComXshut = INVERTER_SENSORES_LATERAIS ? sensorEsq : sensorDir;

    pinMode(VL53L0X_DIR_XSHUT_PIN, OUTPUT);
    digitalWrite(VL53L0X_DIR_XSHUT_PIN, LOW);
    delay(10);

    sensorSemXshut.vl.setTimeout(50);
    sensorSemXshut.ok = sensorSemXshut.vl.init();

    if (sensorSemXshut.ok) {
        sensorSemXshut.vl.setAddress(VL53L0X_ENDERECO_SEM_XSHUT);
        sensorSemXshut.vl.setMeasurementTimingBudget(20000);
        sensorSemXshut.vl.startContinuous(20);
        Serial.println("[MICROMOUSE] VL53L0X (sem XSHUT) inicializado.");
    } else {
        Serial.println("[MICROMOUSE] AVISO: VL53L0X (sem XSHUT) nao respondeu no I2C!");
    }

    digitalWrite(VL53L0X_DIR_XSHUT_PIN, HIGH);
    delay(10);

    sensorComXshut.vl.setTimeout(50);
    sensorComXshut.ok = sensorComXshut.vl.init();

    if (sensorComXshut.ok) {
        sensorComXshut.vl.setAddress(VL53L0X_ENDERECO_COM_XSHUT);
        sensorComXshut.vl.setMeasurementTimingBudget(20000);
        sensorComXshut.vl.startContinuous(20);
        Serial.println("[MICROMOUSE] VL53L0X (com XSHUT) inicializado.");
    } else {
        Serial.println("[MICROMOUSE] AVISO: VL53L0X (com XSHUT) nao respondeu no I2C!");
    }

    sensorFrente.ok = sensorFrente.apds.begin();

    if (sensorFrente.ok) {
        sensorFrente.apds.enableProximity(true);
        Serial.println("[MICROMOUSE] APDS9960 FRONTAL inicializado.");
    } else {
        Serial.println("[MICROMOUSE] AVISO: APDS9960 FRONTAL nao respondeu no I2C!");
    }

    iniciarFiltro(sensorEsq.filtro,    lerDistanciaLateralMM(sensorEsq.vl, sensorEsq.ok));
    iniciarFiltro(sensorDir.filtro,    lerDistanciaLateralMM(sensorDir.vl, sensorDir.ok));
    iniciarFiltro(sensorFrente.filtro, lerProximidadeFrontal(sensorFrente.apds, sensorFrente.ok));
}

void atualizarSensores() {
    uint16_t bruteEsq    = lerDistanciaLateralMM(sensorEsq.vl, sensorEsq.ok);
    uint16_t bruteDir    = lerDistanciaLateralMM(sensorDir.vl, sensorDir.ok);
    uint16_t bruteFrente = lerProximidadeFrontal(sensorFrente.apds, sensorFrente.ok);

    sensorEsq.distanciaMM    = atualizarFiltro(sensorEsq.filtro, bruteEsq);
    sensorDir.distanciaMM    = atualizarFiltro(sensorDir.filtro, bruteDir);
    sensorFrente.proximidade = atualizarFiltro(sensorFrente.filtro, bruteFrente);

    sensorEsq.paredeDetectada = sensorEsq.paredeDetectada
        ? (sensorEsq.distanciaMM < DIST_LATERAL_LIVRE_MM)
        : (sensorEsq.distanciaMM < DIST_LATERAL_PAREDE_MM);

    sensorDir.paredeDetectada = sensorDir.paredeDetectada
        ? (sensorDir.distanciaMM < DIST_LATERAL_LIVRE_MM)
        : (sensorDir.distanciaMM < DIST_LATERAL_PAREDE_MM);

    sensorFrente.paredeDetectada = sensorFrente.paredeDetectada
        ? (sensorFrente.proximidade > PROX_FRONTAL_LIVRE)
        : (sensorFrente.proximidade > PROX_FRONTAL_PAREDE_DETECTADA);

    sensorEsq.muitoProxima    = sensorEsq.distanciaMM < DIST_LATERAL_MUITO_PROXIMA_MM;
    sensorDir.muitoProxima    = sensorDir.distanciaMM < DIST_LATERAL_MUITO_PROXIMA_MM;
    sensorFrente.muitoProxima = sensorFrente.proximidade > PROX_FRONTAL_MUITO_PROXIMA;
}

// Limpa (reinicia) o filtro de um sensor lateral/frontal com uma leitura nova.
void limparFiltroSensorLateral(SensorLateral &s) {
    uint16_t leitura = lerDistanciaLateralMM(s.vl, s.ok);
    iniciarFiltro(s.filtro, leitura);

    // Quando o robo esta parado para decidir curva, nao deixe a histerese antiga prender o estado.
    s.distanciaMM = leitura;
    s.paredeDetectada = leitura < DIST_LATERAL_PAREDE_MM;
    s.muitoProxima = leitura < DIST_LATERAL_MUITO_PROXIMA_MM;
}

void limparFiltroSensorFrontal(SensorFrontal &s) {
    uint16_t leitura = lerProximidadeFrontal(s.apds, s.ok);
    iniciarFiltro(s.filtro, leitura);

    // Reset sem memoria: depois de uma curva, a frente nova nao pode herdar "parede" da direcao antiga.
    s.proximidade = leitura;
    s.paredeDetectada = leitura > PROX_FRONTAL_PAREDE_DETECTADA;
    s.muitoProxima = leitura > PROX_FRONTAL_MUITO_PROXIMA;
}

void limparFiltrosSensores() {
    limparFiltroSensorLateral(sensorEsq);
    limparFiltroSensorLateral(sensorDir);
    limparFiltroSensorFrontal(sensorFrente);

    for (int i = 0; i < FILTER_SIZE_DIST; i++) {
        atualizarSensores();
        delay(SENSOR_UPDATE_TIME);
    }
}

extern ModoOperacao modoAtual;
extern int pwmBaseEsq;
extern int pwmBaseDir;

void analisarArredores() {
    limparFiltroSensorLateral(sensorEsq);
    limparFiltroSensorLateral(sensorDir);
    limparFiltroSensorFrontal(sensorFrente);

    unsigned long duracao = (modoAtual == MODO_MAPEAMENTO) ? TEMPO_ANALISE_ARREDORES_MS : STOP_TIME_SETTLE;
    unsigned long inicio = millis();

    while (millis() - inicio < duracao) {
        atualizarSensores();
        atualizarBateria();
        delay(SENSOR_UPDATE_TIME);
    }
}

uint32_t lerRGB() {
#if USE_RGB
    // TODO: implementar leitura real do sensor RGB
    return 0x000000;
#else
    return 0x000000;
#endif
}

class PositionQueue {
private:
    Position data[MAX_PATH_CAPACITY];
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
        if (tail < pathCapacityAtual()) {
            data[tail++] = p;
        }
    }

    Position pop() {
        return data[head++];
    }
};

class MazeMap {
private:
    Cell cells[MAX_MAZE_SIZE][MAX_MAZE_SIZE];

public:
    void begin() {
        for (uint8_t r = 0; r < mazeSize; r++) {
            for (uint8_t c = 0; c < mazeSize; c++) {
                cells[r][c] = {INF, 0, 0, 0, 0, 0};
            }
        }

        // Paredes externas
        for (uint8_t i = 0; i < mazeSize; i++) {
            cells[0][i].southWall = 1;
            cells[mazeSize - 1][i].northWall = 1;
            cells[i][0].westWall = 1;
            cells[i][mazeSize - 1].eastWall = 1;
        }
    }

    inline Cell& get(int8_t r, int8_t c) {
        return cells[r][c];
    }

    inline bool valid(int8_t r, int8_t c) const {
        return r >= 0 && r < mazeSize && c >= 0 && c < mazeSize;
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
        for (uint8_t r = 0; r < mazeSize; r++) {
            for (uint8_t c = 0; c < mazeSize; c++) {
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

class MotorController {
private:
    int pwmAtualEsq = 0;
    int pwmAtualDir = 0;

    bool leituraLateralUtil(const SensorLateral &s) const {
        return s.ok &&
               s.paredeDetectada &&
               s.distanciaMM >= DIST_LATERAL_MIN_CONFIAVEL_MM &&
               s.distanciaMM <= DIST_LATERAL_MAX_CORRECAO_MM;
    }

    int calcularCorrecaoLateral(long ticksAtual) {
        // Protecao imediata: se estiver quase raspando em uma parede, corrige mesmo perto da borda da celula.
        if (sensorEsq.ok && sensorEsq.distanciaMM < DIST_LATERAL_PERIGO_MM &&
            !(sensorDir.ok && sensorDir.distanciaMM < DIST_LATERAL_PERIGO_MM)) {
            return -PWM_CORRECAO_LATERAL_MAX; // muito perto da esquerda -> aponta para a direita
        }

        if (sensorDir.ok && sensorDir.distanciaMM < DIST_LATERAL_PERIGO_MM &&
            !(sensorEsq.ok && sensorEsq.distanciaMM < DIST_LATERAL_PERIGO_MM)) {
            return PWM_CORRECAO_LATERAL_MAX; // muito perto da direita -> aponta para a esquerda
        }

        // Evita corrigir em cima de intersecoes/aberturas, onde o VL53 pode enxergar o vazio lateral.
        if (ticksAtual < TICKS_IGNORAR_CORR_INICIO ||
            ticksAtual > (TICKS_PER_CELL - TICKS_IGNORAR_CORR_FIM)) {
            return 0;
        }

        bool esqValida = leituraLateralUtil(sensorEsq);
        bool dirValida = leituraLateralUtil(sensorDir);
        int erro;

        if (esqValida && dirValida) {
            erro = (int)sensorEsq.distanciaMM - (int)sensorDir.distanciaMM;
        } else if (esqValida && !dirValida) {
            erro = (int)sensorEsq.distanciaMM - (int)DIST_LATERAL_IDEAL_MM;
        } else if (!esqValida && dirValida) {
            erro = (int)DIST_LATERAL_IDEAL_MM - (int)sensorDir.distanciaMM;
        } else {
            return 0;
        }

        if (abs(erro) < ZONA_MORTA_CENTRALIZACAO_MM) {
            return 0;
        }

        int correcao = (int)((float)erro * KP_CENTRALIZACAO);
        return constrain(correcao, -PWM_CORRECAO_LATERAL_MAX, PWM_CORRECAO_LATERAL_MAX);
    }

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

        pwmAtualEsq = 0;
        pwmAtualDir = 0;
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

    void forwardComCorrecao(long ticksAtual) {
        long left;
        long right;

        noInterrupts();
        left = encoderLeftCount;
        right = encoderRightCount;
        interrupts();

        long diffReta = labs(left) - labs(right); // positivo = esquerda andou mais (puxa pra direita)
        int ajusteReta = (int)constrain((float)diffReta * GANHO_CORRECAO_RETA, -PWM_CORRECAO_MAX, PWM_CORRECAO_MAX);

        int ajusteLateral = calcularCorrecaoLateral(ticksAtual);

        int ajusteTotal = constrain(ajusteReta + ajusteLateral, -PWM_CORRECAO_TOTAL_MAX, PWM_CORRECAO_TOTAL_MAX);

        int pwmAlvoEsq = constrain(pwmBaseEsq - ajusteTotal, PWM_MOTOR_MIN, PWM_MOTOR_MAX);
        int pwmAlvoDir = constrain(pwmBaseDir + ajusteTotal, PWM_MOTOR_MIN, PWM_MOTOR_MAX);

        // Rampa: nunca deixa o PWM pular mais que RAMPA_PWM_MAX_PASSO por ciclo
        pwmAtualEsq += constrain(pwmAlvoEsq - pwmAtualEsq, -RAMPA_PWM_MAX_PASSO, RAMPA_PWM_MAX_PASSO);
        pwmAtualDir += constrain(pwmAlvoDir - pwmAtualDir, -RAMPA_PWM_MAX_PASSO, RAMPA_PWM_MAX_PASSO);

        motorEsquerdoFrente(pwmAtualEsq);
        motorDireitoFrente(pwmAtualDir);
    }

    void turnRight() {
        // Movimento físico para virar para a direita
        motorEsquerdoFrente(TURN_PWM_LEFT);
        motorDireitoTras(TURN_PWM_RIGHT);
    }

    void turnLeft() {
        // Movimento físico para virar para a esquerda
        motorEsquerdoTras(TURN_PWM_LEFT);
        motorDireitoFrente(TURN_PWM_RIGHT);
    }

    void turnBack() {
        // Giro de 180 usando o mesmo sentido da direita
        motorEsquerdoTras(TURN_PWM_LEFT);
        motorDireitoFrente(TURN_PWM_RIGHT);
    }

    void executarRe(int pwm, long ticksAlvo, unsigned long timeoutMs) {
        resetEncoders();

        unsigned long inicio = millis();
        unsigned long limiteSeguranca = inicio + timeoutMs;

        motorEsquerdoTras(pwm);
        motorDireitoTras(pwm);

        while (true) {
            if (lerMediaEncoders() >= ticksAlvo) {
                break;
            }

            if (millis() > limiteSeguranca) {
                // Segurança: encoder pode ter falhado, interrompe por tempo máximo
                break;
            }

            delay(SENSOR_UPDATE_TIME);
        }

        stop();

        Serial.print("[MICROMOUSE] Re executada. PWM=");
        Serial.print(pwm);
        Serial.print(" alvo=");
        Serial.print(ticksAlvo);
        Serial.print(" ticks, conseguiu=");
        Serial.print(lerMediaEncoders());
        Serial.println(" ticks");
    }

    void pulsoReAntesDeCurva() {
        atualizarSensores();
        // Nao de re em toda curva. Recuar fora do centro da celula foi uma das maiores fontes de erro.
        // A re curta so acontece se o sensor frontal indicar risco real de bater na parede antes do giro.
        if (sensorFrente.muitoProxima) {
            executarRe(REVERSE_PULSE_PWM, REVERSE_PULSE_TICKS, REVERSE_PULSE_TIMEOUT_MS);
        }
    }

    void pulsoReDestravar() {
        executarRe(MICRO_RE_PWM, MICRO_RE_TICKS, MICRO_RE_TIMEOUT_MS);
    }

    void pulsoReDestravarProgressivo(long ticksAlvo) {
        executarRe(MICRO_RE_PWM, ticksAlvo, MICRO_RE_TIMEOUT_PROG_MS);
    }

    void pararComLeitura(int tempoMs) {
        stop();
        delay(tempoMs);
    }

    bool andarUmaCelula() {
        limparFiltrosSensores();

        if (sensorFrente.paredeDetectada) {
            stop();
            return false;
        }

        resetEncoders();

        bool chegouNaCelula = false;
        long ticksAtual = 0;
        unsigned long inicio = millis();
        unsigned long timeout = (modoAtual == MODO_MAPEAMENTO) ? CELL_FORWARD_TIMEOUT_MS : (CELL_FORWARD_TIMEOUT_MS - 250);
        unsigned long limiteSeguranca = inicio + timeout;

        while (true) {
            atualizarSensores();
            ticksAtual = lerMediaEncoders();

            if (ticksAtual >= TICKS_PER_CELL) {
                chegouNaCelula = true;
                break;
            }

            if (sensorFrente.muitoProxima) {
                // Se a parede frontal apareceu so no final da celula, considera que chegou ao centro da nova celula.
                chegouNaCelula = ticksAtual >= TICKS_CELULA_OK_MIN;
                break;
            }

            if (millis() > limiteSeguranca) {
                // Se quase completou a celula, aceita; caso contrario nao atualiza a posicao logica.
                chegouNaCelula = ticksAtual >= TICKS_CELULA_OK_MIN;
                break;
            }

            forwardComCorrecao(ticksAtual);
            delay(SENSOR_UPDATE_TIME);
        }

        stop();

        long ticksFinais = lerMediaEncoders();
        if (!chegouNaCelula && ticksFinais >= TICKS_CELULA_OK_MIN) {
            chegouNaCelula = true;
        }

        if (!chegouNaCelula) {
            Serial.print("[MICROMOUSE] Avanco abortado antes de completar celula. ticks=");
            Serial.print(ticksFinais);
            Serial.print(" proxFrente=");
            Serial.println(sensorFrente.proximidade);
        }

        return chegouNaCelula;
    }

    void virarDireita90() {
        Serial.println("[MICROMOUSE] EXECUTANDO CURVA 90");
        pararComLeitura(STOP_TIME_SETTLE);

        turnRight();
        delay(TURN_TIME_90);

        stop();
    }

    void virarEsquerda90() {
        Serial.println("[MICROMOUSE] EXECUTANDO CURVA 90");
        pararComLeitura(STOP_TIME_SETTLE);

        turnLeft();
        delay(TURN_TIME_90);

        stop();
    }

    void virar180() {
        pararComLeitura(STOP_TIME_SETTLE);

        turnBack();
        delay(TURN_TIME_180);

        stop();
    }


    void virarDireita45() {
        Serial.println("[MICROMOUSE] EXECUTANDO CURVA 45");
        pararComLeitura(STOP_TIME_SETTLE);

        turnRight();
        delay(TURN_TIME_45);

        stop();
    }

    void virarEsquerda45() {
        Serial.println("[MICROMOUSE] EXECUTANDO CURVA 45");
        pararComLeitura(STOP_TIME_SETTLE);

        turnLeft();
        delay(TURN_TIME_45);

        stop();
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

// OBJETOS GLOBAIS
MazeMap maze;
FloodFillNavigator floodFill(&maze);
NavigationController navigator(&maze);
MotorController motors;

RobotState estadoAtual = DESLIGADO;
Position currentPos = {0, 0};
Position ultimaPosicaoAntesDaAtual = {-1, -1};
Direction currentDir = NORTH;

ModoOperacao modoAtual = MODO_MAPEAMENTO;
int pwmBaseEsq = PWM_LEFT_MAPEAMENTO;
int pwmBaseDir = PWM_RIGHT_MAPEAMENTO;

enum ObjetivoAtual : uint8_t { OBJETIVO_CENTRO, OBJETIVO_INICIO };
ObjetivoAtual objetivoAtual = OBJETIVO_CENTRO;
Position startGoal[1] = { {0, 0} };

bool atStart() {
    return currentPos.r == 0 && currentPos.c == 0;
}

uint8_t tentativasGiroAtual = 0;
bool robotTravado = false;

MoveCommand ultimoGiro90Preso = STOP_CMD;
uint8_t girosSemAvancoNaMesmaCelula = 0;
Position posUltimoGiroSemAvanco = {-1, -1};

Position centroGoals[4];

void atualizarCentroGoals() {
    uint8_t mid = mazeSize / 2;

    centroGoals[0] = {(int8_t)(mid - 1), (int8_t)(mid - 1)};
    centroGoals[1] = {(int8_t)(mid - 1), (int8_t)mid};
    centroGoals[2] = {(int8_t)mid,       (int8_t)(mid - 1)};
    centroGoals[3] = {(int8_t)mid,       (int8_t)mid};
}

void imprimirStatus() {
    Serial.print("POS: ");
    Serial.print(currentPos.r);
    Serial.print(",");
    Serial.print(currentPos.c);

    Serial.print(" | DIR: ");
    Serial.print(currentDir);

    Serial.print(" | ESQ: ");
    Serial.print(sensorEsq.distanciaMM);
    Serial.print("mm(");
    Serial.print(sensorEsq.paredeDetectada ? "PAREDE" : "LIVRE");
    Serial.print(")");

    Serial.print(" | FRENTE: ");
    Serial.print(sensorFrente.proximidade);
    Serial.print("prox(");
    Serial.print(sensorFrente.paredeDetectada ? "PAREDE" : "LIVRE");
    Serial.print(")");

    Serial.print(" | DIR_SENSOR: ");
    Serial.print(sensorDir.distanciaMM);
    Serial.print("mm(");
    Serial.print(sensorDir.paredeDetectada ? "PAREDE" : "LIVRE");
    Serial.print(")");

    // ========  nova alt (bateria): mostra bateria no status de debug  ========
    Serial.print(" | BAT: ");
    Serial.print(batTensaoAtual, 2);
    Serial.print("V (");
    Serial.print(batPercentAtual, 0);
    Serial.println("%)");
}

void aplicarLeituraParede(Position pos, AbsoluteWall parede, bool claramenteParede, bool claramenteLivre) {
    // V3: nao deixa uma leitura ruim criar uma parede eterna.
    // - Se esta claramente perto, marca parede.
    // - Se esta claramente longe/livre, apaga a parede.
    // - Se esta na faixa duvidosa, mantem o que o mapa ja sabia.
    if (claramenteParede) {
        maze.setWall(pos, parede, true);
    } else if (claramenteLivre) {
        maze.setWall(pos, parede, false);
    }
}

bool lateralClaramenteParede(const SensorLateral &sensor) {
    return sensor.ok && sensor.distanciaMM < DIST_LATERAL_PAREDE_MM;
}

bool lateralClaramenteLivre(const SensorLateral &sensor) {
    // V4: para APAGAR parede do mapa, exige uma leitura mais aberta que o
    // simples limiar de LIVRE. Um pico de 150..180 mm pode acontecer quando
    // o sensor esta torto em relacao a parede e nao deve apagar uma parede.
    return sensor.ok &&
           sensor.distanciaMM >= DIST_LATERAL_LIVRE_MAPA_MM &&
           sensor.distanciaMM < DIST_SEM_LEITURA_MM;
}

bool frenteClaramenteParede() {
    return sensorFrente.ok && sensorFrente.proximidade > PROX_FRONTAL_PAREDE_DETECTADA;
}

bool frenteClaramenteLivre() {
    return sensorFrente.ok && sensorFrente.proximidade < PROX_FRONTAL_LIVRE;
}

void updateWalls() {
    bool frontParede = frenteClaramenteParede();
    bool frontLivre  = frenteClaramenteLivre();
    bool rightParede = lateralClaramenteParede(sensorDir);
    bool rightLivre  = lateralClaramenteLivre(sensorDir);
    bool leftParede  = lateralClaramenteParede(sensorEsq);
    bool leftLivre   = lateralClaramenteLivre(sensorEsq);

    switch (currentDir) {
        case NORTH:
            aplicarLeituraParede(currentPos, WALL_NORTH, frontParede, frontLivre);
            aplicarLeituraParede(currentPos, WALL_EAST, rightParede, rightLivre);
            aplicarLeituraParede(currentPos, WALL_WEST, leftParede, leftLivre);
            break;

        case EAST:
            aplicarLeituraParede(currentPos, WALL_EAST, frontParede, frontLivre);
            aplicarLeituraParede(currentPos, WALL_SOUTH, rightParede, rightLivre);
            aplicarLeituraParede(currentPos, WALL_NORTH, leftParede, leftLivre);
            break;

        case SOUTH:
            aplicarLeituraParede(currentPos, WALL_SOUTH, frontParede, frontLivre);
            aplicarLeituraParede(currentPos, WALL_WEST, rightParede, rightLivre);
            aplicarLeituraParede(currentPos, WALL_EAST, leftParede, leftLivre);
            break;

        case WEST:
            aplicarLeituraParede(currentPos, WALL_WEST, frontParede, frontLivre);
            aplicarLeituraParede(currentPos, WALL_NORTH, rightParede, rightLivre);
            aplicarLeituraParede(currentPos, WALL_SOUTH, leftParede, leftLivre);
            break;
    }
}

void registrarPassagemLivreEntre(Position anterior, Position atual) {
    if (!maze.valid(anterior.r, anterior.c) || !maze.valid(atual.r, atual.c)) {
        return;
    }

    if (atual.r == anterior.r + 1 && atual.c == anterior.c) {
        maze.setWall(anterior, WALL_NORTH, false);
    } else if (atual.r == anterior.r - 1 && atual.c == anterior.c) {
        maze.setWall(anterior, WALL_SOUTH, false);
    } else if (atual.r == anterior.r && atual.c == anterior.c + 1) {
        maze.setWall(anterior, WALL_EAST, false);
    } else if (atual.r == anterior.r && atual.c == anterior.c - 1) {
        maze.setWall(anterior, WALL_WEST, false);
    }
}

void recomputarFloodFillObjetivo() {
    if (objetivoAtual == OBJETIVO_CENTRO) {
        floodFill.computeMulti(centroGoals, 4);
    } else {
        floodFill.computeMulti(startGoal, 1);
    }
}

Direction direcaoResultanteDoComando(Direction dir, MoveCommand cmd) {
    if (cmd == TURN_RIGHT_CMD) {
        return (Direction)((dir + 1) & 0x03);
    }

    if (cmd == TURN_LEFT_CMD) {
        return (Direction)((dir + 3) & 0x03);
    }

    if (cmd == TURN_BACK_CMD) {
        return (Direction)((dir + 2) & 0x03);
    }

    return dir;
}

Position vizinhoRelativo(Position pos, Direction dir, MoveCommand cmd) {
    Position destino = pos;
    Direction destinoDir = direcaoResultanteDoComando(dir, cmd);

    switch (destinoDir) {
        case NORTH: destino.r++; break;
        case EAST:  destino.c++; break;
        case SOUTH: destino.r--; break;
        case WEST:  destino.c--; break;
    }

    return destino;
}

bool mapaBloqueiaComando(MoveCommand cmd) {
    if (!maze.valid(currentPos.r, currentPos.c)) {
        return true;
    }

    Position destino = vizinhoRelativo(currentPos, currentDir, cmd);
    if (!maze.valid(destino.r, destino.c)) {
        return true;
    }

    Cell &cur = maze.get(currentPos.r, currentPos.c);
    Direction destinoDir = direcaoResultanteDoComando(currentDir, cmd);

    switch (destinoDir) {
        case NORTH: return cur.northWall;
        case EAST:  return cur.eastWall;
        case SOUTH: return cur.southWall;
        case WEST:  return cur.westWall;
    }

    return true;
}

uint8_t distFloodDoComando(MoveCommand cmd) {
    if (mapaBloqueiaComando(cmd)) {
        return INF;
    }

    Position destino = vizinhoRelativo(currentPos, currentDir, cmd);
    return maze.get(destino.r, destino.c).dist;
}

bool destinoDoComandoVisitado(MoveCommand cmd) {
    if (mapaBloqueiaComando(cmd)) {
        return true;
    }

    Position destino = vizinhoRelativo(currentPos, currentDir, cmd);
    return maze.get(destino.r, destino.c).visited;
}

bool lateralLivreParaCurva(const SensorLateral &sensor) {
    // Em curva, so chama de livre quando a leitura esta bem acima do limiar.
    // A faixa entre PAREDE e LIVRE fica como duvidosa, nao como parede.
    // Timeout/sem leitura nao e usado como prova de livre para nao virar em parede por falha de I2C.
    return lateralClaramenteLivre(sensor);
}

bool lateralBloqueadaParaCurva(const SensorLateral &sensor) {
    // V3: antes a faixa 120..150 mm era tratada como bloqueada e podia gerar 180/giro em loop.
    // Agora so bloqueia curva se realmente parecer parede.
    return lateralClaramenteParede(sensor);
}

struct ClassificacaoLateralDecisao {
    bool parede;
    bool livre;
    uint8_t validas;
    uint8_t amostrasParede;
    uint8_t amostrasLivre;
    uint16_t minimo;
    uint16_t maximo;
    uint16_t media;
};

struct ClassificacaoLateralDecisao classificarLateralParaDecisao(struct SensorLateral &sensor);
void imprimirClassificacaoLateral(const char *nome, const struct ClassificacaoLateralDecisao &cls);

struct ClassificacaoLateralDecisao classificarLateralParaDecisao(struct SensorLateral &sensor) {
    ClassificacaoLateralDecisao cls;
    cls.parede = false;
    cls.livre = false;
    cls.validas = 0;
    cls.amostrasParede = 0;
    cls.amostrasLivre = 0;
    cls.minimo = DIST_SEM_LEITURA_MM;
    cls.maximo = 0;
    cls.media = DIST_SEM_LEITURA_MM;

    if (!sensor.ok) {
        return cls;
    }

    uint32_t soma = 0;

    for (uint8_t i = 0; i < AMOSTRAS_DECISAO_CURVA; i++) {
        uint16_t mm = lerDistanciaLateralMM(sensor.vl, sensor.ok);

        if (mm < DIST_SEM_LEITURA_MM) {
            cls.validas++;
            soma += mm;

            if (mm < cls.minimo) cls.minimo = mm;
            if (mm > cls.maximo) cls.maximo = mm;

            if (mm < DIST_LATERAL_PAREDE_MM) {
                cls.amostrasParede++;
            }
            if (mm >= DIST_LATERAL_LIVRE_CURVA_MM) {
                cls.amostrasLivre++;
            }
        }

        delay(INTERVALO_AMOSTRA_DECISAO_MS);
    }

    if (cls.validas > 0) {
        cls.media = (uint16_t)(soma / cls.validas);
    }

    // Para bloquear curva, uma parede aparecendo repetidamente vale mais do
    // que um pico alto isolado. Para liberar curva, exige maioria bem livre.
    cls.parede = cls.validas >= MIN_AMOSTRAS_VALIDAS_CURVA &&
                 cls.amostrasParede >= MIN_AMOSTRAS_PAREDE_CURVA;

    cls.livre = cls.validas >= MIN_AMOSTRAS_VALIDAS_CURVA &&
                !cls.parede &&
                cls.amostrasLivre >= MIN_AMOSTRAS_LIVRE_CURVA &&
                cls.media >= DIST_LATERAL_LIVRE_CURVA_MM;

    return cls;
}

void imprimirClassificacaoLateral(const char *nome, const struct ClassificacaoLateralDecisao &cls) {
    Serial.print("[MICROMOUSE] Decisao ");
    Serial.print(nome);
    Serial.print(": media=");
    Serial.print(cls.media);
    Serial.print(" min=");
    Serial.print(cls.minimo);
    Serial.print(" max=");
    Serial.print(cls.maximo);
    Serial.print(" validas=");
    Serial.print(cls.validas);
    Serial.print(" parede=");
    Serial.print(cls.amostrasParede);
    Serial.print(" livre=");
    Serial.print(cls.amostrasLivre);
    Serial.print(" => ");
    Serial.println(cls.livre ? "LIVRE_ESTAVEL" : (cls.parede ? "PAREDE_ESTAVEL" : "DUVIDOSO"));
}

bool posicoesIguais(Position a, Position b) {
    return a.r == b.r && a.c == b.c;
}

bool destinoComandoDentro(MoveCommand cmd) {
    Position destino = vizinhoRelativo(currentPos, currentDir, cmd);
    return maze.valid(destino.r, destino.c);
}

AbsoluteWall paredeAbsolutaDoComando(MoveCommand cmd) {
    Direction destinoDir = direcaoResultanteDoComando(currentDir, cmd);

    switch (destinoDir) {
        case NORTH: return WALL_NORTH;
        case EAST:  return WALL_EAST;
        case SOUTH: return WALL_SOUTH;
        case WEST:  return WALL_WEST;
    }

    return WALL_NORTH;
}

void limparParedeDoComandoSeDestinoValido(MoveCommand cmd) {
    if (!destinoComandoDentro(cmd)) {
        return;
    }

    maze.setWall(currentPos, paredeAbsolutaDoComando(cmd), false);
}

bool comandoVoltaParaCelulaAnterior(MoveCommand cmd) {
    if (!maze.valid(ultimaPosicaoAntesDaAtual.r, ultimaPosicaoAntesDaAtual.c)) {
        return false;
    }

    Position destino = vizinhoRelativo(currentPos, currentDir, cmd);
    return posicoesIguais(destino, ultimaPosicaoAntesDaAtual);
}

MoveCommand escolherEntreLateraisLivres(MoveCommand cmdFlood, bool direitaLivre, bool esquerdaLivre) {
    if (direitaLivre && !esquerdaLivre) {
        return TURN_RIGHT_CMD;
    }

    if (esquerdaLivre && !direitaLivre) {
        return TURN_LEFT_CMD;
    }

    if (direitaLivre && esquerdaLivre) {
        if (modoAtual == MODO_MAPEAMENTO) {
            bool direitaVolta = comandoVoltaParaCelulaAnterior(TURN_RIGHT_CMD);
            bool esquerdaVolta = comandoVoltaParaCelulaAnterior(TURN_LEFT_CMD);

            // Se os dois lados parecem livres, nao escolha a celula de onde acabou de vir
            // a menos que o outro lado tambem volte. Isso reduz o comportamento de
            // "voltar, virar de novo e depois acertar".
            if (direitaVolta && !esquerdaVolta) {
                return TURN_LEFT_CMD;
            }
            if (esquerdaVolta && !direitaVolta) {
                return TURN_RIGHT_CMD;
            }

            bool direitaVisitada = destinoDoComandoVisitado(TURN_RIGHT_CMD);
            bool esquerdaVisitada = destinoDoComandoVisitado(TURN_LEFT_CMD);

            if (!direitaVisitada && esquerdaVisitada) {
                return TURN_RIGHT_CMD;
            }
            if (!esquerdaVisitada && direitaVisitada) {
                return TURN_LEFT_CMD;
            }
        }

        if (cmdFlood == TURN_RIGHT_CMD || cmdFlood == TURN_LEFT_CMD) {
            return cmdFlood;
        }

        uint8_t distDir = distFloodDoComando(TURN_RIGHT_CMD);
        uint8_t distEsq = distFloodDoComando(TURN_LEFT_CMD);
        return (distDir <= distEsq) ? TURN_RIGHT_CMD : TURN_LEFT_CMD;
    }

    return TURN_BACK_CMD;
}

MoveCommand escolherGiroSeguroPorSensores(MoveCommand cmdFlood) {
    ClassificacaoLateralDecisao clsDir = classificarLateralParaDecisao(sensorDir);
    ClassificacaoLateralDecisao clsEsq = classificarLateralParaDecisao(sensorEsq);

    imprimirClassificacaoLateral("DIR", clsDir);
    imprimirClassificacaoLateral("ESQ", clsEsq);

    bool direitaLivre = clsDir.livre;
    bool esquerdaLivre = clsEsq.livre;
    bool direitaBloqueada = clsDir.parede;
    bool esquerdaBloqueada = clsEsq.parede;

    // Se o sensor esta ESTAVELMENTE livre, ele pode corrigir uma parede falsa
    // que ficou gravada no mapa. Nao faz isso em borda externa do labirinto.
    if (direitaLivre && mapaBloqueiaComando(TURN_RIGHT_CMD) && destinoComandoDentro(TURN_RIGHT_CMD)) {
        Serial.println("[MICROMOUSE] Sensor direito livre estavel; apagando parede falsa do mapa.");
        limparParedeDoComandoSeDestinoValido(TURN_RIGHT_CMD);
    }
    if (esquerdaLivre && mapaBloqueiaComando(TURN_LEFT_CMD) && destinoComandoDentro(TURN_LEFT_CMD)) {
        Serial.println("[MICROMOUSE] Sensor esquerdo livre estavel; apagando parede falsa do mapa.");
        limparParedeDoComandoSeDestinoValido(TURN_LEFT_CMD);
    }

    if (mapaBloqueiaComando(TURN_RIGHT_CMD)) {
        direitaLivre = false;
        direitaBloqueada = true;
    }

    if (mapaBloqueiaComando(TURN_LEFT_CMD)) {
        esquerdaLivre = false;
        esquerdaBloqueada = true;
    }

    MoveCommand lateral = escolherEntreLateraisLivres(cmdFlood, direitaLivre, esquerdaLivre);
    if (lateral != TURN_BACK_CMD) {
        return lateral;
    }

    // V4: se nenhum lado ficou LIVRE_ESTAVEL, nao vira para uma parede por
    // chute. So usa um lado duvidoso quando o outro esta estavelmente bloqueado.
    if (direitaBloqueada && !esquerdaBloqueada && !mapaBloqueiaComando(TURN_LEFT_CMD)) {
        return TURN_LEFT_CMD;
    }

    if (esquerdaBloqueada && !direitaBloqueada && !mapaBloqueiaComando(TURN_RIGHT_CMD)) {
        return TURN_RIGHT_CMD;
    }

    // Se os dois lados ficaram duvidosos, ainda podemos escolher o lado que
    // esta muito mais aberto em media. Isso evita 180 desnecessario quando a
    // abertura real nao passou por pouco no criterio LIVRE_ESTAVEL.
    if (!direitaBloqueada && !esquerdaBloqueada) {
        if (clsDir.validas >= MIN_AMOSTRAS_VALIDAS_CURVA &&
            clsEsq.validas >= MIN_AMOSTRAS_VALIDAS_CURVA) {
            int diffMedia = (int)clsDir.media - (int)clsEsq.media;

            if (diffMedia >= MARGEM_MEDIA_ESCOLHA_DUVIDOSA_MM && !mapaBloqueiaComando(TURN_RIGHT_CMD)) {
                Serial.println("[MICROMOUSE] Laterais duvidosas; escolhendo DIREITA por media bem maior.");
                return TURN_RIGHT_CMD;
            }
            if (diffMedia <= -MARGEM_MEDIA_ESCOLHA_DUVIDOSA_MM && !mapaBloqueiaComando(TURN_LEFT_CMD)) {
                Serial.println("[MICROMOUSE] Laterais duvidosas; escolhendo ESQUERDA por media bem maior.");
                return TURN_LEFT_CMD;
            }
        }
    }

    // Se a frente estiver livre, e mais seguro seguir reto do que dar 180 por
    // uma incerteza lateral. O flood fill recalcula na proxima celula.
    if (!sensorFrente.paredeDetectada && !sensorFrente.muitoProxima && !mapaBloqueiaComando(MOVE_FORWARD)) {
        return MOVE_FORWARD;
    }

    return TURN_BACK_CMD;
}

bool comandoBateEmParedePelosSensores(MoveCommand cmd) {
    if (cmd == MOVE_FORWARD) {
        return sensorFrente.paredeDetectada || sensorFrente.muitoProxima || mapaBloqueiaComando(cmd);
    }

    if (cmd == TURN_RIGHT_CMD) {
        return lateralBloqueadaParaCurva(sensorDir) || mapaBloqueiaComando(cmd);
    }

    if (cmd == TURN_LEFT_CMD) {
        return lateralBloqueadaParaCurva(sensorEsq) || mapaBloqueiaComando(cmd);
    }

    return false;
}

void revalidarSensoresPorTempo(unsigned long duracaoMs) {
    motors.stop();
    limparFiltrosSensores();

    unsigned long inicio = millis();
    while (millis() - inicio < duracaoMs) {
        atualizarSensores();
        atualizarBateria();
        delay(SENSOR_UPDATE_TIME);
    }
}

void revalidarSensoresParaCurva() {
    revalidarSensoresPorTempo(TEMPO_RELEITURA_CURVA_MS);
}

void revalidarSensoresAposGiro() {
    revalidarSensoresPorTempo(TEMPO_RELEITURA_APOS_GIRO_MS);
}

bool mesmaPosicao(Position a, Position b) {
    return a.r == b.r && a.c == b.c;
}

void registrarGiroSemAvanco() {
    if (mesmaPosicao(posUltimoGiroSemAvanco, currentPos)) {
        girosSemAvancoNaMesmaCelula++;
    } else {
        posUltimoGiroSemAvanco = currentPos;
        girosSemAvancoNaMesmaCelula = 1;
    }

    Serial.print("[MICROMOUSE] Giro sem avanco na mesma celula: ");
    Serial.println(girosSemAvancoNaMesmaCelula);
}

void limparControleGiroSemAvanco() {
    girosSemAvancoNaMesmaCelula = 0;
    posUltimoGiroSemAvanco = {-1, -1};
}

MoveCommand corrigirComandoPorSensores(MoveCommand cmdFlood) {
    bool frenteBloqueada = sensorFrente.paredeDetectada || sensorFrente.muitoProxima;

    // Em mapeamento, evita dar meia-volta se ainda existe uma frente fisicamente livre.
    // Isso reduz retornos prematuros causados por uma parede falsa gravada no mapa.
    if (modoAtual == MODO_MAPEAMENTO && cmdFlood == TURN_BACK_CMD &&
        !frenteBloqueada && !mapaBloqueiaComando(MOVE_FORWARD)) {
        Serial.println("[MICROMOUSE] Flood queria 180, mas frente esta livre. Explorando frente.");
        return MOVE_FORWARD;
    }

    // Se o flood quer virar, a lateral precisa estar livre de forma estavel.
    // Caso contrario, preferimos seguir reto se for seguro, ou escolher outro giro seguro.
    if (cmdFlood == TURN_RIGHT_CMD || cmdFlood == TURN_LEFT_CMD) {
        SensorLateral &sensorAlvo = (cmdFlood == TURN_RIGHT_CMD) ? sensorDir : sensorEsq;
        ClassificacaoLateralDecisao clsAlvo = classificarLateralParaDecisao(sensorAlvo);
        imprimirClassificacaoLateral(cmdFlood == TURN_RIGHT_CMD ? "ALVO_DIR" : "ALVO_ESQ", clsAlvo);

        if (clsAlvo.livre && mapaBloqueiaComando(cmdFlood) && destinoComandoDentro(cmdFlood)) {
            limparParedeDoComandoSeDestinoValido(cmdFlood);
        }

        bool curvaSegura = clsAlvo.livre && !mapaBloqueiaComando(cmdFlood);
        bool curvaBloqueada = clsAlvo.parede || mapaBloqueiaComando(cmdFlood);

        if (!curvaSegura || curvaBloqueada) {
            if (!frenteBloqueada && !mapaBloqueiaComando(MOVE_FORWARD)) {
                Serial.println("[MICROMOUSE] Curva lateral nao ficou livre estavel. Seguindo frente por seguranca.");
                return MOVE_FORWARD;
            }

            return escolherGiroSeguroPorSensores(cmdFlood);
        }
    }

    if (frenteBloqueada) {
        // Regra principal: com parede na frente, a escolha da curva vem da
        // classificacao estavel dos VL53L0X, nao de uma unica leitura.
        if (cmdFlood == MOVE_FORWARD || cmdFlood == TURN_BACK_CMD || comandoBateEmParedePelosSensores(cmdFlood)) {
            return escolherGiroSeguroPorSensores(cmdFlood);
        }
    }

    if (comandoBateEmParedePelosSensores(cmdFlood)) {
        return escolherGiroSeguroPorSensores(cmdFlood);
    }

    return cmdFlood;
}

const char *nomeComando(MoveCommand cmd) {
    switch (cmd) {
        case MOVE_FORWARD:   return "FRENTE";
        case TURN_LEFT_CMD:  return "ESQUERDA";
        case TURN_RIGHT_CMD: return "DIREITA";
        case TURN_BACK_CMD:  return "180";
        case STOP_CMD:       return "PARAR";
    }
    return "?";
}

void updatePosition(MoveCommand cmd, bool avancou) {
    switch (cmd) {
        case TURN_LEFT_CMD:
            currentDir = (Direction)((currentDir + 3) & 0x03);
            return;   // giro: so direcao, nunca posicao

        case TURN_RIGHT_CMD:
            currentDir = (Direction)((currentDir + 1) & 0x03);
            return;   // giro: so direcao, nunca posicao

        case TURN_BACK_CMD:
            currentDir = (Direction)((currentDir + 2) & 0x03);
            return;   // 180: so inverte direcao; o recuo vem no proximo avanco

        case STOP_CMD:
            return;   // parado: nada muda

        case MOVE_FORWARD:
        default:
            break;    // segue abaixo pra mover a celula, se avancou
    }

    // A partir daqui, so MOVE_FORWARD. So move a celula se realmente andou.
    if (!avancou) {
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

    currentPos.r = constrain(currentPos.r, 0, mazeSize - 1);
    currentPos.c = constrain(currentPos.c, 0, mazeSize - 1);
}

void publicarTelemetria() {
    JsonDocument doc;
    preencherPayloadWeb(doc);

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    webSocket.broadcastTXT(jsonPayload);
}

// VERIFICAÇÃO DE CHEGADA
bool atCenter() {
    uint8_t mid = mazeSize / 2;

    return (currentPos.r == mid - 1 || currentPos.r == mid) &&
           (currentPos.c == mid - 1 || currentPos.c == mid);
}

// COMANDOS RECEBIDOS DO SITE
void configurarNovaTentativa(uint8_t novoMazeSize) {
    if (!mazeSizeValido(novoMazeSize)) {
        Serial.println("[MICROMOUSE] Maze size invalido. Use 4, 8 ou 16.");
        return;
    }

    motors.stop();

    mazeSize = novoMazeSize;

    currentPos = {0, 0};
    ultimaPosicaoAntesDaAtual = {-1, -1};
    currentDir = NORTH;

    modoAtual = MODO_MAPEAMENTO;
    pwmBaseEsq = PWM_LEFT_MAPEAMENTO;
    pwmBaseDir = PWM_RIGHT_MAPEAMENTO;

    objetivoAtual = OBJETIVO_CENTRO;

    caminhoPercorridoSize = 0;
    desafioCumprido = false;
    tempoConclusaoISO = "";
    robotTravado = false;
    tentativasGiroAtual = 0;
    ultimoGiro90Preso = STOP_CMD;   // ========  nova alt (anti dois-90): zera ao iniciar tentativa  ========
    girosSemAvancoNaMesmaCelula = 0;
    posUltimoGiroSemAvanco = {-1, -1};
    comandoIniciarCorrida = false;

    numeroTentativa++;
    inicioCorrida = millis();

    maze.begin();
    inicializarMapaWeb();
    atualizarCentroGoals();

    registrarCaminho(currentPos.r, currentPos.c);
    marcarCelulaVisitada(currentPos.r, currentPos.c);

    limparFiltrosSensores();
    analisarArredores();

    floodFill.computeMulti(centroGoals, 4);

    estadoAtual = MOVIMENTO_FRENTE;

    Serial.print("[MICROMOUSE] Nova tentativa iniciada. Maze size: ");
    Serial.print(mazeSize);
    Serial.println("x");
}

void processarComandosSite() {
    if (comandoPararRato) {
        comandoPararRato = false;
        comandoIniciarMapeamento = false;
        comandoIniciarCorrida = false;

        motors.stop();
        estadoAtual = AGUARDANDO_INICIO;

        Serial.println("[MICROMOUSE] STOP recebido do site. Rato parado, aguardando novo START.");
    }

    if (comandoIniciarMapeamento) {
        comandoIniciarMapeamento = false;
        configurarNovaTentativa(comandoMazeSize);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    randomSeed((uint32_t)millis());

    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    initEncoders();

    initBateriaINA226();


    iniciarSensoresDistancia();

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

    WiFi.mode(WIFI_AP);
    delay(100);

    bool apCriado = WiFi.softAP(ssid, password);

    if (apCriado) {
        Serial.println("[MICROMOUSE] Access Point criado com sucesso.");
    } else {
        Serial.println("[MICROMOUSE] ERRO: falha ao criar o Access Point! Verifique a senha (minimo 8 caracteres) e tente reiniciar a ESP32.");
    }

    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("IP da ESP32: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("MAC do AP: ");
    Serial.println(WiFi.softAPmacAddress());

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    Serial.println("[MICROMOUSE] Hardware inicializado.");
    Serial.print("[MICROMOUSE] TICKS_PER_CELL calculado: ");
    Serial.println(TICKS_PER_CELL);
    Serial.println("[MICROMOUSE] Aguardando START do site...");

    atualizarCentroGoals();
    analisarArredores();
    floodFill.computeMulti(centroGoals, 4);

    estadoAtual = AUTO_INICIAR_SEM_SITE ? MOVIMENTO_FRENTE : AGUARDANDO_INICIO;

    Serial.println("[MICROMOUSE] Pronto. Aguardando START do site...");
}

void loop() {
    if (estadoAtual == FINALIZADO) {
        webSocket.loop();
        processarComandosSite();
        atualizarBateria();   // ========  nova alt (bateria)  ========
        publicarTelemetria();
        delay(200);
        return;
    }

    webSocket.loop();
    processarComandosSite();
    atualizarBateria();       // ========  nova alt (bateria): sempre atualiza  ========

    if (estadoAtual == AGUARDANDO_INICIO) {
        motors.stop();
        publicarTelemetria();
        delay(200);
        return;
    }


    if (estadoAtual == MAPEAMENTO_CONCLUIDO) {
        webSocket.loop();
        motors.stop();

        if (comandoIniciarCorrida) {
            comandoIniciarCorrida = false;

            Serial.println("[MICROMOUSE] Comando recebido! Iniciando modo CORRIDA, voltando ao inicio...");

            modoAtual = MODO_CORRIDA;
            pwmBaseEsq = PWM_LEFT_CORRIDA;
            pwmBaseDir = PWM_RIGHT_CORRIDA;
            objetivoAtual = OBJETIVO_INICIO;
            estadoAtual = MOVIMENTO_FRENTE;
        }

        publicarTelemetria();
        delay(200);
        return;
    }

    if (robotTravado) {
        webSocket.loop();
        motors.stop();
        publicarTelemetria();
        delay(200);
        return;
    }

    webSocket.loop();
    analisarArredores();
    imprimirStatus();

    uint32_t corRGB = lerRGB();

    updateWalls();

    recomputarFloodFillObjetivo();

    bool chegouCentro = atCenter() || corRGB == FINISH_COLOR;
    bool chegouInicio = atStart();

    if (objetivoAtual == OBJETIVO_CENTRO && chegouCentro) {
        if (modoAtual == MODO_MAPEAMENTO) {
            Serial.println("[MICROMOUSE] Mapeamento concluido! Parado, aguardando comando do site...");
            estadoAtual = MAPEAMENTO_CONCLUIDO;
            motors.stop();
        } else {
            // Já estava em modo CORRIDA e chegou ao centro de novo —
            // corrida final concluída.
            desafioCumprido = true;
            tempoConclusaoISO = timestampISO(millis() - inicioCorrida);
            estadoAtual = FINALIZADO;
            motors.stop();
            Serial.println("[MICROMOUSE] CORRIDA FINAL CONCLUIDA! Encerrando...");
        }
    } else if (objetivoAtual == OBJETIVO_INICIO && chegouInicio) {
        Serial.println("[MICROMOUSE] De volta ao inicio! Iniciando corrida final ate o centro...");
        objetivoAtual = OBJETIVO_CENTRO;
    }

    if (estadoAtual == MOVIMENTO_FRENTE && !robotTravado) {
        recomputarFloodFillObjetivo();
        MoveCommand cmdFlood = navigator.decide(currentPos, currentDir);

        // Quando a frente esta bloqueada, para e rele os VL53L0X antes de escolher lado.
        // Isso reduz leituras erradas causadas por o rato estar torto na entrada da celula.
        if (sensorFrente.paredeDetectada || sensorFrente.muitoProxima) {
            revalidarSensoresParaCurva();
            updateWalls();
            recomputarFloodFillObjetivo();
            cmdFlood = navigator.decide(currentPos, currentDir);
        }

        MoveCommand cmd = corrigirComandoPorSensores(cmdFlood);

        if (cmd != cmdFlood) {
            Serial.print("[MICROMOUSE] Comando corrigido pelos sensores: ");
            Serial.print(nomeComando(cmdFlood));
            Serial.print(" -> ");
            Serial.println(nomeComando(cmd));
        }

        bool comandoEhGiro = (cmd == TURN_LEFT_CMD || cmd == TURN_RIGHT_CMD || cmd == TURN_BACK_CMD);

        Position posicaoAnterior = currentPos;
        bool avancou = false;

        if (cmd == MOVE_FORWARD) {
            avancou = motors.execute(MOVE_FORWARD);
            updatePosition(MOVE_FORWARD, avancou);
        } else if (comandoEhGiro) {
            // V3: giro nao pode ser uma acao final. Depois de girar, o rato deve tentar sair da celula.
            // Isso elimina o loop "gira, para, redecide, gira de novo" na mesma posicao logica.
            motors.pulsoReAntesDeCurva();
            motors.execute(cmd);
            updatePosition(cmd, false);  // atualiza somente a direcao

            revalidarSensoresAposGiro();
            updateWalls();
            recomputarFloodFillObjetivo();

            bool frenteLivreDepoisDoGiro = !sensorFrente.paredeDetectada &&
                                           !sensorFrente.muitoProxima &&
                                           !mapaBloqueiaComando(MOVE_FORWARD);

            if (frenteLivreDepoisDoGiro) {
                Serial.println("[MICROMOUSE] Curva concluida; frente livre. Avancando na mesma decisao.");
                posicaoAnterior = currentPos;
                avancou = motors.execute(MOVE_FORWARD);
                updatePosition(MOVE_FORWARD, avancou);
            } else {
                Serial.println("[MICROMOUSE] Curva terminou sem frente livre. Marcando parede e reavaliando sem girar em loop.");
                updateWalls();
                registrarGiroSemAvanco();
            }
        }

        if (avancou) {
            ultimaPosicaoAntesDaAtual = posicaoAnterior;
            registrarPassagemLivreEntre(posicaoAnterior, currentPos);
            atualizarMapaMovimento(posicaoAnterior, currentPos);
            tentativasGiroAtual = 0;
            ultimoGiro90Preso = STOP_CMD;
            limparControleGiroSemAvanco();
        } else {
            // Nao atualiza posicao se nao completou a celula. Marca paredes de novo e tenta redecidir no proximo loop.
            updateWalls();
            tentativasGiroAtual++;

            if (girosSemAvancoNaMesmaCelula >= MAX_GIROS_SEM_AVANCO_MESMA_CELULA) {
                Serial.println("[MICROMOUSE] Anti-loop: muitos giros sem sair da mesma celula. Tentando re curta e nova leitura.");
                motors.pulsoReDestravarProgressivo(MICRO_RE_TICKS_BASE);
                revalidarSensoresParaCurva();
                updateWalls();

                if (!sensorFrente.paredeDetectada && !sensorFrente.muitoProxima && !mapaBloqueiaComando(MOVE_FORWARD)) {
                    Position posAntesRecuperacao = currentPos;
                    bool avancouRecuperacao = motors.execute(MOVE_FORWARD);
                    updatePosition(MOVE_FORWARD, avancouRecuperacao);

                    if (avancouRecuperacao) {
                        ultimaPosicaoAntesDaAtual = posAntesRecuperacao;
                        registrarPassagemLivreEntre(posAntesRecuperacao, currentPos);
                        atualizarMapaMovimento(posAntesRecuperacao, currentPos);
                        tentativasGiroAtual = 0;
                        ultimoGiro90Preso = STOP_CMD;
                        limparControleGiroSemAvanco();
                        avancou = true;
                    }
                }
            }

            if (!avancou && tentativasGiroAtual >= MIN_TENTATIVAS_GIRO) {
                robotTravado = true;
                motors.stop();
                Serial.println("[MICROMOUSE] TRAVAMENTO DETECTADO - varias tentativas sem completar celula.");
            }
        }

        if (maze.valid(currentPos.r, currentPos.c)) {
            maze.get(currentPos.r, currentPos.c).visited = 1;
        }
    }

    publicarTelemetria();
    delay(20);
}


