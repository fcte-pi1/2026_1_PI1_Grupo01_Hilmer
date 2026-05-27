#include "sim_comm.h"

#include <stdio.h>

void hal_comm_begin()
{
    printf("SIM COMM START\n");
}

void hal_comm_send(const char *msg)
{
    printf("[SIM]: %s\n", msg);
}