#include "app_freertos.h"

/* app相关文件 */
#include "app_data.h"
#include "app_ipc.h"
#include "app_timer.h"


/* task相关文件 */
#include "monitor_task.h"
#include "collect_task.h"
#include "control_task.h"
#include "log_task.h"
#include "watchdog_task.h"


/*start_task的配置*/
#define START_TASK_STACK 128
#define START_TASK_PRIORITY 1
static TaskHandle_t start_task_handle;


/*collect_task的配置*/
#define COLLECT_TASK_STACK 128
#define COLLECT_TASK_PRIORITY 4
static TaskHandle_t collect_task_handle;
/*control_task的配置*/
#define CONTROL_TASK_STACK 128
#define CONTROL_TASK_PRIORITY 5
static TaskHandle_t control_task_handle;
/*log_task的配置*/
#define LOG_TASK_STACK 128
#define LOG_TASK_PRIORITY 2
static TaskHandle_t log_task_handle;
/* watchdog_task的配置 */
#define WATCHDOG_TASK_STACK 128
#define WATCHDOG_TASK_PRIORITY 3
static TaskHandle_t watchdog_task_handle;


/* 函数声明 */
void start_task(void* pvParameters);

/**
  * @brief  启动FreeRTOS
  * @note   None
  * @param  None
  * @retval None
  */
void freertos_start(void)
{
  /* 创建一个启动任务*/
  xTaskCreate( (TaskFunction_t) start_task,               //任务函数的地址
              (char *) "start_task",                       //任务名字字符串
              (configSTACK_DEPTH_TYPE) START_TASK_STACK,   //任务栈大小
              (void *) NULL,                               //传递给任务的参数
              (UBaseType_t) START_TASK_PRIORITY,           //任务优先级
              (TaskHandle_t *)  &start_task_handle );      //任务句柄的地址
  /* 启动任务调度器，会自动创建空闲函数*/
  vTaskStartScheduler();
}

/**
  * @brief  启动任务:用来创建其他task
  * @note   None
  * @param  None
  * @retval None
  */
void start_task(void* pvParameters)
{
  taskENTER_CRITICAL();

  /* 1. 初始化任务监测模块 */
  MonitorTask_Init();

  /* 2. 初始化IPC */
  App_IPC_Init();

  /* 3. 启动软件定时器 */
  App_Timer_Init();

  /* 4. 设置系统事件标志 */
  xEventGroupSetBits(
        system_event_group,
        EVENT_ADC_READY |
        EVENT_PWM_READY |
        EVENT_SYSTEM_READY
    );

  /* 5. 创建其他任务 */
  BaseType_t xReturn = xTaskCreate( (TaskFunction_t) log_task,       
              (char *) "log_task",                     
              (configSTACK_DEPTH_TYPE) LOG_TASK_STACK, 
              (void *) NULL,                           
              (UBaseType_t) LOG_TASK_PRIORITY,           
              (TaskHandle_t *)  &log_task_handle );
  if(xReturn != pdPASS)
  {
    while(1)
    {
      App_Log("log_task create failed!\r\n");
    }
  }


  xReturn = xTaskCreate( (TaskFunction_t) collect_task,
              (char *) "collect_task",
              (configSTACK_DEPTH_TYPE) COLLECT_TASK_STACK,
              (void *) NULL,
              (UBaseType_t) COLLECT_TASK_PRIORITY,
              (TaskHandle_t *) &collect_task_handle);
  if(xReturn != pdPASS)
  {
    while(1)
    {
      App_Log("collect_task create failed!\r\n");
    }
  }

  xReturn = xTaskCreate( (TaskFunction_t) control_task,       
              (char *) "control_task",                     
              (configSTACK_DEPTH_TYPE) CONTROL_TASK_STACK, 
              (void *) NULL,                           
              (UBaseType_t) CONTROL_TASK_PRIORITY,           
              (TaskHandle_t *)  &control_task_handle );   
  if(xReturn != pdPASS)
  {
    while(1)
    {
      App_Log("control_task create failed!\r\n");
    }
  }

  xReturn = xTaskCreate( (TaskFunction_t) watchdog_task,       
              (char *) "watchdog_task",                     
              (configSTACK_DEPTH_TYPE) WATCHDOG_TASK_STACK, 
              (void *) NULL,                           
              (UBaseType_t) WATCHDOG_TASK_PRIORITY,           
              (TaskHandle_t *)  &watchdog_task_handle );
  if(xReturn != pdPASS)
  {
    while(1)
    {
      App_Log("watchdog_task create failed!\r\n");
    }
  }

  /*退出临界区*/
  taskEXIT_CRITICAL();

  /*启动任务只需执行一次，用完删除自身*/
  vTaskDelete(NULL);
}

