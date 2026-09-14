#include "log_task.h"
#include "monitor_task.h"

#include "app_data.h"
#include "app_ipc.h"


/**
  * @brief  日志任务：接收日志消息并处理
  * @note   None
  * @param  None
  * @retval None
  */
void log_task(void* pvParameters)
{
    LogMessage_t log_message;

    while(1)
    {
        if (xQueueReceive(log_queue, &log_message, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            /* 处理日志消息 */
            if (printf("%s", log_message.message) > 0)
            {
                /* 日志输出成功 */
                MonitorTask_ReportFunction(TASK_ID_LOG, 1);
            }
            else
            {
                /* 日志输出失败 */
                MonitorTask_ReportFunction(TASK_ID_LOG, 0);
            }

        }

        /* 任务存活报告 */
        MonitorTask_ReportHeartbeat(TASK_ID_LOG);

    }
}

void App_Log(const char *format, ...)
{
    LogMessage_t log_message;

    va_list args;

    va_start(args, format);

    vsnprintf(log_message.message,
              sizeof(log_message.message),
              format,
              args);

    va_end(args);

    xQueueSend(log_queue,
               &log_message,
               pdMS_TO_TICKS(10));
}

