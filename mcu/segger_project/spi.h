// spi.h
#ifndef SPI_H
#define SPI_H

#include "stm32l4xx_hal.h"

extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_rx;

/* Init SPI1 as master with RX DMA configured */
void SPI1_Init(void);

/* Start a DMA RX transfer (master generates clock, RX fills buffer) */
HAL_StatusTypeDef SPI1_Receive_DMA(uint8_t *pData, uint16_t Size);

#endif // SPI_H
