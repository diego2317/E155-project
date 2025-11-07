/* xclk.h */
#pragma once
#include "stm32l4xx_hal.h"
HAL_StatusTypeDef XCLK_Start(uint32_t freq_hz, float duty);
