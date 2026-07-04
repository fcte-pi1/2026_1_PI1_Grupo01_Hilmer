/*
 * Monitor de bateria LiPo 2S via INA226 (I2C)
 *
 * Arduino IDE:
 *   Placa:           ESP32C3 Dev Module
 *   USB CDC On Boot: Enabled
 *   Biblioteca:      INA226 by Rob Tillaart (Library Manager)
 *   Monitor Serial:  115200 baud
 *
 * Sketch pronto: sensor_bateria/sensor_bateria.ino (abrir essa pasta na Arduino IDE)
 *
 * Teste Nivel 1 (sem hardware): descomente SIMULATION_MODE abaixo.
 * Teste Nivel 2 (com INA226):   comente SIMULATION_MODE e ligue SDA=GPIO8, SCL=GPIO9.
 */

#define SIMULATION_MODE

#define SDA_PIN 8
#define SCL_PIN 9

#define INA226_ADDRESS 0x40

#define LOW_BATTERY_VOLTAGE 7.0
#define VERY_LOW_BATTERY_VOLTAGE 6.6
#define CRITICAL_BATTERY_VOLTAGE 6.0

#include <Arduino.h>
#include <Wire.h>
#include <INA226.h>

#ifndef SIMULATION_MODE
INA226 ina(INA226_ADDRESS);
#endif

#ifdef SIMULATION_MODE
static float simulatedVoltage = 7.4;
static float simulatedCurrent = 0.35;
#endif

void initBatteryMonitor()
{
#ifdef SIMULATION_MODE
    Serial.println("SIMULATION MODE ENABLED");
#else
    Wire.begin(SDA_PIN, SCL_PIN);

    if (!ina.begin())
    {
        Serial.println("INA226 NOT DETECTED");

        while (1)
            ;
    }

    Serial.println("INA226 CONNECTED");
#endif
}

float readVoltage()
{
#ifdef SIMULATION_MODE
    return simulatedVoltage;
#else
    return ina.getBusVoltage();
#endif
}

float readCurrent()
{
#ifdef SIMULATION_MODE
    return simulatedCurrent;
#else
    return ina.getCurrent_mA() / 1000.0;
#endif
}

float readPower()
{
#ifdef SIMULATION_MODE
    return simulatedVoltage * simulatedCurrent;
#else
    return ina.getPower_mW() / 1000.0;
#endif
}

bool lowBattery(float voltage)
{
    return voltage <= LOW_BATTERY_VOLTAGE;
}

bool veryLowBattery(float voltage)
{
    return voltage <= VERY_LOW_BATTERY_VOLTAGE;
}

bool criticalBattery(float voltage)
{
    return voltage <= CRITICAL_BATTERY_VOLTAGE;
}

float batteryPercentage(float voltage)
{
    if (voltage >= 8.4)
        return 100;
    if (voltage >= 8.2)
        return 90;
    if (voltage >= 8.0)
        return 80;
    if (voltage >= 7.8)
        return 60;
    if (voltage >= 7.6)
        return 40;
    if (voltage >= 7.4)
        return 20;
    if (voltage >= 7.0)
        return 10;

    return 0;
}

void printTelemetry()
{
    float voltage = readVoltage();
    float current = readCurrent();
    float power = readPower();

    Serial.println("====================");

    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.println(" V");

    Serial.print("Current: ");
    Serial.print(current);
    Serial.println(" A");

    Serial.print("Power: ");
    Serial.print(power);
    Serial.println(" W");

    if (criticalBattery(voltage))
    {
        Serial.println("CRITICAL BATTERY");
    }
    else if (veryLowBattery(voltage))
    {
        Serial.println("VERY LOW BATTERY");
    }
    else if (lowBattery(voltage))
    {
        Serial.println("LOW BATTERY");
    }
    else
    {
        Serial.println("BATTERY OK");
    }

    float percentage = batteryPercentage(voltage);

    Serial.print("Battery: ");
    Serial.print(percentage);
    Serial.println("%");
}

void setup()
{
    Serial.begin(115200);

    delay(1000);

    initBatteryMonitor();
}

void loop()
{
    printTelemetry();

    delay(1000);
}
