#include "gpio.h"
#include "main.h"

void GPIO_Capture_Init(void) {
    // Initializes the input pins for the data stream from the FPGA
    GPIO_InitTypeDef GPIO_InitStruct = {0};
   
    __HAL_RCC_GPIOA_CLK_ENABLE();
   
    // Configure input pins: PA0 (PCLK), PA1 (Data Valid), PA3 (Frame Active), PA4 (Pixel Data)
    GPIO_InitStruct.Pin = PCLK_PIN | DATA_VALID_PIN | PIXEL_DATA_PIN | FRAME_ACTIVE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}