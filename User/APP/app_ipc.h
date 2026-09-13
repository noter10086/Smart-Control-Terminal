#ifndef __APP_IPC_H
#define __APP_IPC_H

#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"

#define EVENT_ADC_READY       (1U << 0)
#define EVENT_PWM_READY       (1U << 1)
#define EVENT_SYSTEM_READY    (1U << 2)
#define EVENT_FAULT           (1U << 3)

extern QueueHandle_t sensor_queue;

extern QueueHandle_t log_queue;

extern EventGroupHandle_t system_event_group;

void App_IPC_Init(void);

#endif

