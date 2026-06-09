#include "battery.h"
#include "../config/config.h"

#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#include <INA226.h>

INA226 ina(INA226_ADDRESS);
#endif

void initBatteryMonitor()
{
#ifdef ARDUINO
    Wire.begin(SDA_PIN, SCL_PIN);
    if (!ina.begin())
    {
        Serial.println("INA226 NOT DETECTED");
        // while (1); // Descomente para travar caso o INA não seja detectado
    }
    Serial.println("INA226 CONNECTED");
#endif
}

float readVoltage()
{
#ifdef ARDUINO
    return ina.getBusVoltage();
#else
    return 7.4f;
#endif
}

float readCurrent()
{
#ifdef ARDUINO
    return ina.getCurrent_mA() / 1000.0f;
#else
    return 0.35f;
#endif
}

float readPower()
{
#ifdef ARDUINO
    return ina.getPower_mW() / 1000.0f;
#else
    return readVoltage() * readCurrent();
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
    if (voltage >= 8.4) return 100;
    if (voltage >= 8.2) return 90;
    if (voltage >= 8.0) return 80;
    if (voltage >= 7.8) return 60;
    if (voltage >= 7.6) return 40;
    if (voltage >= 7.4) return 20;
    if (voltage >= 7.0) return 10;
    return 0;
}
