#ifndef BATTERY_H
#define BATTERY_H

void initBatteryMonitor();

float readVoltage();

float readCurrent();

float readPower();

bool lowBattery(float voltage);

bool veryLowBattery(float voltage);

bool criticalBattery(float voltage);

float batteryPercentage(float voltage);

#endif