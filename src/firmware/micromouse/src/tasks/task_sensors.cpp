#include <Arduino.h>

#include "../sensors/sensors.h"
#include "../memory/sensor_memory.h"

void task_sensors(void *param)
{
    while (true)
    {
        sensors.front = (uint16_t)sensors_read_front();
        sensors.left  = (uint16_t)sensors_read_left();
        sensors.right = (uint16_t)sensors_read_right();

        sensors.battery_voltage = readVoltage();
        sensors.battery_current = readCurrent();
        sensors.battery_percentage = batteryPercentage(sensors.battery_voltage);

        sensors.rgb = readRGB();

        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}