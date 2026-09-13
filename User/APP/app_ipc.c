#include "app_ipc.h"
#include "app_data.h"

#include "log_task.h"


QueueHandle_t sensor_queue = NULL;

QueueHandle_t log_queue = NULL;

EventGroupHandle_t system_event_group = NULL;

void App_IPC_Init(void)
{
    sensor_queue = xQueueCreate(10, sizeof(SensorData_t));
    log_queue = xQueueCreate(10, sizeof(LogMessage_t));

    system_event_group = xEventGroupCreate();

    if (sensor_queue == NULL || log_queue == NULL || system_event_group == NULL)
    {
        /* IPC´´½¨Ê§°Ü */
        while (1)
        {
            App_Log("IPC creation failed!\r\n");
        }
    }
}

