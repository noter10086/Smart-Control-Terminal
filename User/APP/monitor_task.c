#include "monitor_task.h"

static TaskMonitorInfo_t task_monitor[TASK_ID_MAX];

void MonitorTask_Init(void)
{
    TickType_t now = xTaskGetTickCount();

    /* CollectTask */
    task_monitor[TASK_ID_COLLECT].heartbeat = 0;
    task_monitor[TASK_ID_COLLECT].last_tick = now;
    task_monitor[TASK_ID_COLLECT].heartbeat_timeout = pdMS_TO_TICKS(500);

    task_monitor[TASK_ID_COLLECT].function_ok = 1;
    task_monitor[TASK_ID_COLLECT].last_function_tick = now;
    task_monitor[TASK_ID_COLLECT].function_timeout = pdMS_TO_TICKS(500);

    /* ControlTask */
    task_monitor[TASK_ID_CONTROL].heartbeat = 0;
    task_monitor[TASK_ID_CONTROL].last_tick = now;
    task_monitor[TASK_ID_CONTROL].heartbeat_timeout = pdMS_TO_TICKS(500);

    task_monitor[TASK_ID_CONTROL].function_ok = 1;
    task_monitor[TASK_ID_CONTROL].last_function_tick = now;
    task_monitor[TASK_ID_CONTROL].function_timeout = pdMS_TO_TICKS(500);

    /* LogTask */
    task_monitor[TASK_ID_LOG].heartbeat = 0;
    task_monitor[TASK_ID_LOG].last_tick = now;
    task_monitor[TASK_ID_LOG].heartbeat_timeout = pdMS_TO_TICKS(1000);

    task_monitor[TASK_ID_LOG].function_ok = 1;
    task_monitor[TASK_ID_LOG].last_function_tick = now;
    task_monitor[TASK_ID_LOG].function_timeout = pdMS_TO_TICKS(1000);
}

void MonitorTask_ReportHeartbeat(TaskMonitorId_t task_id)
{
    if (task_id < TASK_ID_MAX)
    {
        task_monitor[task_id].heartbeat++;

        task_monitor[task_id].last_tick = xTaskGetTickCount();
    }
}

void MonitorTask_ReportFunction(TaskMonitorId_t task_id, uint8_t function_ok)
{
    if (task_id < TASK_ID_MAX)
    {
        task_monitor[task_id].function_ok = function_ok;

        task_monitor[task_id].last_function_tick = xTaskGetTickCount();
    }
}

uint8_t MonitorTask_IsAllHealthy(void)
{
    TickType_t now = xTaskGetTickCount();

    uint32_t i;

    for (i = 0; i < TASK_ID_MAX; i++)
    {
        /*
         * 1. Liveness 检查：
         *    任务是否在规定时间内报告心跳？
         */
        if ((now - task_monitor[i].last_tick) >
            task_monitor[i].heartbeat_timeout)
        {
            return 0;
        }

        /*
         * 2. Functionality 检查：
         *    最近一次业务状态是否正常？
         */
        if (task_monitor[i].function_ok == 0)
        {
            return 0;
        }

        /*
         * 3. Functionality 新鲜度检查：
         *    功能状态是否长期没有更新？
         */
        if ((now - task_monitor[i].last_function_tick) >
            task_monitor[i].function_timeout)
        {
            return 0;
        }
    }

    return 1;
}

