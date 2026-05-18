#include <WiFi.h>
#include <WebSocketsServer.h>

// Definições do Access Point
const char *ssid = "Micromouse_Telemetry";
const char *password = "12345678"; // Mínimo 8 caracteres

WebSocketsServer webSocket = WebSocketsServer(81); // Servidor na porta 81
unsigned long lastMillis = 0;

// Função de callback para gerenciar eventos do WebSocket
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
            // Caso queira receber comandos do frontend/backend (ex: iniciar robô)
            Serial.printf("[%d] Mensagem recebida: %s\n", num, payload);
            break;
    }
}

void setup() {
    Serial.begin(115200);

    // Configura a ESP32 como Access Point
    Serial.println("Configurando Access Point...");
    WiFi.softAP(ssid, password);

    // Por padrão, o IP da ESP32 no modo AP será 192.168.4.1
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP iniciado! Nome da rede: ");
    Serial.println(ssid);
    Serial.print("IP da ESP32: ");
    Serial.println(IP);

    // Inicia o servidor WebSocket
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

void loop() {
    webSocket.loop();

    // Envia a telemetria a cada 100ms para TODOS os clientes conectados
    if (millis() - lastMillis > 100) {
        lastMillis = millis();

        // Dados simulados do Micromouse
        int posX = 5;
        int posY = 12;
        float bateria = 7.2;

        String jsonPayload = "{\"posX\":" + String(posX) + 
                             ",\"posY\":" + String(posY) + 
                             ",\"bateria\":" + String(bateria) + "}";

        // Envia o texto para todos os clientes conectados (Node.js ou React direto)
        webSocket.broadcastTXT(jsonPayload);
    }
}