#include "task_control.h"
#include "../navigation/floodfill.h"

#ifdef TARGET_ESP32
#include <Arduino.h>
#endif

void task_control(void *param)
{
    floodfill_init();

    while (true)
    {
        floodfill_step();

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}