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
#define COM7_QVGA  0x10  // CORRECTED: Bit[4] for QVGA
#define COM7_YUV   0x00

// Camera control pins
#define CAM_VSYNC_PIN  GPIO_PIN_8
#define CAM_VSYNC_PORT GPIOA
#define CAM_HREF_PIN   GPIO_PIN_9
#define CAM_HREF_PORT  GPIOA
#define CAM_PCLK_PIN   GPIO_PIN_10
#define CAM_PCLK_PORT  GPIOA

// Register configuration structure
typedef struct {
    uint8_t reg;
    uint8_t value;
    uint16_t delay_ms;
    char description[35];
} camera_reg;


// Public API
HAL_StatusTypeDef OV7670_ReadReg(uint8_t reg, uint8_t *value);
HAL_StatusTypeDef OV7670_WriteReg(uint8_t reg, uint8_t value);
int  OV7670_Init_QVGA(void);
void Camera_ControlPins_Init(void);
void Camera_SignalTest(void);
void Camera_MeasureFrameRate_VSYNC(void);
void Camera_MeasureFrameRate(void);
void Camera_ShowCurrentConfig(void);
void Camera_VerifyFormat(void);

#endif // OV7670_H
