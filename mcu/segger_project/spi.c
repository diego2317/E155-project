// spi.c
#include "spi.h"

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_rx;


static void SPI1_GPIO_Init(void);
static void SPI1_DMA_Init(void);

void SPI1_Init(void)
{
    __HAL_RCC_SPI1_CLK_ENABLE();
    SPI1_GPIO_Init();
    SPI1_DMA_Init();

    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;   // full duplex, RX used
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;     // SPI mode 0
    hspi1.Init.CLKPhase    = SPI_PHASE_1EDGE;
    hspi1.Init.NSS         = SPI_NSS_SOFT;         // manage CS in GPIO
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; // adjust
    hspi1.Init.FirstBit    = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode      = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial  = 7;
    hspi1.Init.NSSPMode    = SPI_NSS_PULSE_DISABLE;

    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        // Call your error handler as needed
        // Error_Handler();
    }
}

HAL_StatusTypeDef SPI1_Receive_DMA(uint8_t *pData, uint16_t Size)
{
    return HAL_SPI_Receive_DMA(&hspi1, pData, Size);
}

/* --- static helpers ------------------------------------------------------ */

static void SPI1_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* SPI1 SCK: PA5, MISO: PA6
       (MOSI PA7 is unused here; configure if your slave requires it) */
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void SPI1_DMA_Init(void)
{
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* Adjust DMA instance / request to match your STM32L4 RM / Cube config */
    hdma_spi1_rx.Instance = DMA2_Channel3;
    hdma_spi1_rx.Init.Request = DMA_REQUEST_1;
    hdma_spi1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_spi1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi1_rx.Init.Mode = DMA_NORMAL;           // or CIRCULAR
    hdma_spi1_rx.Init.Priority = DMA_PRIORITY_HIGH;

    if (HAL_DMA_Init(&hdma_spi1_rx) != HAL_OK) {
        // Error_Handler();
    }

    /* Link DMA to SPI RX */
    __HAL_LINKDMA(&hspi1, hdmarx, hdma_spi1_rx);

    /* NVIC config for DMA RX interrupt */
    HAL_NVIC_SetPriority(DMA2_Channel3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Channel3_IRQn);
}
