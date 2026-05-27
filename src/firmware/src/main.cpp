#include <Arduino.h>
#include <Wire.h>
#include "pin_mapping.h"

void defineGPIOModes();
void testAPDS9960();
void testProximitySensors();
void testMotors();
void testINA226();

void setup() {

  Serial.begin(115200);

  defineGPIOModes();

  Serial.println("Início dos testes\n");

  testAPDS9960();
  testMotors();
  testProximitySensors();
  testINA226();

  Serial.println("Testes finalizados\n");
}

void loop() {

  testProximitySensors();

  delay(500);
}

void defineGPIOModes() {


  pinMode(PIN_M1B1, OUTPUT);
  pinMode(PIN_M1B2, OUTPUT);
  pinMode(PIN_M2B1, OUTPUT);
  pinMode(PIN_M2B2, OUTPUT);

  pinMode(PIN_PROX_1, INPUT_PULLUP);
  pinMode(PIN_PROX_2, INPUT_PULLUP);
  pinMode(PIN_PROX_3, INPUT_PULLUP);

  pinMode(PIN_CLK_APDS9960_SCL, OUTPUT);
  Wire.begin(PIN_INA226_SDA, PIN_INA226_SCL);
}

void testMotors() {

  Serial.println("Testando motores...");


  digitalWrite(PIN_M1B1, HIGH);
  digitalWrite(PIN_M1B2, LOW);

  delay(1000);

  digitalWrite(PIN_M1B1, LOW);
  digitalWrite(PIN_M1B2, LOW);

  delay(500);

  digitalWrite(PIN_M2B1, HIGH);
  digitalWrite(PIN_M2B2, LOW);

  delay(1000);

  digitalWrite(PIN_M2B1, LOW);
  digitalWrite(PIN_M2B2, LOW);

  delay(500);

  Serial.println("Motores testados\n");
}

void testAPDS9960() {

  Serial.println("Verificando APDS9960...");

  Wire.beginTransmission(0x39);

  byte error = Wire.endTransmission();

  if (error == 0) {

    Serial.println("APDS9960 encontrado no endereco 0x39\n");

  } else {

    Serial.println("APDS9960 NAO encontrado\n");
  }
}

void testINA226() {

  Serial.println("Verificando INA226...");

  Wire.beginTransmission(0x40);

  byte error = Wire.endTransmission();

  if (error == 0) {

    Serial.println("INA226 encontrado no endereco 0x40\n");

  } else {

    Serial.println("INA226 NAO encontrado\n");
  }
}

void testProximitySensors() {

  Serial.println("Testando sensores de proximidade...");

  uint8_t prox1 = digitalRead(PIN_PROX_1);
  uint8_t prox2 = digitalRead(PIN_PROX_2);
  uint8_t prox3 = digitalRead(PIN_PROX_3);

  Serial.print("PROX 1: ");
  Serial.println(prox1);

  Serial.print("PROX 2: ");
  Serial.println(prox2);

  Serial.print("PROX 3: ");
  Serial.println(prox3);

  Serial.println();

  delay(1000);
}