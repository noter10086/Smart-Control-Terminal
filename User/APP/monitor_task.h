#ifndef __MONITOR_TASK_H
#define __MONITOR_TASK_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

typedef enum
{
    TASK_ID_COLLECT = 0,
    TASK_ID_CONTROL,
    TASK_ID_LOG,
    TASK_ID_MAX
} TaskMonitorId_t;

typedef struct
{
    /* Liveness：任务存活监测 */
    uint32_t heartbeat;
    TickType_t last_tick;
    TickType_t heartbeat_timeout;

    /* Functionality：任务功能监测 */
    uint8_t function_ok;
    TickType_t last_function_tick;
    TickType_t function_timeout;

} TaskMonitorInfo_t;

void MonitorTask_Init(void);

void MonitorTask_ReportHeartbeat(TaskMonitorId_t task_id);

void MonitorTask_ReportFunction(TaskMonitorId_t task_id,
                                uint8_t function_ok);

uint8_t MonitorTask_IsAllHealthy(void);

#endif

