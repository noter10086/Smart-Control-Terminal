#include "bsp_pwm.h"
#include "tim.h"

#define PWM_MAX_DUTY    100U

void BSP_PWM_Start(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

    BSP_PWM_SetDuty(0);
}

void BSP_PWM_SetDuty(uint8_t duty)
{
    uint32_t compare_value;

    if (duty > PWM_MAX_DUTY)
    {
        duty = PWM_MAX_DUTY;
    }

    compare_value = ((htim3.Init.Period + 1U) * duty) / 100U;

    __HAL_TIM_SET_COMPARE(&htim3,
                          TIM_CHANNEL_1,
                          compare_value);
}

