// main.h
#ifndef MAIN_H
#define MAIN_H

#include "stm32l4xx_hal.h"
#include "config.h"
#include "ov7670.h"
#include "camera_capture.h"
#include "camera_vision.h"
#include <stdio.h>
#include <stdbool.h>
// Expose handles defined in main.c so other modules can use them
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;
SPI_HandleTypeDef hspi1;

void Error_Handler(void);

#endif // MAIN_H