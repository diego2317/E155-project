/* board_init.h */
#pragma once
#include "stm32l4xx_hal.h"

void Board_SystemClock_Config(void);
void Board_InitLED(void);
void Board_LED_Toggle(void);
