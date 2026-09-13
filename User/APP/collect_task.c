#include "collect_task.h"
#include "monitor_task.h"
#include "log_task.h"

#include "app_data.h"
#include "app_ipc.h"

#include "bsp_adc.h"

/**
  * @brief  采集任务：每100ms采集ADC数据，并将数据放入队列中
  * @note   None
  * @param  None
  * @retval None
  */
void collect_task(void* pvParameters)
{
  SensorData_t sensor_data;

  uint8_t function_ok ;

  while(1)
  {
    function_ok = 1;  //默认功能正常
    
    sensor_data.adc_value = BSP_ADC_GetValue();  //采集数据

    xQueueSend(sensor_queue, 
              &sensor_data,
              pdMS_TO_TICKS(10));  //将数据放入队列中

    /* 记录日志 */
    App_Log("CollectTask ADC = %u\r\n", sensor_data.adc_value);

    /* 业务功能是否正常报告 */
    if (sensor_data.adc_value > 4095U)
    {
        function_ok = 0;
    }
    MonitorTask_ReportFunction(TASK_ID_COLLECT, function_ok);

    /* 任务存活报告 */
    MonitorTask_ReportHeartbeat(TASK_ID_COLLECT);
    
    vTaskDelay(pdMS_TO_TICKS(100));  //延时100ms
  }
}

