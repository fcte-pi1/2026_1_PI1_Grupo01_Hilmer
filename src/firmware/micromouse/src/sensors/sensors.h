#ifndef SENSORS_H
#define SENSORS_H

#ifdef TARGET_ESP32
#include "hal_sensors.h"
#define sensors_init hal_sensors_init
#define sensors_read_front hal_read_front_sensor
#define sensors_read_left hal_read_left_sensor
#define sensors_read_right hal_read_right_sensor
#else
#include "sim_sensors.h"
#define sensors_init sim_sensors_init
#define sensors_read_front sim_read_front_sensor
#define sensors_read_left sim_read_left_sensor
#define sensors_read_right sim_read_right_sensor
#endif

// Include battery and RGB as well
#include "battery.h"
#include "rgb_sensor.h"

#endif