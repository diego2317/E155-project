#include "gpio.h"
#include "main.h"

void GPIO_Capture_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. Enable Clocks for both Port A and Port B
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // --- Configure Port A: PA10 (Pixel Data) ---
    GPIO_InitStruct.Pin = FRAME_ACTIVE_PIN; 
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // --- Configure Port B: PB3 (PCLK), PB4 (Data Valid), PB5 (Frame Active) ---
    // Note: PB3 is often associated with SWO/JTDO. Ensure debug features 
    // aren't conflicting in your specific MCU configuration.
    GPIO_InitStruct.Pin = PCLK_PIN | DATA_VALID_PIN | PIXEL_DATA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}