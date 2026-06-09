#include "hal_comm.h"

#ifdef ARDUINO

#include <Arduino.h>

void hal_comm_begin()
{
    Serial.begin(115200);
}

void hal_comm_send(const char *msg)
{
    Serial.println(msg);
}

#endif
