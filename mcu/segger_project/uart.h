// uart.h
#ifndef UART_H
#define UART_H

#include "stm32l4xx_hal.h"

extern UART_HandleTypeDef huart2;

void UART2_Init(void);

#endif // UART_H
