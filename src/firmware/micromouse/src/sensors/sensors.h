#ifndef SENSORS_H
#define SENSORS_H

#ifdef TARGET_ESP32
#include "hal_sensors.h"
#else
#include "sim_sensors.h"
#endif

#endif