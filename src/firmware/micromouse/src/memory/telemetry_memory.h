#ifndef TELEMETRY_MEMORY_H
#define TELEMETRY_MEMORY_H

#include <stdint.h>

typedef struct
{
    uint32_t loops;
    uint32_t errors;
} TelemetryData;

extern TelemetryData telemetry;

#endif