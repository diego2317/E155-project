// main.h
#ifndef MAIN_H
#define MAIN_H

#include "stm32l4xx_hal.h"

// Expose handles defined in main.c so other modules can use them
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

void Error_Handler(void);

#endif // MAIN_H