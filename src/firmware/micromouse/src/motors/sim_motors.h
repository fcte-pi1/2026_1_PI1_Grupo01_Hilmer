#ifndef SIM_MOTORS_H
#define SIM_MOTORS_H

extern int sim_left_motor;
extern int sim_right_motor;

void hal_motor_left(int speed);
void hal_motor_right(int speed);

#endif