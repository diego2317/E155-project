// spi.h
#ifndef SPI_H
#define SPI_H

#include "stm32l4xx_hal.h"
#include <stdbool.h>

#define SPI_RX_BUFFER_BYTES 9600U

extern SPI_HandleTypeDef hspi1;
extern DMA_HandleTypeDef hdma_spi1_rx;

extern volatile bool spi_rx_half_complete;
extern volatile bool spi_rx_full_complete;
extern volatile bool spi_rx_error;

/* Init SPI1 as master with RX DMA configured */
void SPI1_Init(void);

/* Start a DMA RX transfer (master generates clock, RX fills buffer) */
HAL_StatusTypeDef SPI1_Receive_DMA(uint8_t *pData, uint16_t Size);

/* Stop an in-flight DMA RX transfer */
void SPI1_Stop_DMA(void);

#endif // SPI_H
