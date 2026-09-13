#include "bsp.h"
#include "bsp_adc.h"
#include "bsp_pwm.h"

void BSP_Init(void)
{
    BSP_ADC_Start();
    BSP_PWM_Start();
}

