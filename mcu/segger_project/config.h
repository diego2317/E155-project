// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include "stm32l4xx_hal.h"

// ============================================================================
// Pin Definitions
// ============================================================================
// OV7670 I2C Address
#define OV7670_WRITE_ADDR 0x42
#define OV7670_READ_ADDR  0x43

// Camera control pins for the streaming interface from FPGA (Input to MCU)
// PIXEL_DATA_PIN MOVED TO PA4 TO AVOID UART TX (PA2) CONFLICT
#define PCLK_PIN           GPIO_PIN_9  // PA9
#define DATA_VALID_PIN     GPIO_PIN_5  // PA5, Corresponds to FPGA's cam_wr_en
#define FRAME_ACTIVE_PIN   GPIO_PIN_10  // PA10 Corresponds to FPGA's in_frame
#define PIXEL_DATA_PIN     GPIO_PIN_6  // PA6, Corresponds to FPGA's cam_wr_data

// ============================================================================
// Image Dimensions
// ============================================================================
// QVGA YUV 4:2:2 -> 1-bit per pixel
#define IMAGE_WIDTH        320
#define IMAGE_HEIGHT       240
#define IMAGE_SIZE_PIXELS  76800
#define IMAGE_SIZE_BYTES   9600 // (320 * 240) / 8
#define DMA_RAW_BUFFER_SIZE  2048

#endif // CONFIG_H