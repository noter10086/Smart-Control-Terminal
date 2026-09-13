#include "main.h"
#include "bsp_usart.h"
#include <stdio.h>

extern UART_HandleTypeDef huart1;

void BSP_UART_SendByte(uint8_t data)
{
    HAL_UART_Transmit(&huart1, &data, 1, 100);
}

void BSP_UART_SendString(const char *str)
{
    while (*str != '\0')
    {
        BSP_UART_SendByte((uint8_t)*str);
        str++;
    }
}

int fputc(int ch, FILE *f)
{
    (void)f;

    BSP_UART_SendByte((uint8_t)ch);

    return ch;
}
