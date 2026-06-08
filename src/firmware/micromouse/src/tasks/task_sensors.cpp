#include <Arduino.h>

#include "../sensors/sensors.h"
#include "../memory/sensor_memory.h"

void task_sensors(void *param)
{
    while (true)
    {
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}