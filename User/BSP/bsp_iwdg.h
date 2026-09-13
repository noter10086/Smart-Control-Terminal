#ifndef __BSP_IWDG_H
#define __BSP_IWDG_H

#include <stdint.h>

void BSP_IWDG_Refresh(void);
uint8_t BSP_IWDG_ResetOccurred(void);
void BSP_IWDG_ClearResetFlags(void);

#endif

