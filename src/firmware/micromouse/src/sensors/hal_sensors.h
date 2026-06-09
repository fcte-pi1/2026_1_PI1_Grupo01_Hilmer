#ifndef HAL_SENSORS_H
#define HAL_SENSORS_H

void hal_sensors_init();
float hal_read_front_sensor();
float hal_read_left_sensor();
float hal_read_right_sensor();

#endif