// ov7670.h
#ifndef OV7670_H
#define OV7670_H

#include "stm32l4xx_hal.h"

// OV7670 I2C Address
#define OV7670_WRITE_ADDR 0x42
#define OV7670_READ_ADDR  0x43

// Key registers
#define REG_PID   0x0A  // Product ID MSB (should be 0x76)
#define REG_VER   0x0B  // Product ID LSB (should be 0x73)
#define REG_COM7  0x12  // Common Control 7
#define COM7_RESET 0x80
#define COM7_QVGA  0x40
#define COM7_YUV   0x00

typedef struct {
    uint8_t reg;
    uint8_t value;
} camera_reg;

// Public API
HAL_StatusTypeDef OV7670_ReadReg(uint8_t reg, uint8_t *value);
HAL_StatusTypeDef OV7670_WriteReg(uint8_t reg, uint8_t value);
void OV7670_MinimalTest(void);
int  OV7670_Init(void);

#endif // OV7670_H
