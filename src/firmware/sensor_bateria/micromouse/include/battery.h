#ifndef BATTERY_H
#define BATTERY_H

void initBatteryMonitor();

float readVoltage();

float readCurrent();

float readPower();

bool lowBattery(float voltage);

#endif