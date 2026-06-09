#include "sim_motors.h"

int sim_left_motor = 0;
int sim_right_motor = 0;

void hal_motors_init()
{
    sim_left_motor = 0;
    sim_right_motor = 0;
}

void hal_motor_left(int speed)
{
    sim_left_motor = speed;
}

void hal_motor_right(int speed)
{
    sim_right_motor = speed;
}