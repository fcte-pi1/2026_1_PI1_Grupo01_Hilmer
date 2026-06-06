#ifdef PIO_UNIT_TESTING

#include "battery.h"
#include "config.h"

#else

#include <Arduino.h>
#include <Wire.h>
#include <INA226.h>

#include "battery.h"
#include "config.h"

INA226 ina(INA226_ADDRESS);

#endif

#ifdef SIMULATION_MODE

static float simulatedVoltage = 7.4;
static float simulatedCurrent = 0.35;

#endif

void initBatteryMonitor()
{

#ifndef PIO_UNIT_TESTING

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