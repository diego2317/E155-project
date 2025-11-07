/* sccb.h */
#pragma once
#include "stm32l4xx_hal.h"

typedef struct {
  I2C_HandleTypeDef hi2c;
  uint8_t dev7;     // 7-bit address (e.g., 0x21 for OV7670)
} sccb_t;

HAL_StatusTypeDef SCCB_Init_I2C1(sccb_t *bus, uint32_t timing);
HAL_StatusTypeDef SCCB_Read8 (sccb_t *bus, uint8_t reg, uint8_t *val);
HAL_StatusTypeDef SCCB_Write8(sccb_t *bus, uint8_t reg, uint8_t val);
