#include "sensors/sensors.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif

void sensors_init()
{
#ifdef ARDUINO
    // Inicialização dos sensores
#endif
}

uint16_t sensors_read_front()
{
#ifdef ARDUINO
    return analogRead(34);
#else
    return 100;
#endif
}

uint16_t sensors_read_left()
{
#ifdef ARDUINO
    return analogRead(35);
#else
    return 80;
#endif
}

uint16_t sensors_read_right()
{
#ifdef ARDUINO
    return analogRead(32);
#else
    return 90;
#endif
}