#ifndef __DATA_APP_H
#define __DATA_APP_H

#include <stdint.h>

typedef struct
{
    uint16_t adc_value;
}SensorData_t;

typedef struct
{
    char message[64];
} LogMessage_t;

#endif

