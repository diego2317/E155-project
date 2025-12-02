// main.c
#include "main.h"
#include "config.h"
#include "ov7670.h"
#include "camera_capture.h"
#include "camera_vision.h"
#include <stdio.h>
#include <stdbool.h>

// Global Handles
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;
SPI_HandleTypeDef hspi1;

// Private Function Prototypes 
void SystemClock_Config(void);
void I2C1_Init(void);
void UART2_Init(void);
void XCLK_Init(void);
void GPIO_Capture_Init(void);
void SPI1_Init(void);
static void SPI1_GPIO_Init(void);
void LPTIM2_PWM_Init(void);

volatile bool spi_rx_error = false;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
   
    // Initialize peripherals
    //UART2_Init();
    I2C1_Init();
    GPIO_Capture_Init();
    SPI1_Init();
   
    //printf("\r\n=================================\r\n");
    //printf("STM32 Camera Config & Capture\r\n");
    //printf("Image: %dx%d (1-bit bitmask)\r\n", IMAGE_WIDTH, IMAGE_HEIGHT);
    //printf("=================================\r\n\r\n");
   
    // 1. Configure Camera
    //printf("Starting XCLK (10MHz on PA11)...\n");
    XCLK_Init();
    LPTIM2_PWM_Init();
    HAL_Delay(300);  
    //OV7670_Init_QVGA();
    HAL_Delay(300);
    uint8_t pid, ver;
    if (OV7670_ReadReg(0x0A, &pid) == HAL_OK && OV7670_ReadReg(0x0B, &ver) == HAL_OK) {
        //printf("? Camera detected (PID=0x%02X, VER=0x%02X)\n\n", pid, ver);
        if (OV7670_Init_QVGA() == 0) {
            //printf("? Configuration Success! Video streaming to FPGA.\r\n\r\n"); 
        } else {
            //printf("? Configuration failed! Halting.\r\n");
            while(1);
        }
    } else {
        //printf("? Camera not responding! Halting.\r\n");
        while(1);
    }
   
    // 2. Continuous Capture Loop 
    int avg[5];
    int thresh = 0;
    while (1)
    {
        ////printf("Waiting for next frame...\r\n");
        uint32_t start_time = HAL_GetTick();
        int32_t black_pixels = 0;
        capture_frame_spi();
       
        uint32_t capture_time = HAL_GetTick() - start_time;
        uint8_t toggle = 0;
        if (pixel_count >= 76000) {
            thresh = 0;
            ////printf("Frame captured! %d pixels in %d ms\r\n\r\n", pixel_count, capture_time); 
            // Analyze and display results
            //HAL_Delay(500);
            black_pixels = visualize_image_compact();
            if (black_pixels >= 60000) {
              // Black
              //printf("BLACK");
              HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 0);
            } else if (black_pixels <= 60000){
              // White
              //printf("WHITE");
              HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, 1);
            }
            //toggle = 0;
            //visualize_image_line_stats();
            //image_to_file();
            //determine_direction();
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
        } //else {
        //    ////printf("Capture Error: Only received %d pixels.\r\n", pixel_count);
        //    //HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 0);
        //}
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
    GPIO_InitStruct.Pin = FRAME_ACTIVE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN; // Keep lines low if FPGA is disconnected
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitTypeDef GPIO_InitStruc = {0};
    GPIO_InitStruc.Pin = GPIO_PIN_9;
    GPIO_InitStruc.Pull = GPIO_NOPULL;
    GPIO_InitStruc.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruc.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruc);

}

void SPI1_Init(void)
{
    __HAL_RCC_SPI1_CLK_ENABLE();
    SPI1_GPIO_Init();
    //SPI1_DMA_Init();

    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES_RXONLY;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;     // SPI mode 0
    hspi1.Init.CLKPhase    = SPI_PHASE_1EDGE;
    hspi1.Init.NSS         = SPI_NSS_SOFT;         // manage CS in GPIO
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi1.Init.FirstBit    = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode      = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial  = 7;
    hspi1.Init.NSSPMode    = SPI_NSS_PULSE_DISABLE;

    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        spi_rx_error = true;
    }
}

static void SPI1_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* SPI1 SCK: PB3, MISO: PB4
       (MOSI PA7 is unused in master RX-only mode) */
    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
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

void LPTIM2_PWM_Init(void)
{
    // Enable clocks
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;     // Enable GPIOA clock
    RCC->APB1ENR2 |= RCC_APB1ENR2_LPTIM2EN;  // Enable LPTIM2 clock
    
    // Configure PA8 as alternate function (AF14 for LPTIM2_OUT)
    GPIOA->MODER &= ~GPIO_MODER_MODE8_Msk;   // Clear mode bits
    GPIOA->MODER |= (0x2 << GPIO_MODER_MODE8_Pos); // Set to alternate function mode
    
    GPIOA->OTYPER &= ~GPIO_OTYPER_OT8;       // Output push-pull
    GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED8;  // High speed
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD8_Msk;   // No pull-up/pull-down
    
    // Set alternate function AF14 for PA8 (LPTIM2_OUT)
    GPIOA->AFR[1] &= ~GPIO_AFRH_AFSEL8_Msk;  // Clear AF bits
    GPIOA->AFR[1] |= (14 << GPIO_AFRH_AFSEL8_Pos); // AF14
    
    // Configure LPTIM2
    // Select LSI as clock source (if using LSI, ~32kHz)
    // Or use PCLK for higher frequency
    RCC->CCIPR |= (0x0 << RCC_CCIPR_LPTIM2SEL_Pos); // PCLK as clock source
    
    // Disable LPTIM2 before configuration
    LPTIM2->CR &= ~LPTIM_CR_ENABLE;
    
    // Configure LPTIM2 in PWM mode
    // Set prescaler: /1 (no division)
    LPTIM2->CFGR &= ~LPTIM_CFGR_PRESC_Msk;
    LPTIM2->CFGR |= (0x0 << LPTIM_CFGR_PRESC_Pos); // Prescaler /1
    
    // Configure wave polarity and mode
    LPTIM2->CFGR &= ~LPTIM_CFGR_WAVPOL;      // PWM mode, output high when CNT < CMP
    LPTIM2->CFGR &= ~LPTIM_CFGR_PRELOAD;     // Registers updated immediately
    
    // Enable LPTIM2
    LPTIM2->CR |= LPTIM_CR_ENABLE;
    
    // Set ARR (Auto-Reload Register) - defines PWM period
    // For example: ARR = 999 gives 1000 counts (0-999)
    LPTIM2->ARR = 999;
    
    // Set CMP (Compare Register) - defines duty cycle
    // For 10% duty cycle: CMP = 0.1 * 1000 = 100
    LPTIM2->CMP = 499;  // 100/1000 = 10% duty cycle
    
    // Start continuous mode
    LPTIM2->CR |= LPTIM_CR_CNTSTRT;
}
