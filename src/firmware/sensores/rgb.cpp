#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_APDS9960.h>

struct RgbSensorConfig {
  uint8_t sdaPin;
  uint8_t sclPin;
  uint16_t limiarVermelho;
  uint16_t limiarVerdeMax;
  uint16_t limiarAzulMax;
  bool compensarLuzAmbiente;
};

struct RgbLeitura {
  uint16_t vermelho;
  uint16_t verde;
  uint16_t azul;
  uint16_t ambiente;
  bool alvoDetectado;
  bool leituraValida;
};

static SparkFun_APDS9960 g_apds;
static uint16_t g_luzAmbienteBase = 0;
static bool g_rgbInicializado = false;

static const RgbSensorConfig RGB_CONFIG_PADRAO = {
    21,
    22,
    200,
    100,
    100,
    true};

static uint16_t subtrairComSaturacao(uint16_t valor, uint16_t desconto) {
  return valor > desconto ? valor - desconto : 0;
}

bool initRgbSensor(const RgbSensorConfig& config = RGB_CONFIG_PADRAO) {
  g_luzAmbienteBase = 0;
  g_rgbInicializado = false;

  Wire.begin(config.sdaPin, config.sclPin);

  if (!g_apds.init()) {
    return false;
  }

  if (!g_apds.enableLightSensor(false)) {
    return false;
  }

  delay(500);

  if (!g_apds.readAmbientLight(g_luzAmbienteBase)) {
    return false;
  }

  g_rgbInicializado = true;
  return true;
}

RgbLeitura lerRgbSensor(const RgbSensorConfig& config = RGB_CONFIG_PADRAO) {
  RgbLeitura leitura = {0, 0, 0, 0, false, false};

  if (!g_rgbInicializado) {
    return leitura;
  }

  const bool leituraOk =
      g_apds.readRedLight(leitura.vermelho) &&
      g_apds.readGreenLight(leitura.verde) &&
      g_apds.readBlueLight(leitura.azul) &&
      g_apds.readAmbientLight(leitura.ambiente);

  if (!leituraOk) {
    return leitura;
  }

  if (config.compensarLuzAmbiente && leitura.ambiente > g_luzAmbienteBase) {
    const uint16_t excessoLuz = leitura.ambiente - g_luzAmbienteBase;
    leitura.vermelho = subtrairComSaturacao(leitura.vermelho, excessoLuz);
    leitura.verde = subtrairComSaturacao(leitura.verde, excessoLuz);
    leitura.azul = subtrairComSaturacao(leitura.azul, excessoLuz);
  }

  leitura.alvoDetectado =
      leitura.vermelho >= config.limiarVermelho &&
      leitura.verde <= config.limiarVerdeMax &&
      leitura.azul <= config.limiarAzulMax;
  leitura.leituraValida = true;

  return leitura;
}

uint16_t getRgbAmbientBase() {
  return g_luzAmbienteBase;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!initRgbSensor()) {
    Serial.println("Falha ao inicializar sensor RGB.");

    while (true) {
      delay(1000);
    }
  }

  Serial.println("Sensor RGB inicializado.");
  Serial.print("Luz ambiente base: ");
  Serial.println(getRgbAmbientBase());
}

void loop() {
  const RgbLeitura leitura = lerRgbSensor();

  if (!leitura.leituraValida) {
    Serial.println("Falha na leitura do sensor RGB.");
    delay(200);
    return;
  }

  Serial.print("R: ");
  Serial.print(leitura.vermelho);
  Serial.print(" | G: ");
  Serial.print(leitura.verde);
  Serial.print(" | B: ");
  Serial.print(leitura.azul);
  Serial.print(" | Ambiente: ");
  Serial.print(leitura.ambiente);
  Serial.print(" | Alvo: ");
  Serial.println(leitura.alvoDetectado ? "DETECTADO" : "NAO");

  delay(200);
}
