// main.h
#ifndef MAIN_H
#define MAIN_H

#include "stm32l4xx_hal.h"
#include <stdio.h>

#define PCLK_PIN GPIO_PIN_3
#define DATA_VALID_PIN GPIO_PIN_4
#define PIXEL_DATA_PIN GPIO_PIN_5
#define FRAME_ACTIVE_PIN GPIO_PIN_11

// System-level functions
void SystemClock_Config(void);
void Error_Handler(void);

#endif // MAIN_H
