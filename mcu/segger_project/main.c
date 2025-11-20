// main.c
#include "main.h"
#include "config.h"
#include "ov7670.h"
#include "camera_capture.h"
#include "camera_vision.h"
#include <stdio.h>

// Global Handles
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

// Private Function Prototypes 
void SystemClock_Config(void);
void I2C1_Init(void);
void UART2_Init(void);
void XCLK_Init(void);
void GPIO_Capture_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
   
    // Initialize peripherals
    UART2_Init();
    I2C1_Init();
    GPIO_Capture_Init();
   
    printf("\r\n=================================\r\n");
    printf("STM32 Camera Config & Capture\r\n");
    printf("Image: %dx%d (1-bit bitmask)\r\n", IMAGE_WIDTH, IMAGE_HEIGHT);
    printf("=================================\r\n\r\n");
   
    // 1. Configure Camera
    printf("Starting XCLK (10MHz on PA11)...\n");
    XCLK_Init();
    HAL_Delay(300);  
    OV7670_Init_QVGA();
    HAL_Delay(300);
    uint8_t pid, ver;
    if (OV7670_ReadReg(0x0A, &pid) == HAL_OK && OV7670_ReadReg(0x0B, &ver) == HAL_OK) {
        printf("? Camera detected (PID=0x%02X, VER=0x%02X)\n\n", pid, ver);
        if (OV7670_Init_QVGA() == 0) {
            printf("? Configuration Success! Video streaming to FPGA.\r\n\r\n"); 
        } else {
            printf("? Configuration failed! Halting.\r\n");
            while(1);
        }
    } else {
        printf("? Camera not responding! Halting.\r\n");
        while(1);
    }
   
    // 2. Continuous Capture Loop 
    while (1)
    {
        //printf("Waiting for next frame...\r\n");
        uint32_t start_time = HAL_GetTick();
       
        capture_frame();
       
        uint32_t capture_time = HAL_GetTick() - start_time;
        if (pixel_count >= 76000) {
            printf("Frame captured! %d pixels in %d ms\r\n\r\n", pixel_count, capture_time); 
            // Analyze and display results
            visualize_image_compact();
            //visualize_image_line_stats();
           // HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
        } else {
            printf("Capture Error: Only received %d pixels.\r\n", pixel_count);
           // HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
        }
       
        //HAL_Delay(500);
    }
}

// ============================================================================
// Hardware Initialization Functions
// ============================================================================

void XCLK_Init(void) {
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(GPIOA, &gpio); 

    TIM_HandleTypeDef htim1 = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 0;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 7; // For 10MHz XCLK @ 80MHz SYSCLK
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim1);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 4; // 50% duty cycle
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4);

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
}

void I2C1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
   
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
   
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x10909CEC; // Fast Mode
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
   
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}

void UART2_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
   
    // UART TX (PA2) and RX (PA15)
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
   
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
   
    HAL_UART_Init(&huart2);
}

void GPIO_Capture_Init(void) {
   GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Only GPIOA Clock needed for these pins
    __HAL_RCC_GPIOA_CLK_ENABLE();
   
    // Configure all 4 pins as Inputs
    GPIO_InitStruct.Pin = PCLK_PIN | DATA_VALID_PIN | FRAME_ACTIVE_PIN | PIXEL_DATA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN; // Keep lines low if FPGA is disconnected
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitTypeDef GPIO_InitStruc = {0};
    GPIO_InitStruc.Pin = GPIO_PIN_3;
    GPIO_InitStruc.Pull = GPIO_NOPULL;
    GPIO_InitStruc.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruc.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruc);

}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = 0;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI; 
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 40;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}

int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, 1000);
    return len;
}