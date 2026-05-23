#ifndef PIO_UNIT_TESTING

#include <Arduino.h>

#include "battery.h"
#include "telemetry.h"

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

    if (lowBattery(voltage))
    {
        Serial.println("LOW BATTERY");
    }
}

#endif