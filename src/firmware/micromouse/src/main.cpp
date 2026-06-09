#include <Arduino.h>

#include "config/config.h"

#include "sensors/sensors.h"
#include "motors/motors.h"

#include "communication/comm.h"

#include "tasks/task_sensors.h"
#include "tasks/task_control.h"
#include "tasks/task_comm.h"

void sensors_init()
{
#ifdef ARDUINO
    // Inicialização dos sensores
#endif
}

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
