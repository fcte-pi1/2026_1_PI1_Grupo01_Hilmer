#ifndef TASK_CONTROL_H
#define TASK_CONTROL_H

#include <Arduino.h>

#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>

void sensors_init();

uint16_t sensors_read_front();
uint16_t sensors_read_left();
uint16_t sensors_read_right();

#endif

void task_control(void *pvParameters);

#endif