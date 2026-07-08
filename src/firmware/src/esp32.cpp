#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

const char *ssid = "Micromouse_Telemetry";
const char *password = "12345678";

WebSocketsServer webSocket = WebSocketsServer(81);
unsigned long lastMillis = 0;

constexpr uint8_t MAP_SIZE = 33;
constexpr uint8_t MAZE_SIZE = 16;
constexpr uint16_t PATH_CAPACITY = MAZE_SIZE * MAZE_SIZE;

uint8_t mapaLabirinto[MAP_SIZE][MAP_SIZE];
int8_t caminhoLinhas[PATH_CAPACITY];
int8_t caminhoColunas[PATH_CAPACITY];

int8_t linhaAtual = 0;
int8_t colunaAtual = 0;
uint8_t direcaoAtual = 1;  // 0=NORTE, 1=LESTE, 2=SUL, 3=OESTE
uint16_t passoAtual = 1;
uint16_t caminhoTamanho = 0;
uint32_t numeroTentativa = 0;

float bateriaAtual = 100.0f;
float tensaoAtual = 7.4f;
float correnteRecente = 1.1f;
float velocidadeAtual = 0.55f;
bool desafioCumprido = false;
bool aguardandoInicio = true;
unsigned long inicioCorrida = 0;
String tempoConclusaoISO;

const int8_t deltaLinha[4] = {1, 0, -1, 0};
const int8_t deltaColuna[4] = {0, 1, 0, -1};
const char *nomesDirecao[4] = {"NORTE", "LESTE", "SUL", "OESTE"};

void inicializarMapa() {
    for (int i = 0; i < MAP_SIZE; i++) {
        for (int j = 0; j < MAP_SIZE; j++) {
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

bool dentroDoLabirinto(int8_t linha, int8_t coluna) {
    return linha >= 0 && linha < MAZE_SIZE && coluna >= 0 && coluna < MAZE_SIZE;
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

void marcarSegmentoVisitado(int8_t linhaAnterior, int8_t colunaAnterior, int8_t linhaNova, int8_t colunaNova) {
    if (!dentroDoLabirinto(linhaAnterior, colunaAnterior) || !dentroDoLabirinto(linhaNova, colunaNova)) {
        return;
    }

    uint8_t mapaLinhaAnterior = 0;
    uint8_t mapaColunaAnterior = 0;
    uint8_t mapaLinhaNova = 0;
    uint8_t mapaColunaNova = 0;

    celulaParaMapa(linhaAnterior, colunaAnterior, mapaLinhaAnterior, mapaColunaAnterior);
    celulaParaMapa(linhaNova, colunaNova, mapaLinhaNova, mapaColunaNova);

    mapaLabirinto[(mapaLinhaAnterior + mapaLinhaNova) / 2][(mapaColunaAnterior + mapaColunaNova) / 2] = 0;
}

void registrarCaminho(int8_t linha, int8_t coluna) {
    if (caminhoTamanho < PATH_CAPACITY) {
        caminhoLinhas[caminhoTamanho] = linha;
        caminhoColunas[caminhoTamanho] = coluna;
        caminhoTamanho++;
    }
}

String timestampISO(unsigned long uptimeMillis) {
    unsigned long totalSeconds = uptimeMillis / 1000UL;
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

bool podeAvancar(uint8_t direcao, int8_t linha, int8_t coluna) {
    int8_t proximaLinha = linha + deltaLinha[direcao];
    int8_t proximaColuna = coluna + deltaColuna[direcao];
    return dentroDoLabirinto(proximaLinha, proximaColuna);
}

uint8_t escolherProximaDirecao() {
    const uint8_t preferidas[4] = {
        direcaoAtual,
        (uint8_t)((direcaoAtual + 1) & 0x03),
        (uint8_t)((direcaoAtual + 3) & 0x03),
        (uint8_t)((direcaoAtual + 2) & 0x03),
    };

    for (uint8_t i = 0; i < 4; i++) {
        uint8_t direcao = preferidas[i];
        if (podeAvancar(direcao, linhaAtual, colunaAtual)) {
            return direcao;
        }
    }

    return direcaoAtual;
}

bool atCenter() {
    const int8_t centroMenor = (int8_t)(MAZE_SIZE / 2 - 1);
    const int8_t centroMaior = (int8_t)(MAZE_SIZE / 2);

    return (linhaAtual == centroMenor || linhaAtual == centroMaior) &&
           (colunaAtual == centroMenor || colunaAtual == centroMaior);
}

void simularPasso() {
    if (desafioCumprido) {
        return;
    }

    uint8_t direcaoEscolhida = escolherProximaDirecao();
    int8_t linhaAnterior = linhaAtual;
    int8_t colunaAnterior = colunaAtual;

    linhaAtual = (int8_t)(linhaAtual + deltaLinha[direcaoEscolhida]);
    colunaAtual = (int8_t)(colunaAtual + deltaColuna[direcaoEscolhida]);
    direcaoAtual = direcaoEscolhida;

    marcarSegmentoVisitado(linhaAnterior, colunaAnterior, linhaAtual, colunaAtual);
    marcarCelulaVisitada(linhaAtual, colunaAtual);
    registrarCaminho(linhaAtual, colunaAtual);

    passoAtual++;
    bateriaAtual -= 0.08f;

    if (bateriaAtual < 15.0f) {
        bateriaAtual = 15.0f;
    }

    tensaoAtual = 7.6f - ((100.0f - bateriaAtual) * 0.006f);
    correnteRecente = 0.9f + random(0, 40) / 100.0f;
    velocidadeAtual = 0.45f + random(0, 30) / 100.0f;

    if (atCenter()) {
        desafioCumprido = true;
        tempoConclusaoISO = timestampISO(millis() - inicioCorrida);
    }
}

void preencherMatriz(JsonDocument &doc, const char *chave) {
    JsonArray mapaJson = doc[chave].to<JsonArray>();

    for (uint8_t i = 0; i < MAP_SIZE; i++) {
        JsonArray linhaJson = mapaJson.add<JsonArray>();
        for (uint8_t j = 0; j < MAP_SIZE; j++) {
            linhaJson.add(mapaLabirinto[i][j]);
        }
    }
}

void iniciarNovaTentativa() {
    numeroTentativa = random(1000, 9999);
    linhaAtual = 0;
    colunaAtual = 0;
    direcaoAtual = 1;
    passoAtual = 1;
    caminhoTamanho = 0;
    desafioCumprido = false;
    aguardandoInicio = false;
    tempoConclusaoISO = "";
    bateriaAtual = 100.0f;
    inicioCorrida = millis();

    inicializarMapa();
    marcarCelulaVisitada(linhaAtual, colunaAtual);
    registrarCaminho(linhaAtual, colunaAtual);
}

void processarComandoWebSocket(const char *payload, size_t length) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) {
        return;
    }

    const char *type = doc["type"] | "";
    if (strcmp(type, "START") == 0 || strcmp(type, "START_RUN") == 0) {
        iniciarNovaTentativa();
        Serial.printf("[ESP32] Nova tentativa iniciada: %u\n", numeroTentativa);
    }
}

void preencherPayload(JsonDocument &doc) {
    const String tempoColeta = timestampISO(millis());
    uint8_t posicaoMapaLinha = 0;
    uint8_t posicaoMapaColuna = 0;
    uint8_t inicioMapaLinha = 0;
    uint8_t inicioMapaColuna = 0;
    uint8_t objetivoMapaLinha = 0;
    uint8_t objetivoMapaColuna = 0;

    celulaParaMapa(linhaAtual, colunaAtual, posicaoMapaLinha, posicaoMapaColuna);
    celulaParaMapa(0, 0, inicioMapaLinha, inicioMapaColuna);
    celulaParaMapa((int8_t)(MAZE_SIZE / 2 - 1), (int8_t)(MAZE_SIZE / 2 - 1), objetivoMapaLinha, objetivoMapaColuna);

    doc["numTentativa"] = numeroTentativa;
    doc["tempoColeta"] = tempoColeta;
    doc["tensaoRecente"] = tensaoAtual;
    doc["correnteRecente"] = correnteRecente;
    doc["posHRecente"] = colunaAtual;
    doc["posVRecente"] = linhaAtual;
    doc["velocidadeAtual"] = velocidadeAtual;
    doc["bateriaAtual"] = bateriaAtual;
    doc["tensaoAtual"] = tensaoAtual;
    doc["sensorCor"] = "#000000";
    doc["sensorEsquerda"] = 0;
    doc["sensorDireita"] = 0;
    doc["sensorFrontal"] = 0;
    doc["tipoLabirinto"] = "16x16";
    doc["desafioCumprido"] = desafioCumprido ? "SIM" : "NAO";
    doc["status"] = aguardandoInicio
        ? "waiting_start"
        : (desafioCumprido ? "success" : "running");
    doc["elapsedSeconds"] = millis() / 1000.0;
    doc["batteryPercent"] = bateriaAtual;
    doc["speedMps"] = velocidadeAtual;
    doc["position"].add(posicaoMapaLinha);
    doc["position"].add(posicaoMapaColuna);
    doc["start"].add(inicioMapaLinha);
    doc["start"].add(inicioMapaColuna);
    doc["goal"].add(objetivoMapaLinha);
    doc["goal"].add(objetivoMapaColuna);

    if (desafioCumprido) {
        doc["tempoConclusao"] = tempoConclusaoISO;
    }

    JsonObject trajetoAtual = doc["trajetoAtual"].to<JsonObject>();
    trajetoAtual["numTentativa"] = numeroTentativa;
    trajetoAtual["passo"] = passoAtual;
    trajetoAtual["pos_h"] = colunaAtual;
    trajetoAtual["pos_v"] = linhaAtual;
    trajetoAtual["direcao"] = direcaoParaTexto(direcaoAtual);

    JsonArray caminhoJson = doc["visitedPath"].to<JsonArray>();
    for (uint16_t i = 0; i < caminhoTamanho; i++) {
        JsonArray ponto = caminhoJson.add<JsonArray>();
        uint8_t pontoLinha = 0;
        uint8_t pontoColuna = 0;
        celulaParaMapa(caminhoLinhas[i], caminhoColunas[i], pontoLinha, pontoColuna);
        ponto.add(pontoLinha);
        ponto.add(pontoColuna);
    }

    preencherMatriz(doc, "mapa");

    JsonObject resumoHistorico = doc["historico"].to<JsonObject>();
    resumoHistorico["percentualBateria"] = bateriaAtual;
    resumoHistorico["velocidadeMedia"] = velocidadeAtual;
    resumoHistorico["desafioCumprido"] = desafioCumprido ? "SIM" : "NAO";
    resumoHistorico["correnteEletrica"] = correnteRecente;
    resumoHistorico["tensaoEletrica"] = tensaoAtual;
    resumoHistorico["tipoLabirinto"] = "16x16";
    if (desafioCumprido) {
        resumoHistorico["tempoConclusao"] = tempoConclusaoISO;
    }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
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
            processarComandoWebSocket((const char *)payload, length);
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    randomSeed((uint32_t)millis());

    inicializarMapa();
    iniciarNovaTentativa();
    aguardandoInicio = true;

    Serial.println("Configurando Access Point...");
    WiFi.softAP(ssid, password);
    Serial.print("IP da ESP32: ");
    Serial.println(WiFi.softAPIP());

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

void loop() {
    webSocket.loop();

    if (millis() - lastMillis <= 100) {
        return;
    }

    lastMillis = millis();

    if (aguardandoInicio) {
        JsonDocument doc;
        preencherPayload(doc);
        String jsonPayload;
        serializeJson(doc, jsonPayload);
        webSocket.broadcastTXT(jsonPayload);
        return;
    }

    simularPasso();

    JsonDocument doc;
    preencherPayload(doc);

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    webSocket.broadcastTXT(jsonPayload);
}