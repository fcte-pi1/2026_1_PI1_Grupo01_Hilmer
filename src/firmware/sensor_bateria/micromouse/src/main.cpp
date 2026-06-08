#ifndef PIO_UNIT_TESTING

#include <Arduino.h>

#include "battery.h"
#include "telemetry.h"

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

#endif