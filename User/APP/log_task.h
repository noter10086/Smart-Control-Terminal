#ifndef __LOG_TASK_H__
#define __LOG_TASK_H__

#include <stdio.h>
#include <stdarg.h>

void log_task(void* pvParameters);
void App_Log(const char *format, ...);

#endif
