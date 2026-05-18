#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h> 

const char *ssid = "Micromouse_Telemetry";
const char *password = "12345678";

WebSocketsServer webSocket = WebSocketsServer(81);
unsigned long lastMillis = 0;


const int MAP_SIZE = 33;

uint8_t mapaLabirinto[MAP_SIZE][MAP_SIZE];

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
    }
}

void setup() {
    Serial.begin(115200);
    
    inicializarMapa();
    
    Serial.println("Configurando Access Point...");
    WiFi.softAP(ssid, password);
    Serial.print("IP da ESP32: ");
    Serial.println(WiFi.softAPIP());
    
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

void loop() {
    webSocket.loop();
    
    if (millis() - lastMillis > 100) {
        lastMillis = millis();

        int posX_falsa = random(1, MAP_SIZE - 1);
        int posY_falsa = random(1, MAP_SIZE - 1);
        mapaLabirinto[posX_falsa][posY_falsa] = random(0, 2); 

        JsonDocument doc;
        
        doc["tipoLabirinto"] = "16x16";
        doc["bateriaConsumo"] = 45.2;               
        doc["velocidadeMedia"] = 0.55;              
        doc["tempoConclusao"] = millis() / 1000.0;  
        doc["desafioCumprido"] = "N";               
        
        JsonArray mapaJson = doc["mapa"].to<JsonArray>();
        for (int i = 0; i < MAP_SIZE; i++) {
            JsonArray linhaJson = mapaJson.add<JsonArray>();
            for (int j = 0; j < MAP_SIZE; j++) {
                linhaJson.add(mapaLabirinto[i][j]);
            }
        }
        
        String jsonPayload;
        serializeJson(doc, jsonPayload);

        
        webSocket.broadcastTXT(jsonPayload);
    }
}