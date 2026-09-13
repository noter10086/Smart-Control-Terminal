#include "app_timer.h"
#include "app_data.h"
#include "app_ipc.h"

#include "log_task.h"

#include "gpio.h"
#include <stdio.h>


static TimerHandle_t heartbeat_timer;
static TimerHandle_t log_timer;


static void HeartbeatTimerCallback(TimerHandle_t xTimer)
{
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}


static void LogTimerCallback(TimerHandle_t xTimer)
{
    App_Log("system alive\r\n");
}

void App_Timer_Init(void)
{
    heartbeat_timer = xTimerCreate(
        "HeartbeatTimer",
        pdMS_TO_TICKS(500),
        pdTRUE,
        NULL,
        HeartbeatTimerCallback
    );

    log_timer = xTimerCreate(
        "LogTimer",
        pdMS_TO_TICKS(2000),
        pdTRUE,
        NULL,
        LogTimerCallback
    );

    if ((heartbeat_timer == NULL) || (log_timer == NULL))
    {
        /* Timer´´½¨Ê§°Ü */
        while (1)
        {
            App_Log("Timer creation failed!\r\n");
        }
    }

    xTimerStart(heartbeat_timer, 0);
    xTimerStart(log_timer, 0);
}


