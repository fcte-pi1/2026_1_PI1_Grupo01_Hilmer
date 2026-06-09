#include "task_comm.h"

#include "comm/comm.h"

void task_comm(void *pvParameters)
{
    while (1)
    {
        comm_send("Sistema ativo");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
