#include "control_task.h"
#include "monitor_task.h"
#include "log_task.h"

#include "app_data.h"
#include "app_ipc.h"

#include "bsp_pwm.h"

/**
  * @brief  控制任务：根据传感器数据调整PWM占空比
  * @note   None
  * @param  None
  * @retval None
  */
void control_task(void* pvParameters)
{
  SensorData_t sensor_data;
  
  uint8_t duty;
  uint8_t function_ok;

  xEventGroupWaitBits(
        system_event_group,
        EVENT_ADC_READY |
        EVENT_PWM_READY |
        EVENT_SYSTEM_READY,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY
    );

  while(1)
  {
    function_ok = 1;  //默认功能正常
    
    if (xQueueReceive(sensor_queue, &sensor_data, pdMS_TO_TICKS(100)) == pdTRUE)
    {

      duty = (uint8_t)((sensor_data.adc_value * 100U) / 4095U);
      
      if (duty > 100U)
      {
        /* 业务处理失败 */
        function_ok = 0;
      }
      else
      {
        /* 业务处理成功 */  
        function_ok = 1;
      }
      BSP_PWM_SetDuty(duty);

      /* 记录日志 */
      App_Log("ControlTask PWM = %u%%\r\n", duty);

      /* 业务功能是否正常报告 */
      MonitorTask_ReportFunction(TASK_ID_CONTROL, function_ok);

    }
    /* 任务存活报告 */
    MonitorTask_ReportHeartbeat(TASK_ID_CONTROL);
      
  }
}

