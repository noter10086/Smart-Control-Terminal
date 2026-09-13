#include "FreeRTOS.h"
#include "task.h"

#include "watchdog_task.h"
#include "bsp_iwdg.h"
#include "monitor_task.h"

void watchdog_task(void *pvParameters)
{
    while (1)
    {
        if (MonitorTask_IsAllHealthy())
        {
            /*
             * 所有关键任务都正常：
             * 允许喂狗。
             */
            BSP_IWDG_Refresh();
        }
        else
        {
            /*
             * 至少有一个关键任务异常。
             *
             * 故意不喂狗。
             *
             * 等待 IWDG 超时复位 MCU。
             */
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

