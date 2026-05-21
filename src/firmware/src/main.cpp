#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_APDS9960.h>

// Definindo os pinos I2C padrão para a ESP32-C3
#define I2C_SDA 8
#define I2C_SCL 9

SparkFun_APDS9960 apds = SparkFun_APDS9960();

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  Serial.println("Inicializando Sensor RGB APDS-9960...");

  // Inicializa o sensor
  if (apds.init()) {
    Serial.println("APDS-9960 inicializado com sucesso!");
  } else {
    Serial.println("Falha ao inicializar o sensor. Verifique as conexões I2C.");
  }

  // Habilita a leitura de cores e luz ambiente
  if (apds.enableLightSensor(false)) {
    Serial.println("Leitor de cores ativado!");
  } else {
    Serial.println("Erro ao ativar leitor de cores.");
  }
}

void loop() {
  uint16_t r = 0, g = 0, b = 0, c = 0;

  // Realiza a leitura dos canais de cor
  if (apds.readRedLight(r) && apds.readGreenLight(g) && apds.readBlueLight(b) && apds.readAmbientLight(c)) {
    Serial.printf("Cores - R: %d | G: %d | B: %d | Ambiente: %d\n", r, g, b, c);

    // LÓGICA DE FIM DE LABIRINTO
    // Exemplo: Se o centro do labirinto for uma fita vermelha no chão
    if (r > 200 && g < 100 && b < 100) {
      Serial.println("🎯 FIM DO LABIRINTO DETECTADO! Acionando freios...");
      
      // Aqui você futuramente enviará a telemetria via WebSocket avisando o término:
      // doc["desafioCumprido"] = "S";
    }
  }

  delay(200); // 5Hz de frequência de leitura para não sobrecarregar a rotina principal
}