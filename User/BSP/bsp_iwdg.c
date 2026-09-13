#include "bsp_iwdg.h"

#include "iwdg.h"


void BSP_IWDG_Refresh(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}

uint8_t BSP_IWDG_ResetOccurred(void)
{
    return (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET);
}

void BSP_IWDG_ClearResetFlags(void)
{
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

