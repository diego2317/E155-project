
//#include "stm32l4xx_hal.h"
//#include <stdio.h>
//// OV7670 Address
//#define OV7670_WRITE_ADDR 0x42
//#define OV7670_READ_ADDR  0x43

//// Key registers
//#define REG_PID   0x0A  // Product ID MSB (should be 0x76)
//#define REG_VER   0x0B  // Product ID LSB (should be 0x73)
//#define REG_COM7  0x12  // Common Control 7
//#define COM7_RESET 0x80
//#define COM7_QVGA  0x40
//#define COM7_YUV   0x00

//// Camera register configuration
//typedef struct {
//    uint8_t reg;
//    uint8_t value;
//} camera_reg;

//// Basic QVGA YUV422 configuration
//const camera_reg ov7670_config[] = {
//    {REG_COM7, COM7_RESET},  // Reset
//    {0xFF, 100},              // Delay 100ms
//    {0x11, 0x01},  // Clock prescaler
//    {REG_COM7, COM7_YUV | COM7_QVGA},  // QVGA YUV
//    {0x32, 0x80},  // HREF
//    {0x17, 0x16},  // HSTART
//    {0x18, 0x04},  // HSTOP
//    {0x19, 0x02},  // VSTART
//    {0x1A, 0x7A},  // VSTOP
//    {0x03, 0x0A},  // VREF
//    {0x70, 0x3A},  // X scaling
//    {0x71, 0x35},  // Y scaling
//    {0x72, 0x11},  // Downsample by 2
//    {0x73, 0xF1},  // Clock divide
//    {0x15, 0x00},  // COM10
//    {0x3A, 0x04},  // TSLB
//    {0x12, 0x00},  // COM7
//    {0x8C, 0x00},  // RGB444
//    {0x04, 0x00},  // COM1
//    {0x40, 0xC0},  // COM15
//    {0x14, 0x48},  // COM9
//    {0x4F, 0x80},  // MTX1
//    {0x50, 0x80},  // MTX2
//    {0x51, 0x00},  // MTX3
//    {0x52, 0x22},  // MTX4
//    {0x53, 0x5E},  // MTX5
//    {0x54, 0x80},  // MTX6
//    {0x58, 0x9E},  // MTXS
//    {0xFF, 0xFF}   // End marker
//};

//extern I2C_HandleTypeDef hi2c1;
//extern UART_HandleTypeDef huart2;

//void I2C1_init(void);

//void UART2_Init(void);

//int _write(int file, char *ptr, int len);

//HAL_StatusTypeDef OV7670_ReadReg(uint8_t reg, uint8_t *value);

//HAL_StatusTypeDef OV7670_WriteReg(uint8_t reg, uint8_t value);

//void OV7670_MinimalTest(void);

//void SystemClock_Config(void);

//int OV7670_Init(void);

//void Error_Handler(void);

//void XCLK_Init(void);