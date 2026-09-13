#include "bsp_adc.h"
#include "adc.h"


static uint16_t adc_value = 0;

void BSP_ADC_Start(void)
{
    HAL_ADC_Start_DMA(&hadc1,
                      (uint32_t *)&adc_value,
                      1);
}

uint16_t BSP_ADC_GetValue(void)
{
    return adc_value;
}


