#ifndef SENSORS_H
#define SENSORS_H

void sensors_init();

float sensors_read_front();
float sensors_read_left();
float sensors_read_right();

#endif