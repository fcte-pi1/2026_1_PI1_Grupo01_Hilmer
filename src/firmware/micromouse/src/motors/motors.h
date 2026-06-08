#ifndef MOTORS_H
#define MOTORS_H

void motors_init();

void motors_set_left(float speed);
void motors_set_right(float speed);

void motors_stop();

#endif