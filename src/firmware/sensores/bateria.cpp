#include <Arduino.h>
#include <Wire.h>
#include <INA226.h>

struct BateriaSensorConfig {
  uint8_t sdaPin;
  uint8_t sclPin;
  uint8_t enderecoIna226;
  float tensaoMaxima;
  float tensaoMinima;
  float limiarBaixa;
  float limiarMuitoBaixa;
  float limiarCritica;
};

struct BateriaLeitura {
  float tensao;
  float corrente;
  float potencia;
  float percentual;
  bool baixa;
  bool muitoBaixa;
  bool critica;
  bool leituraValida;
};

static INA226* g_ina = nullptr;
static bool g_bateriaInicializada = false;

static const BateriaSensorConfig BATERIA_CONFIG_PADRAO = {
    8,
    9,
    0x40,
    8.4f,
    7.0f,
    7.0f,
    6.6f,
    6.0f};

static float limitar(float valor, float minimo, float maximo) {
  if (valor < minimo) {
    return minimo;
  }

  if (valor > maximo) {
    return maximo;
  }

  return valor;
}

static float calcularPercentual(float tensao, const BateriaSensorConfig& config) {
  const float faixa = config.tensaoMaxima - config.tensaoMinima;

  if (faixa <= 0.0f) {
    return 0.0f;
  }

  const float percentual = ((tensao - config.tensaoMinima) * 100.0f) / faixa;
  return limitar(percentual, 0.0f, 100.0f);
}

bool initBateriaSensor(const BateriaSensorConfig& config = BATERIA_CONFIG_PADRAO) {
  g_bateriaInicializada = false;

  Wire.begin(config.sdaPin, config.sclPin);

  if (g_ina != nullptr) {
    delete g_ina;
    g_ina = nullptr;
  }

  g_ina = new INA226(config.enderecoIna226);

  if (g_ina == nullptr) {
    return false;
  }

  if (!g_ina->begin()) {
    return false;
  }

  g_bateriaInicializada = true;
  return true;
}

BateriaLeitura lerBateriaSensor(const BateriaSensorConfig& config = BATERIA_CONFIG_PADRAO) {
  BateriaLeitura leitura = {0.0f, 0.0f, 0.0f, 0.0f, false, false, false, false};

  if (!g_bateriaInicializada || g_ina == nullptr) {
    return leitura;
  }

  leitura.tensao = g_ina->getBusVoltage();
  leitura.corrente = g_ina->getCurrent_mA() / 1000.0f;
  leitura.potencia = g_ina->getPower_mW() / 1000.0f;
  leitura.percentual = calcularPercentual(leitura.tensao, config);
  leitura.baixa = leitura.tensao <= config.limiarBaixa;
  leitura.muitoBaixa = leitura.tensao <= config.limiarMuitoBaixa;
  leitura.critica = leitura.tensao <= config.limiarCritica;
  leitura.leituraValida = true;

  return leitura;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!initBateriaSensor()) {
    Serial.println("Falha ao inicializar sensor de bateria.");

    while (true) {
      delay(1000);
    }
  }

  Serial.println("Sensor de bateria inicializado.");
}

void loop() {
  const BateriaLeitura leitura = lerBateriaSensor();

  if (!leitura.leituraValida) {
    Serial.println("Falha na leitura da bateria.");
    delay(1000);
    return;
  }

  Serial.print("Tensao: ");
  Serial.print(leitura.tensao);
  Serial.print(" V | Corrente: ");
  Serial.print(leitura.corrente);
  Serial.print(" A | Potencia: ");
  Serial.print(leitura.potencia);
  Serial.print(" W | Bateria: ");
  Serial.print(leitura.percentual);
  Serial.print("% | Estado: ");

  if (leitura.critica) {
    Serial.println("CRITICA");
  } else if (leitura.muitoBaixa) {
    Serial.println("MUITO BAIXA");
  } else if (leitura.baixa) {
    Serial.println("BAIXA");
  } else {
    Serial.println("OK");
  }

  delay(1000);
}
