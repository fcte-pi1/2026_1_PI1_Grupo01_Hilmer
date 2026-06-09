#include "motors.h"
#include "hal_motors.h"

void motors_init()
{
    hal_motors_init();
}

void motors_set_left(float speed)
{
    // Clamp speed between -1.0 and 1.0
    if (speed > 1.0f) speed = 1.0f;
    if (speed < -1.0f) speed = -1.0f;
    
    int pwm = (int)(speed * 255.0f);
    hal_motor_left(pwm);
}

void motors_set_right(float speed)
{
    // Clamp speed between -1.0 and 1.0
    if (speed > 1.0f) speed = 1.0f;
    if (speed < -1.0f) speed = -1.0f;
    
    int pwm = (int)(speed * 255.0f);
    hal_motor_right(pwm);
}

void motors_stop()
{
    motors_set_left(0.0f);
    motors_set_right(0.0f);
}
