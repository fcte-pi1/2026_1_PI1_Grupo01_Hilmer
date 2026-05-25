#include <Arduino.h>
#include <Wire.h>

// Motor 1
#define PIN_M1_B1 4
#define PIN_M1_B2 5

// Motor 2
#define PIN_M2_B1 6
#define PIN_M2_B2 7

// RGB
#define PIN_RGB 8

// Sensor APDS9960
#define PIN_APDS9960_SCL 12

// Sensor INA226
#define PIN_INA226_SDA 36
#define PIN_INA226_SCL 37

// Sensores de proximidade
#define PIN_PROX_1 1
#define PIN_PROX_2 2
#define PIN_PROX_3 38

void setup() {

  Serial.begin(115200);

  // Configura as portas dos motores
  pinMode(PIN_M1_B1, OUTPUT);
  pinMode(PIN_M1_B2, OUTPUT);
  pinMode(PIN_M2_B1, OUTPUT);
  pinMode(PIN_M2_B2, OUTPUT);

  // Configura a porta do RGB
  pinMode(PIN_RGB, OUTPUT);

  // Configura as portas dos sensores
  pinMode(PIN_PROX_1, INPUT_PULLUP);
  pinMode(PIN_PROX_2, INPUT_PULLUP);
  pinMode(PIN_PROX_3, INPUT_PULLUP);

  // Configura porta do APDS9960
  pinMode(PIN_APDS9960_SCL, OUTPUT);

  // Configura pinos que fazem barramento i2c na INA226
  Wire.begin(PIN_INA226_SDA, PIN_INA226_SCL);

  Serial.println("Teste de GPIO iniciado");
}

void loop() {

  int prox1 = digitalRead(PIN_PROX_1);
  int prox2 = digitalRead(PIN_PROX_2);
  int prox3 = digitalRead(PIN_PROX_3);

  Serial.printf("P1: %d | P2: %d | P3: %d\n", prox1, prox2, prox3);

  // ver se a porta com APDS9960 está mapeada
  digitalWrite(PIN_APDS9960_SCL, HIGH);
  delay(300);

  digitalWrite(PIN_APDS9960_SCL, LOW);
  delay(300);

  if (prox1 == LOW || prox2 == LOW || prox3 == LOW) {

    Serial.println("Obstaculo detectado");

    digitalWrite(PIN_M1_B1, LOW);
    digitalWrite(PIN_M1_B2, LOW);
    digitalWrite(PIN_M2_B1, LOW);
    digitalWrite(PIN_M2_B2, LOW);

    digitalWrite(PIN_RGB, HIGH);

  } else {

    Serial.println("Caminho livre");

    digitalWrite(PIN_M1_B1, HIGH);
    digitalWrite(PIN_M1_B2, LOW);

    digitalWrite(PIN_M2_B1, HIGH);
    digitalWrite(PIN_M2_B2, LOW);

    digitalWrite(PIN_RGB, LOW);
  }

  // Teste marcha re
  digitalWrite(PIN_M1_B1, LOW);
  digitalWrite(PIN_M1_B2, HIGH);

  digitalWrite(PIN_M2_B1, LOW);
  digitalWrite(PIN_M2_B2, HIGH);

  delay(100);

  // Teste dos motores
  digitalWrite(PIN_M1_B1, LOW);
  digitalWrite(PIN_M1_B2, LOW);

  digitalWrite(PIN_M2_B1, LOW);
  digitalWrite(PIN_M2_B2, LOW);

  Serial.printf("INA226 SDA: %d | SCL: %d\n", PIN_INA226_SDA, PIN_INA226_SCL);

  Serial.println("----------------");

  delay(100);
}