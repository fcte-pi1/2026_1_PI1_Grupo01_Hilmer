#ifndef COMM_H
#define COMM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void comm_init(void);

    void comm_send(const char *msg);

    void comm_send_value(const char *label, int32_t value);

#ifdef __cplusplus
}
#endif

#endif