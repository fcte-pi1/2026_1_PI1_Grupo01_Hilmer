// =============================================================================
// MICROMOUSE UNIFICADO — ESP32
// Flood Fill + Sensores filtrados + PWM separado por motor + ENCODERS
// + BATERIA REAL via INA226 (I2C) transmitida por WiFi/WebSocket
//
// >>> NESTA VERSAO <<<
// A leitura de bateria deixou de ser fixa (100% / 7.4V hardcoded) e passou
// a vir da INA226 (U12 no esquematico) via I2C. Os valores reais sao
// enviados por WiFi/WebSocket usando EXATAMENTE os mesmos nomes de campo
// que o site (Dashboard.jsx) ja le: batteryPercent, percentualBateria,
// tensaoEletrica, correnteEletrica, tensaoAtual, bateriaAtual. O site NAO
// precisa de nenhuma alteracao.
//
// Biblioteca usada: "INA226" do Rob Tillaart (instale pelo Gerenciador de
// Bibliotecas do Arduino IDE: procure por "INA226 Tillaart").
//
// Versão com encoders, pulsos de andamento e 2 modos (mapeamento/corrida):
// - Usa encoders esquerdo/direito para controlar a distância percorrida
//   por célula (em vez de depender só de tempo).
// - Parada IMEDIATA (sem delay) ao detectar parede na frente, evitando impacto.
// - Pulso de ré (1/5 da célula) antes de TODA curva (90 ou 180), medido
//   por encoder, pra facilitar a manobra.
// - Confirmação de sensores pós-curva: espera até 1seg (sai mais cedo se
//   estabilizar) verificando se a parede que motivou o giro ainda existe.
// - Detecção de travamento em DUAS ETAPAS: depois de N tentativas de giro
//   sem avançar, dá uma ré bem pequena pra tentar desempacar; se ainda
//   assim não avançar até o total de tentativas, declara travamento e para.
// - PULSO DE ANDAMENTO: anda 1 célula, PARA DE VEZ, lê os sensores com
//   calma, só então decide o próximo passo.
// - Tamanho do labirinto configurável pelo site via WebSocket (4x4 / 8x8 / 16x16).
//   O firmware reserva memória para o máximo 16x16 e usa mazeSize em tempo de execução.
// - Dois modos: MAPEAMENTO (devagar, descobrindo o labirinto) e CORRIDA
//   (mais rápido, usando o mapa já descoberto). O robô MAPEIA até achar
//   o centro e PARA DE VEZ — não troca de modo sozinho. A CORRIDA só
//   começa quando o site manda o comando "INICIAR_CORRIDA" via WebSocket;
//   aí ele volta ao início (rápido, usando o mapa) e faz a corrida final
//   de volta ao centro.
//
// >>> NOVA FEATURE NESTA VERSAO (curva de 45 graus pos-desempaque) <<<
// Quando o robo trava numa quina e faz a mini-re de desempacar, muitas
// vezes essa re ja realinha o chassi. Fazer um 90 cheio logo em seguida
// "passa do ponto" e ele bate de novo. Entao, SO nessa situacao especifica
// (logo apos a mini-re de desempaque E quando o giro seria um 90 pra
// esquerda ou direita), o robo agora gira apenas 45 graus (TURN_TIME_45),
// ajustavel por teste. Nada mais foi alterado: giros normais de 90/180 e
// todo o resto seguem exatamente iguais.
// =============================================================================

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <stdint.h>
#include <math.h>

// ========
// nova alt (bateria): includes da INA226 (I2C)
// ========
#include <Wire.h>
#include <INA226.h>

// ========
// nova alt: declaração antecipada (forward declaration) dos tipos
// customizados (structs/enums). O Arduino IDE gera automaticamente os
// prototypes de todas as funções do sketch e insere isso logo no topo do
// arquivo, ANTES das structs serem definidas mais abaixo. Sem essas
// declarações aqui, o compilador não reconhece tipos como "SensorParede"
// nos prototypes automáticos e gera erro de "declared void" / "was not
// declared in this scope". Isso não muda nenhum comportamento do robô,
// só corrige a ordem de compilação.
// ========
struct Position;
struct Cell;
struct TelemetryPoint;
struct SensorParede;
enum Direction : uint8_t;
enum MoveCommand : uint8_t;
enum AbsoluteWall : uint8_t;
enum RobotState : uint8_t;

// ========
// nova alt: enum completo aqui em cima (não só o tipo) porque
// MotorController usa o VALOR "MODO_MAPEAMENTO" diretamente, não só o
// tipo — então precisa estar definido antes da classe usar.
// ========
enum ModoOperacao : uint8_t { MODO_MAPEAMENTO, MODO_CORRIDA };
class MazeMap;
class PositionQueue;
class FloodFillNavigator;
class MotorController;
class NavigationController;

// =============================================================================
// ========================================================================
// PARÂMETROS PARA AJUSTE DURANTE OS TESTES
// (tudo que normalmente precisa ser calibrado está concentrado aqui)
// ========================================================================
//
// NOTAS GERAIS DO PROJETO (não mexer, só referência física):
//   a. Rato: ~269g, 2 rodas motrizes, controlado por ESP32 (Arduino IDE).
//      Dimensões do chassi: 11.2 x 10 cm.
//   b. Cada célula do labirinto: 18 x 18 cm.
//   c. Alterações deste arquivo se limitam ao que afeta a movimentação
//      básica (andar, parar, virar, detectar travamento). Telemetria/Wi-Fi/
//      JSON para o computador não foram alteradas na lógica, só os pontos
//      necessários para continuar funcionando com o novo fluxo.
//   d. Pinos dos encoders confirmados em teste isolado (3 portas conhecidas
//      + 1 encontrada): Encoder esquerdo = GPIO 16 e 17 (16 foi a porta
//      "nova" encontrada). Encoder direito = GPIO 18 e 19 (já confirmados).
//      Motores = 25, 26, 32, 33. Sensores de parede = 34, 36, 39.
//
// -----------------------------------------------------------------------
// PWM de movimento reto e curva
// -----------------------------------------------------------------------
#define PWM_LEFT             74     // PWM motor esquerdo andando reto
#define PWM_RIGHT            76     // PWM motor direito andando reto
#define TURN_PWM_LEFT         73    // PWM motor esquerdo nas curvas de 90/180
#define TURN_PWM_RIGHT         75    // PWM motor direito nas curvas de 90/180

// -----------------------------------------------------------------------
// Tempos de curva (ainda baseados em tempo, não em encoder)
// -----------------------------------------------------------------------
#define TURN_TIME_90          400    // ms para girar 90 graus
#define TURN_TIME_180         855    // ms para girar 180 graus

// ========
// nova alt (curva 45): tempo pra girar 45 graus, usado SOMENTE na curva
// de desempaque (logo apos a mini-re de destravamento). Ajuste por teste.
// Comeca em ~metade do 90 (425/2 ≈ 210), mas o valor real depende de
// atrito/bateria — calibre olhando o rato. NAO afeta os giros normais.
// ========
#define TURN_TIME_45          90    // ms para girar 45 graus (curva de desempaque)

// -----------------------------------------------------------------------
// ========
// nova alt: parâmetros do pulso de ré antes de curvas em quina
// ========
// -----------------------------------------------------------------------
#define REVERSE_PULSE_PWM       70    // PWM do pulso de ré (mesmo para os 2 motores) — AUMENTADO (10 era baixo demais pra vencer o atrito estático do motor, ré não movia de verdade)


#define AUTO_INICIAR_SEM_SITE  true   // true = liga e já começa a mapear sozinho (teste standalone) / false = espera START do site

// ========
// nova alt: a ré antes da curva agora é medida por encoder, como fração
// da distância de andar 1 célula. Pedido: 1/5 da célula, em TODA curva
// (90 OU 180), não só nas quinas — facilita a manobra de qualquer giro.
// ========
#define REVERSE_PULSE_FRACTION    3    // a ré é 1/(esse numero) da distancia de 1 celula. 5 = 1/5
#define REVERSE_PULSE_TICKS      (TICKS_PER_CELL / REVERSE_PULSE_FRACTION)
#define REVERSE_PULSE_TIMEOUT_MS 200   // tempo MAXIMO de seguranca da re, caso o encoder falhe

// ========
// nova alt: ré BEM pequena de "desempacar" — usada quando o robô fica
// girando repetidamente sem avançar (ver TENTATIVAS_PARA_RE_DESTRAVAR
// mais abaixo). É bem menor que a ré normal de curva, só o suficiente
// pra tentar soltar o chassi de uma quina onde ficou preso fisicamente.
// ========
#define MICRO_RE_PWM              70    // PWM da re pequena de desempacar — AUMENTADO (40 podia nao ser suficiente pra vencer o atrito estatico)
#define MICRO_RE_FRACTION         4    // fracao da celula pra re de desempacar (bem pequena: 1/15)
#define MICRO_RE_TICKS            (TICKS_PER_CELL / MICRO_RE_FRACTION)
#define MICRO_RE_TIMEOUT_MS       280   // tempo MAXIMO de seguranca dessa re pequena — AUMENTADO (120 podia ser curto demais pra sair da inercia)

// ========
// nova alt: ANÁLISE DE ARREDORES UNIFICADA. Antes, cada tipo de
// movimento (andar/virar) tinha sua própria pausa de leitura espalhada
// pelo código (limparFiltrosSensores, pararComLeitura, confirmarSensores
// AposGiro...). Agora existe um único momento de "parar e analisar",
// chamado sempre no topo do loop principal, depois de qualquer movimento
// (andar OU virar). No modo MAPEAMENTO isso demora 1 segundo inteiro,
// bem devagar, pra dar tempo de entender os arredores com calma antes de
// decidir o próximo passo. No modo CORRIDA, como já se confia no mapa
// conhecido, essa pausa é bem mais curta (usa STOP_TIME_SETTLE).
// ========
#define TEMPO_ANALISE_ARREDORES_MS   1500  // mapeamento: parado 1s lendo os sensores com calma (pedido)

// -----------------------------------------------------------------------
// ========
// nova alt (bateria): parametros da INA226 e da bateria 2S (~7.4V)
// ========
// A INA226 e um sensor DIGITAL I2C. GPIO21 = SDA, GPIO22 = SCL (confirme
// o SCL na sua fiacao). A tensao de barramento (getBusVoltage) e o que
// vira "% de bateria". Shunt/corrente dependem da calibracao abaixo.
// -----------------------------------------------------------------------
#define I2C_SDA_PIN        21        // SDA da INA226 (confirmado no esquematico)
#define I2C_SCL_PIN        22        // SCL da INA226 — CONFIRME na sua fiacao
#define INA226_ENDERECO    0x40      // endereco I2C padrao do modulo (mude se o scanner apontar outro)
#define INA226_SHUNT_OHMS  0.1       // resistor shunt do modulo (tipico 0.1 ohm)
#define INA226_CORRENTE_MAX_A  2.0   // corrente maxima esperada (para calibracao)
#define BAT_TENSAO_MAX     8.4       // 2S Li-ion/LiPo carregada (~8.4V) = 100%
#define BAT_TENSAO_MIN     6.0       // corte de descarga aproximado (~6.0V) = 0%
#define BAT_UPDATE_MS      500       // intervalo de leitura da bateria (ms)

// -----------------------------------------------------------------------
// ========
// nova alt: parâmetros de movimentação por ENCODER
// ========
// Ajuste WHEEL_DIAMETER_CM e ENCODER_PPR conforme medição real da roda e
// datasheet/teste do encoder. TICKS_PER_CELL é calculado a partir deles,
// mas pode ser sobrescrito manualmente (fixando um valor numérico) caso a
// calibração por fórmula não bata com o resultado prático.
// -----------------------------------------------------------------------
#define CELL_SIZE_CM            18.0   // tamanho de uma célula do labirinto (cm) — NÃO MEXER (fixo pela regra)
#define WHEEL_DIAMETER_CM        4.2   // diâmetro da roda em cm — AJUSTAR conforme medição real
#define ENCODER_PPR               15   // pulsos por volta do encoder — AJUSTAR conforme datasheet/teste

// Calculado automaticamente a partir dos dois valores acima — mantido como
// referência/comentário. O valor abaixo (TICKS_PER_CELL) foi SUBSTITUÍDO
// pelo valor medido fisicamente com o teste_calibracao_encoders.ino, que é
// muito mais confiável do que a fórmula com diâmetro/PPR chutados.
// #define TICKS_PER_CELL  ((uint16_t)((CELL_SIZE_CM / (PI * WHEEL_DIAMETER_CM)) * ENCODER_PPR))

// ========
// nova alt: valor calibrado fisicamente (teste_calibracao_encoders.ino,
// empurrando o robô 18 cm na mão, 3 repetições consistentes em 300 ticks)
// ========
#define TICKS_PER_CELL 305

#define CELL_FORWARD_TIMEOUT_MS 885  // tempo MÁXIMO de segurança por célula, caso o encoder falhe

// ========
// nova alt: REMOVIDO — antes existia uma janela pra ignorar ruído do
// motor logo no início do trajeto (IGNORAR_SENSOR_INICIO_MS). Como agora
// o sensor da frente simplesmente NÃO é mais lido/processado durante o
// trajeto reto (pedido explícito — "anda com o sensor desligado"), essa
// janela ficou sem função: não existe mais leitura no meio do caminho
// pra precisar ser ignorada.
// ========

// -----------------------------------------------------------------------
// ========
// nova alt: correção de reta em tempo real usando os encoders
// ========
// O motor esquerdo é fisicamente mais forte que o direito (confirmado em
// teste físico — o rato inclinava pra direita andando reto). Em vez de só
// "chutar" um PWM fixo diferente pros dois lados, agora comparamos os
// ticks acumulados de cada encoder DURANTE o trajeto e ajustamos o PWM de
// cada motor em tempo real pra manter os dois emparelhados.
// GANHO_CORRECAO_RETA: quanto maior, mais agressiva a correção.
// PWM_CORRECAO_MAX: trava o tamanho máximo do ajuste, pra não estourar o PWM.
// -----------------------------------------------------------------------
#define GANHO_CORRECAO_RETA       5    // ajuste de PWM por tick de diferença entre encoders
#define PWM_CORRECAO_MAX         40    // ajuste máximo permitido (pra cima ou pra baixo) por motor

// -----------------------------------------------------------------------
// ========
// nova alt: assentamento mecânico rápido antes de iniciar um giro (não é
// leitura de sensor — isso agora é só a análise unificada de arredores).
// ========
// STOP_TIME_SETTLE: pequena pausa mecânica antes de iniciar uma curva,
//   só pra garantir que o chassi já está parado de vez antes de virar.
// -----------------------------------------------------------------------
#define STOP_TIME_SETTLE        100    // ms — assentamento mecânico antes de iniciar um giro

// -----------------------------------------------------------------------
// ========
// nova alt: detecção de travamento em DUAS ETAPAS.
// TENTATIVAS_PARA_RE_DESTRAVAR: depois de quantos giros seguidos sem
//   avançar de célula ele tenta a ré pequena de desempacar (uma vez só).
// MIN_TENTATIVAS_GIRO: total de tentativas até declarar travamento de
//   vez e parar. Cada célula é um quadrado (4 lados possíveis), então
//   dar mais margem que isso permite tentar a ré de desempacar no meio
//   do caminho antes de desistir de vez.
// -----------------------------------------------------------------------
#define TENTATIVAS_PARA_RE_DESTRAVAR   5    // tentativas antes de tentar a re pequena de desempacar
#define MIN_TENTATIVAS_GIRO           10    // tentativas totais antes de avisar travamento (era 8)

// -----------------------------------------------------------------------
// Sensores de parede (filtro por leituras)
// -----------------------------------------------------------------------ticks>
#define FILTER_SIZE             3    // tamanho do filtro de leitura dos sensores
#define SENSOR_UPDATE_TIME      10   // ms entre leituras dos sensores
#define PRINT_SENSOR_TIME       150   // ms entre prints de status (debug)

// ========
// nova alt: votos necessários por sensor. ESQ/DIR usam o critério normal
// (maioria simples). FRENTE agora exige TODAS as leituras do filtro
// (unânime) — fica mais "durão"/menos sensível, só confirma parede
// quando está bem mais perto de verdade. Isso ataca o caso em que o
// chassi ficava "travado" numa situação em que só seguir em frente
// resolvia, mas o sensor da frente disparava cedo demais e não deixava.
// ========
#define VOTOS_NECESSARIOS_LATERAL   3    // ESQ/DIR: 3 de FILTER_SIZE (4) leituras
#define VOTOS_NECESSARIOS_FRENTE    FILTER_SIZE   // FRENTE: TODAS as leituras (mais rigoroso, só dispara mais perto)

// -----------------------------------------------------------------------
// ========
// nova alt: tamanho do labirinto configurável. O labirinto da competição
// pode ser 4x4, 8x8 ou 16x16 — troque esse valor ANTES de compilar,
// dependendo da prova. O resto do código já é genérico em cima desse
// número (mapa, flood fill, telemetria, tudo se ajusta sozinho).
// ========
// -----------------------------------------------------------------------
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

// -----------------------------------------------------------------------
// ========
// nova alt: dois modos de operação — MAPEAMENTO (devagar, célula por
// célula, parando e lendo os sensores com calma) e CORRIDA (mais rápido,
// usando o que já foi mapeado). O robô começa em MAPEAMENTO e muda pra
// CORRIDA sozinho na primeira vez que chega ao centro.
// ========
// -----------------------------------------------------------------------
#define PWM_LEFT_MAPEAMENTO     70    // PWM mais lento pro modo mapeamento (cauteloso)
#define PWM_RIGHT_MAPEAMENTO    70
#define PWM_LEFT_CORRIDA        PWM_LEFT   // modo corrida usa os PWM normais (mais rápidos)
#define PWM_RIGHT_CORRIDA       PWM_RIGHT

// =============================================================================
// PINOS
// =============================================================================
// ========
// nova alt: pinos ESQUERDO e FRENTE estavam fisicamente trocados na
// fiação (confirmado em teste isolado com teste_sensores.ino — colocar a
// mão na frente mudava a leitura do ESQ, e vice-versa). Corrigido aqui.
// ========
#define SENSOR_LEFT   36
#define SENSOR_FRONT  34
#define SENSOR_RIGHT  39

// Motores
// MOTOR_IN1 = motor esquerdo para trás
// MOTOR_IN2 = motor esquerdo para frente
// MOTOR_IN3 = motor direito para trás
// MOTOR_IN4 = motor direito para frente
#define MOTOR_IN1 25
#define MOTOR_IN2 26
#define MOTOR_IN3 32
#define MOTOR_IN4 33

// ========
// nova alt: pinos dos encoders esquerdo/direito
// ========
// Canal conhecido do encoder esquerdo: GPIO 16, 17 (16 foi a porta nova
// encontrada em teste; 17 já era conhecida).
// Encoder direito: GPIO 18, 19 (já confirmados em teste).
#define ENCODER_LEFT_A   17
#define ENCODER_LEFT_B   16
#define ENCODER_RIGHT_A  19
#define ENCODER_RIGHT_B  18

// RGB desativado por enquanto
#define USE_RGB 0
#define SENSOR_RGB_PIN 0

// Botão não será usado para iniciar automaticamente
#define BUTTON_PIN 0

// =============================================================================
// CONFIGURAÇÕES
// =============================================================================
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

// ========
// nova alt (bateria): objeto INA226 + estado de leitura da bateria
// ========
// inaOk fica true se a INA226 respondeu no I2C. Se ela nao estiver
// conectada, o firmware NAO trava — mantem os ultimos valores padrao e o
// site aplica o fallback ?? 0. As variaveis "Atual" guardam a ultima
// leitura boa pra telemetria nunca mandar lixo.
INA226 ina(INA226_ENDERECO);
bool inaOk = false;
float batTensaoAtual   = 7.4;    // V   (bus voltage)
float batPercentAtual  = 100.0;  // %   (derivado da tensao)
float batCorrenteAtual = 0.0;    // mA
unsigned long ultimoUpdateBateria = 0;

// ========
// nova alt: o mapa de telemetria agora usa memória máxima 16x16, mas o
// tamanho real enviado ao site é calculado por mapSizeWeb(), com base no
// mazeSize recebido via WebSocket.
// ========

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
    FINALIZADO,
    TRAVADO, // ========  nova alt: novo estado de travamento  ========
    // ========
    // nova alt: novo estado — mapeamento terminou (chegou ao centro pela
    // primeira vez) e o robô está PARADO DE VEZ, esperando o comando de
    // "iniciar corrida" vindo do site via WiFi. Continua mandando
    // telemetria normalmente nesse estado.
    // ========
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

// =============================================================================
// ========
// nova alt: ENCODERS — contadores globais e ISRs
// ========
// =============================================================================
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

// =============================================================================
// ========
// nova alt (bateria): INICIALIZACAO E LEITURA DA INA226
// ========
// =============================================================================
float tensaoParaPercentualBateria(float tensao) {
    // Conversao linear tensao->%. E uma aproximacao: a curva real de
    // descarga de LiPo/Li-ion nao e linear, entao a % "cai rapido" perto
    // do fim. Pra competicao costuma bastar; se quiser mais fiel, use uma
    // tabela de pontos (tensao->%) da sua bateria especifica.
    float pct = (tensao - BAT_TENSAO_MIN) / (BAT_TENSAO_MAX - BAT_TENSAO_MIN) * 100.0;
    return constrain(pct, 0.0, 100.0);
}

void initBateriaINA226() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    if (ina.begin()) {
        // Calibra o shunt: necessario para corrente/potencia. A tensao de
        // barramento (usada pra % de bateria) funciona independente disso.
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

// Le a INA226 e atualiza as variaveis globais de bateria. Nao bloqueia:
// respeita BAT_UPDATE_MS entre leituras. Se a INA226 nao inicializou,
// simplesmente mantem os ultimos valores.
void atualizarBateria() {
    if (!inaOk) {
        return;
    }

    unsigned long agora = millis();
    if (agora - ultimoUpdateBateria < BAT_UPDATE_MS) {
        return;
    }
    ultimoUpdateBateria = agora;

    float tensao = ina.getBusVoltage();   // V

    // Descarta leitura claramente invalida (ex: ~0 = sem barramento).
    // Alimentando so por USB voce ve ~4.3V aqui (residuo do USB), o que
    // e esperado — so com a bateria 2S ligada e que aparece ~6.5-8.4V.
    if (tensao > 1.0) {
        batTensaoAtual   = tensao;
        batPercentAtual  = tensaoParaPercentualBateria(tensao);
        batCorrenteAtual = ina.getCurrent_mA();
    }
}

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

// ========
// nova alt: agora recebe quantos votos (leituras LOW) são necessários
// pra confirmar "parede". Permite que o sensor da frente exija um
// critério mais rigoroso (unânime) do que os laterais (maioria simples).
// LOW = parede
// ========
bool lerSensorFiltrado(SensorParede &s, uint8_t votosNecessarios) {
    s.readings[s.index] = digitalRead(s.pin);
    s.index = (s.index + 1) % FILTER_SIZE;

    int leiturasLow = 0;

    for (int i = 0; i < FILTER_SIZE; i++) {
        if (s.readings[i] == LOW) {
            leiturasLow++;
        }
    }

    return leiturasLow >= votosNecessarios;
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
extern struct SensorParede sensorEsq;
extern struct SensorParede sensorFrente;
extern struct SensorParede sensorDir;
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

    // ========
    // nova alt (bateria): corrente em Ampere (a INA226 le em mA) pra
    // manter a mesma unidade que o payload ja usava (1.0 A fixo antes).
    // ========
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
    doc["sensorEsquerda"] = sensorEsq.wallDetected ? 1 : 0;
    doc["sensorDireita"] = sensorDir.wallDetected ? 1 : 0;
    doc["sensorFrontal"] = sensorFrente.wallDetected ? 1 : 0;

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
    // ========
    // nova alt: status agora também reflete o estado de travamento
    // ========
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
    // ========
    // nova alt (bateria): batteryPercent e o campo que o Dashboard.jsx le
    // como data.batteryPercent -> percentualBateria no banco. Agora e real.
    // ========
    doc["batteryPercent"] = batPercentAtual;
    doc["speedMps"] = 0.55;

    // ========
    // nova alt (bateria): tensaoEletrica tambem no nivel raiz, caso o site
    // leia data.tensaoEletrica direto (alem do bloco historico abaixo).
    // ========
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

// ========
// nova alt: flag setada quando o site manda o comando de iniciar a
// corrida via WebSocket. É só verificada no loop() — quem decide o que
// fazer com ela é o loop principal, não essa função de evento.
// ========
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

// Sensores globais
SensorParede sensorEsq    = {SENSOR_LEFT};
SensorParede sensorFrente = {SENSOR_FRONT};
SensorParede sensorDir    = {SENSOR_RIGHT};

void atualizarSensores() {
    // ========
    // nova alt: FRENTE agora usa VOTOS_NECESSARIOS_FRENTE (mais rigoroso,
    // exige leitura unânime) — ESQ/DIR continuam com o critério normal.
    // ========
    sensorEsq.wallDetected    = lerSensorFiltrado(sensorEsq, VOTOS_NECESSARIOS_LATERAL);
    sensorFrente.wallDetected = lerSensorFiltrado(sensorFrente, VOTOS_NECESSARIOS_FRENTE);
    sensorDir.wallDetected    = lerSensorFiltrado(sensorDir, VOTOS_NECESSARIOS_LATERAL);
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
// ========
// nova alt: não é mais chamada em lugar nenhum do fluxo principal —
// substituída por analisarArredores(), que já limpa o filtro e lê por
// mais tempo (1s no mapeamento). Mantida aqui como utilitário, útil pra
// testes manuais/debug se precisar limpar os sensores rapidinho.
// ========
void limparFiltrosSensores() {
    limparFiltroSensor(sensorEsq);
    limparFiltroSensor(sensorFrente);
    limparFiltroSensor(sensorDir);

    for (int i = 0; i < FILTER_SIZE; i++) {
        atualizarSensores();
        delay(SENSOR_UPDATE_TIME);
    }
}

// ========
// nova alt: estado de modo (MAPEAMENTO/CORRIDA) e PWM base usados dentro
// de MotorController, que é definida mais abaixo no arquivo. Precisa do
// "extern" aqui porque a classe é compilada antes da definição real
// dessas variáveis (mesmo motivo do forward declaration dos structs).
// ========
extern ModoOperacao modoAtual;
extern int pwmBaseEsq;
extern int pwmBaseDir;

// ========
// nova alt: ANÁLISE DE ARREDORES — o "parar e ler com calma" pedido.
// Chamada UMA VEZ no topo de cada ciclo do loop principal, sempre com o
// robô 100% parado (depois de qualquer andar ou virar anterior). Limpa o
// filtro e faz leituras contínuas por TEMPO_ANALISE_ARREDORES_MS (1s no
// modo mapeamento, bem devagar — pedido explícito, pra dar tempo de
// entender os arredores direito antes de decidir o próximo passo). No
// modo corrida, usa STOP_TIME_SETTLE (bem mais rápido), já que ali se
// confia no mapa e a prioridade é velocidade.
// ========
void analisarArredores() {
    limparFiltroSensor(sensorEsq);
    limparFiltroSensor(sensorFrente);
    limparFiltroSensor(sensorDir);

    unsigned long duracao = (modoAtual == MODO_MAPEAMENTO) ? TEMPO_ANALISE_ARREDORES_MS : STOP_TIME_SETTLE;
    unsigned long inicio = millis();

    while (millis() - inicio < duracao) {
        atualizarSensores();
        // ========
        // nova alt (bateria): aproveita a pausa de analise pra tambem
        // atualizar a bateria (respeitando BAT_UPDATE_MS internamente).
        // ========
        atualizarBateria();
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

// =============================================================================
// MAPA DO LABIRINTO
// =============================================================================
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

    // ========
    // nova alt: anda pra frente corrigindo o PWM em tempo real, comparando
    // quantos ticks cada encoder acumulou. Se a esquerda andou mais
    // (motor mais forte), reduz o PWM dela e aumenta o da direita, e
    // vice-versa. Substitui o uso de forward() fixo dentro de
    // andarUmaCelula().
    // ========
    void forwardComCorrecao() {
        long left;
        long right;

        noInterrupts();
        left = encoderLeftCount;
        right = encoderRightCount;
        interrupts();

        long diff = labs(left) - labs(right); // positivo = esquerda andou mais (puxa pra direita)

        int ajuste = (int)constrain(diff * GANHO_CORRECAO_RETA, -PWM_CORRECAO_MAX, PWM_CORRECAO_MAX);

        // ========
        // nova alt: usa o PWM base do modo atual (mapeamento = mais lento,
        // corrida = mais rápido) em vez do PWM fixo
        // ========
        int pwmEsq = constrain(pwmBaseEsq - ajuste, 0, 255);
        int pwmDir = constrain(pwmBaseDir + ajuste, 0, 255);

        motorEsquerdoFrente(pwmEsq);
        motorDireitoFrente(pwmDir);
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

    // ========
    // nova alt: pulso de ré antes de curvas em quina, agora medido por
    // encoder em vez de tempo fixo — mais consistente independente da
    // velocidade real do motor. Refatorado pra uma função genérica
    // (executarRe) reaproveitada tanto pela ré normal de curva quanto
    // pela ré pequena de desempacar (ver mais abaixo).
    // ========
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

        // ========
        // nova alt: print de diagnóstico — mostra quantos ticks a ré
        // realmente conseguiu fazer o robô andar. Se esse número ficar
        // sempre em 0 (ou muito baixo/igual em toda tentativa), é sinal
        // de que o PWM usado não está vencendo o atrito estático do
        // motor — ou seja, a ré não está movendo o robô de verdade.
        // ========
        Serial.print("[MICROMOUSE] Re executada. PWM=");
        Serial.print(pwm);
        Serial.print(" alvo=");
        Serial.print(ticksAlvo);
        Serial.print(" ticks, conseguiu=");
        Serial.print(lerMediaEncoders());
        Serial.println(" ticks");
    }

    void pulsoReAntesDeCurva() {
        executarRe(REVERSE_PULSE_PWM, REVERSE_PULSE_TICKS, REVERSE_PULSE_TIMEOUT_MS);
    }

    // ========
    // nova alt: ré BEM pequena de "desempacar", usada quando o robô fica
    // repetindo o mesmo padrão de parede (quina) várias vezes seguidas
    // sem avançar — às vezes só uma ré minúscula já libera o chassi de
    // uma quina onde ele ficou meio preso fisicamente.
    // ========
    void pulsoReDestravar() {
        executarRe(MICRO_RE_PWM, MICRO_RE_TICKS, MICRO_RE_TIMEOUT_MS);
    }

    // ========
    // nova alt: pausa mecânica simples antes de iniciar um giro (só
    // garante que o chassi está parado de vez). Não lê mais sensor aqui —
    // a leitura de verdade é toda concentrada em analisarArredores(), no
    // topo do loop principal, depois que o movimento termina.
    // ========
    void pararComLeitura(int tempoMs) {
        stop();
        delay(tempoMs);
    }

    bool andarUmaCelula() {
        // ========
        // nova alt: checagem rápida usando a ÚLTIMA leitura já feita pela
        // análise de arredores (não faz uma leitura nova aqui) — isso é
        // só uma rede de segurança, já que a decisão de mandar
        // MOVE_FORWARD só acontece quando o sensor da frente já estava
        // livre na última análise.
        // ========
        if (sensorFrente.wallDetected) {
            stop();
            return false;
        }

        // ========
        // nova alt: PULSO DE ANDAMENTO SEM SENSOR. Pedido explícito: andar
        // a célula inteira "com o sensor desligado" — ou seja, sem ler
        // nem processar o sensor da frente durante o trajeto. A distância
        // é controlada só pelo encoder (TICKS_PER_CELL). Isso reduz a
        // quantidade de decisões tomadas no meio do movimento; a leitura
        // de verdade só acontece DEPOIS, parado, na análise de arredores
        // (analisarArredores(), chamada no topo do loop principal).
        // Trade-off: se o encoder estiver mal calibrado ou a roda
        // escorregar, não há mais parada de emergência por sensor durante
        // o trajeto — por isso manter TICKS_PER_CELL bem calibrado é
        // importante.
        // ========
        resetEncoders();

        unsigned long inicio = millis();
        unsigned long limiteSeguranca = inicio + CELL_FORWARD_TIMEOUT_MS;

        while (true) {
            forwardComCorrecao();

            if (lerMediaEncoders() >= TICKS_PER_CELL) {
                break;
            }

            if (millis() > limiteSeguranca) {
                // Segurança: encoder pode ter falhado/escorregado a roda.
                // Para por tempo máximo em vez de travar o loop.
                break;
            }

            delay(SENSOR_UPDATE_TIME);
        }

        stop();
        return true;
    }

    // ========
    // nova alt: as 3 curvas agora são bem mais simples — giram por tempo
    // (TURN_TIME_90/180) sem ler sensor durante o giro, e sem nenhuma
    // leitura pós-curva própria. A leitura de verdade é sempre feita
    // depois, parada, por analisarArredores() no topo do loop principal
    // — que agora é o ÚNICO lugar do código que processa os sensores.
    // ========
    void virarDireita90() {
        pararComLeitura(STOP_TIME_SETTLE);

        turnRight();
        delay(TURN_TIME_90);

        stop();
    }

    void virarEsquerda90() {
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

    // ========
    // nova alt (curva 45): giros de 45 graus, usados SOMENTE na curva de
    // desempaque (logo apos a mini-re de destravamento). Sao copias diretas
    // das versoes de 90, so trocando TURN_TIME_90 por TURN_TIME_45 — mesma
    // mecanica, mesmo PWM de curva, so o tempo (angulo) muda. Nao entram no
    // execute() padrao, entao NAO afetam nenhum giro normal.
    // ========
    void virarDireita45() {
        pararComLeitura(STOP_TIME_SETTLE);

        turnRight();
        delay(TURN_TIME_45);

        stop();
    }

    void virarEsquerda45() {
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

// ========
// nova alt: dois modos de operação.
// MAPEAMENTO: anda devagar, célula por célula, parando e lendo os
//   sensores com calma — usado pra descobrir o labirinto até o centro.
// CORRIDA: usa o mapa já descoberto pra andar mais rápido (PWM normal),
//   com leitura de sensores mais rápida entre as células.
// O robô começa em MAPEAMENTO e muda pra CORRIDA sozinho quando chega ao
// centro pela primeira vez. (enum definido no topo do arquivo)
// ========
ModoOperacao modoAtual = MODO_MAPEAMENTO;
int pwmBaseEsq = PWM_LEFT_MAPEAMENTO;
int pwmBaseDir = PWM_RIGHT_MAPEAMENTO;

// ========
// nova alt: objetivo atual da navegação. Depois que o mapeamento chega ao
// centro, o robô passa a perseguir o INICIO (célula 0,0) em modo corrida,
// usando o mapa recém-descoberto — e, ao chegar lá, faz uma última corrida
// de volta ao centro (agora rápida) antes de finalizar de vez. Isso imita
// o formato clássico de competição de micromouse: mapear, depois correr.
// ========
enum ObjetivoAtual : uint8_t { OBJETIVO_CENTRO, OBJETIVO_INICIO };
ObjetivoAtual objetivoAtual = OBJETIVO_CENTRO;
Position startGoal[1] = { {0, 0} };

bool atStart() {
    return currentPos.r == 0 && currentPos.c == 0;
}

// ========
// nova alt: variável de controle do travamento — agora é só uma contagem
// de comandos de giro consecutivos sem o robô avançar de célula. A versão
// anterior comparava se os sensores "mudavam" antes/depois do giro, mas
// isso sempre muda só por causa da reorientação (o sensor da frente passa
// a olhar pra onde antes era lateral), então o contador nunca disparava de
// verdade. Esse contador zera sempre que o robô avança com sucesso.
// ========
uint8_t tentativasGiroAtual = 0;
bool robotTravado = false;

Position centroGoals[4];

void atualizarCentroGoals() {
    uint8_t mid = mazeSize / 2;

    centroGoals[0] = {(int8_t)(mid - 1), (int8_t)(mid - 1)};
    centroGoals[1] = {(int8_t)(mid - 1), (int8_t)mid};
    centroGoals[2] = {(int8_t)mid,       (int8_t)(mid - 1)};
    centroGoals[3] = {(int8_t)mid,       (int8_t)mid};
}

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
    Serial.print(sensorDir.wallDetected ? "PAREDE" : "LIVRE");

    // ========  nova alt (bateria): mostra bateria no status de debug  ========
    Serial.print(" | BAT: ");
    Serial.print(batTensaoAtual, 2);
    Serial.print("V (");
    Serial.print(batPercentAtual, 0);
    Serial.println("%)");
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

// =============================================================================
// VERIFICAÇÃO DE CHEGADA
// =============================================================================
bool atCenter() {
    uint8_t mid = mazeSize / 2;

    return (currentPos.r == mid - 1 || currentPos.r == mid) &&
           (currentPos.c == mid - 1 || currentPos.c == mid);
}

// =============================================================================
// COMANDOS RECEBIDOS DO SITE
// =============================================================================
void configurarNovaTentativa(uint8_t novoMazeSize) {
    if (!mazeSizeValido(novoMazeSize)) {
        Serial.println("[MICROMOUSE] Maze size invalido. Use 4, 8 ou 16.");
        return;
    }

    motors.stop();

    mazeSize = novoMazeSize;

    currentPos = {0, 0};
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

    // ========
    // nova alt: inicialização dos encoders esquerdo/direito
    // ========
    initEncoders();

    // ========
    // nova alt (bateria): inicializa a INA226 (I2C). Se falhar, o robo
    // segue funcionando normalmente e a bateria fica nos valores padrao.
    // ========
    initBateriaINA226();

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

    // ========
    // nova alt: define o modo WiFi explicitamente como AP antes de criar
    // o Access Point. Sem isso, dependendo do estado anterior do chip
    // (ex: se em algum momento já tentou WiFi.begin() como estação), o
    // rádio pode ficar num modo ambíguo e o AP não aparece de verdade
    // pros dispositivos ao redor, mesmo sem erro visível no código.
    // ========
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

// =============================================================================
// LOOP PRINCIPAL
// =============================================================================
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

    // ========
    // nova alt: mapeamento terminou e o robô está PARADO DE VEZ, esperando
    // o comando "INICIAR_CORRIDA" do site via WebSocket. Continua mandando
    // telemetria normalmente (o site precisa ver que ele tá parado e
    // pronto). Só sai desse estado quando comandoIniciarCorrida vira true.
    // ========
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

    // ========
    // nova alt: se o robô foi marcado como travado, para e avisa
    // repetidamente em vez de continuar tentando se mover
    // ========
    if (robotTravado) {
        webSocket.loop();
        motors.stop();
        publicarTelemetria();
        delay(200);
        return;
    }

    // ========
    // nova alt: CICLO DE PULSO REESTRUTURADO, exatamente como pedido:
    // 1) fica PARADO analisando os arredores com calma (analisarArredores,
    //    1 segundo no modo mapeamento) — único lugar do código que lê os
    //    sensores de verdade
    // 2) decide o que fazer com base NESSA leitura (reto ou curva)
    // 3) executa o movimento (anda 1 célula OU vira) sem processar sensor
    //    durante o trajeto (ver andarUmaCelula/virar*)
    // 4) volta pro passo 1 na próxima volta do loop
    // Isso concentra toda decisão em um momento só, com o robô sempre
    // 100% parado, reduzindo bastante a chance de erro de curva.
    // ========
    webSocket.loop();
    analisarArredores();
    imprimirStatus();

    uint32_t corRGB = lerRGB();

    updateWalls();

    // ========
    // nova alt: o alvo do flood fill agora depende do objetivo atual —
    // CENTRO (mapeamento e corrida final) ou INICIO (corrida de volta,
    // depois que o centro já foi mapeado pela primeira vez).
    // ========
    if (objetivoAtual == OBJETIVO_CENTRO) {
        floodFill.computeMulti(centroGoals, 4);
    } else {
        floodFill.computeMulti(startGoal, 1);
    }

    bool chegouCentro = atCenter() || corRGB == FINISH_COLOR;
    bool chegouInicio = atStart();

    if (objetivoAtual == OBJETIVO_CENTRO && chegouCentro) {
        if (modoAtual == MODO_MAPEAMENTO) {
            // ========
            // nova alt: terminou o mapeamento até o centro. PARA DE VEZ e
            // fica esperando o comando "INICIAR_CORRIDA" do site via
            // WebSocket — não troca mais de modo sozinho.
            // ========
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
        // ========
        // nova alt: voltou ao início em modo corrida. Agora faz a corrida
        // final de volta ao centro, já rápida, usando o mapa conhecido.
        // ========
        Serial.println("[MICROMOUSE] De volta ao inicio! Iniciando corrida final ate o centro...");
        objetivoAtual = OBJETIVO_CENTRO;
    }

    if (estadoAtual == MOVIMENTO_FRENTE && !robotTravado) {
        MoveCommand cmd = navigator.decide(currentPos, currentDir);

        // ========
        // nova alt: a regra de quina agora manda sempre que o sensor da
        // frente acusar parede AGORA — independente do que o flood fill
        // (baseado no mapa acumulado) tinha decidido antes. Antes, isso só
        // era checado quando cmd == MOVE_FORWARD, então se o mapa já
        // tivesse decidido virar por outro motivo, a regra de "vira pro
        // lado livre" nunca era aplicada e ele acabava sempre fazendo 180.
        // Regra: parede na frente + só 1 lado livre -> 90 pro lado livre.
        // Parede na frente + nos dois lados -> 180.
        // ========
        if (sensorFrente.wallDetected) {
            if (!sensorDir.wallDetected) {
                cmd = TURN_RIGHT_CMD;
            } else if (!sensorEsq.wallDetected) {
                cmd = TURN_LEFT_CMD;
            } else {
                cmd = TURN_BACK_CMD;
            }
        }

        // ========
        // nova alt: detecção de travamento em DUAS ETAPAS.
        // Etapa 1 (TENTATIVAS_PARA_RE_DESTRAVAR): a partir dessa
        // quantidade de giros seguidos sem avançar de célula, dá uma ré
        // BEM pequena a CADA tentativa (não só uma vez) — muitas vezes o
        // chassi só está preso fisicamente numa quina, e vai tentando
        // desempacar em cada nova tentativa até ou avançar ou esgotar as
        // tentativas.
        // Etapa 2 (MIN_TENTATIVAS_GIRO): se mesmo assim continuar sem
        // avançar até esse total de tentativas, aí sim declara
        // travamento de verdade e para.
        // ========
        bool comandoEhGiro = (cmd == TURN_LEFT_CMD || cmd == TURN_RIGHT_CMD || cmd == TURN_BACK_CMD);
        bool jaFezReDestravar = false;

        if (comandoEhGiro) {
            tentativasGiroAtual++;

            Serial.print("[MICROMOUSE] Tentativa de giro sem avancar de celula: ");
            Serial.println(tentativasGiroAtual);

            if (tentativasGiroAtual >= TENTATIVAS_PARA_RE_DESTRAVAR) {
                Serial.println("[MICROMOUSE] Tentando re pequena para desempacar...");
                motors.pulsoReDestravar();
                // ========
                // nova alt: evita ré dobrada — se já fez a ré pequena de
                // desempacar nesta tentativa, NÃO faz também a ré normal
                // de curva logo em seguida (eram duas rés seguidas antes
                // do giro, o que só confundia mais do que ajudava).
                // ========
                jaFezReDestravar = true;
            }

            if (tentativasGiroAtual >= MIN_TENTATIVAS_GIRO) {
                robotTravado = true;
                motors.stop();
                Serial.println("[MICROMOUSE] TRAVAMENTO DETECTADO - muitos giros seguidos sem avancar de celula.");
            }
        }

        if (!robotTravado) {
            // ========
            // nova alt (curva 45): quando acabamos de fazer a mini-re de
            // desempaque (jaFezReDestravar == true) E o giro decidido e um
            // 90 (esquerda ou direita), fazemos SO 45 graus em vez do 90
            // cheio. Motivo (pedido): a mini-re muitas vezes ja realinha o
            // rato; um 90 completo passa do ponto e ele bate de novo. Um
            // passo de 45 corrige mais fino. O 180 (beco sem saida) NAO
            // entra aqui — continua 180 normal. E quando NAO houve
            // desempaque, tudo segue exatamente como antes (ré normal +
            // execute() de 90/180).
            // ========
            if (jaFezReDestravar && (cmd == TURN_RIGHT_CMD || cmd == TURN_LEFT_CMD)) {
                // Curva reduzida de 45 graus logo apos o desempaque.
                // Nao faz a re normal de curva aqui (a mini-re ja aconteceu).
                if (cmd == TURN_RIGHT_CMD) {
                    motors.virarDireita45();
                } else {
                    motors.virarEsquerda45();
                }

                // ========
                // nova alt (curva 45): o giro de 45 NAO completa uma virada
                // de 90, entao a orientacao logica (currentDir) do rato
                // ainda NAO mudou de quadrante. Por isso NAO chamamos
                // updatePosition aqui — deixamos currentDir intacto. Na
                // proxima volta do loop o rato reavalia os sensores ja
                // realinhado; se ainda precisar girar, completa o restante.
                // Isso evita "contar" um 90 que nao aconteceu de fato.
                // ========
            } else {
                // ========
                // Fluxo ORIGINAL, intocado: ré normal antes de toda curva
                // (se nao fez a mini-re de desempaque nesta tentativa) e
                // execute() do comando (90/180) como sempre foi.
                // ========
                if (comandoEhGiro && !jaFezReDestravar) {
                    motors.pulsoReAntesDeCurva();
                }

                Position posicaoAnterior = currentPos;
                bool avancou = motors.execute(cmd);

                updatePosition(cmd, avancou);

                if (avancou) {
                    atualizarMapaMovimento(posicaoAnterior, currentPos);

                    // ========
                    // nova alt: ao avançar de célula com sucesso, zera o contador
                    // de tentativas de giro travado (a contagem é por célula)
                    // ========
                    tentativasGiroAtual = 0;
                }

                if (maze.valid(currentPos.r, currentPos.c)) {
                    maze.get(currentPos.r, currentPos.c).visited = 1;
                }
            }
        }
    }

    publicarTelemetria();
    delay(20);
}
