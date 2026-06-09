#include <Arduino.h>

#include "src/config/config.h"

#include "src/sensors/sensors.h"
#include "src/motors/motors.h"

#include "src/communication/comm.h"

#include "src/tasks/task_sensors.h"
#include "src/tasks/task_control.h"
#include "src/tasks/task_comm.h"



void setup()
{
    Serial.begin(115200);

    sensors_init();
    motors_init();
    comm_init();

    xTaskCreatePinnedToCore(
        task_sensors,
        "task_sensors",
        TASK_STACK_SMALL,
        NULL,
        TASK_PRIORITY_HIGH,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        task_control,
        "task_control",
        TASK_STACK_MEDIUM,
        NULL,
        TASK_PRIORITY_HIGH,
        NULL,
        1);

    xTaskCreatePinnedToCore(
        task_comm,
        "task_comm",
        TASK_STACK_SMALL,
        NULL,
        TASK_PRIORITY_LOW,
        NULL,
        0);

    Serial.println("Micromouse firmware iniciado");
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
}

