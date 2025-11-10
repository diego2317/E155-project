// Complete MCU Test Code for OV7670 Camera
// STM32L432KC - Tests I2C communication and configuration

#include "stm32l4xx_hal.h"
#include <stdio.h>
// Function prototypes
void SystemClock_Config(void);
void Error_Handler(void);
void I2C1_Init(void);
void UART2_Init(void);
HAL_StatusTypeDef OV7670_ReadReg(uint8_t reg, uint8_t *value);
HAL_StatusTypeDef OV7670_WriteReg(uint8_t reg, uint8_t value);
void OV7670_MinimalTest(void);
int OV7670_Init(void);

// OV7670 I2C Address
#define OV7670_WRITE_ADDR 0x42
#define OV7670_READ_ADDR  0x43

// Key registers
#define REG_PID   0x0A  // Product ID MSB (should be 0x76)
#define REG_VER   0x0B  // Product ID LSB (should be 0x73)
#define REG_COM7  0x12  // Common Control 7
#define COM7_RESET 0x80
#define COM7_QVGA  0x40
#define COM7_YUV   0x00

// Handles
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

// Camera register configuration
typedef struct {
    uint8_t reg;
    uint8_t value;
} camera_reg;

// Basic QVGA YUV422 configuration
const camera_reg ov7670_config[] = {
    {REG_COM7, COM7_RESET},  // Reset
    {0xFF, 100},              // Delay 100ms
    {0x11, 0x01},  // Clock prescaler
    {REG_COM7, COM7_YUV | COM7_QVGA},  // QVGA YUV
    {0x32, 0x80},  // HREF
    {0x17, 0x16},  // HSTART
    {0x18, 0x04},  // HSTOP
    {0x19, 0x02},  // VSTART
    {0x1A, 0x7A},  // VSTOP
    {0x03, 0x0A},  // VREF
    {0x70, 0x3A},  // X scaling
    {0x71, 0x35},  // Y scaling
    {0x72, 0x11},  // Downsample by 2
    {0x73, 0xF1},  // Clock divide
    {0x15, 0x00},  // COM10
    {0x3A, 0x04},  // TSLB
    {0x12, 0x00},  // COM7
    {0x8C, 0x00},  // RGB444
    {0x04, 0x00},  // COM1
    {0x40, 0xC0},  // COM15
    {0x14, 0x48},  // COM9
    {0x4F, 0x80},  // MTX1
    {0x50, 0x80},  // MTX2
    {0x51, 0x00},  // MTX3
    {0x52, 0x22},  // MTX4
    {0x53, 0x5E},  // MTX5
    {0x54, 0x80},  // MTX6
    {0x58, 0x9E},  // MTXS
    {0xFF, 0xFF}   // End marker
};

// I2C Initialization
void I2C1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Enable clocks
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    
    // Configure I2C pins: PB6 (SCL), PB7 (SDA)
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // Configure I2C (100kHz)
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x10909CEC;  // 100kHz @ 80MHz
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}

// UART Initialization for printf
void UART2_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    // PA2: TX, PA15: RX
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

// Redirect printf to UART
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, 1000);
    return len;
}

// Read register from OV7670
HAL_StatusTypeDef OV7670_ReadReg(uint8_t reg, uint8_t *value) {
    HAL_StatusTypeDef status;
    
    status = HAL_I2C_Master_Transmit(&hi2c1, OV7670_WRITE_ADDR, &reg, 1, 1000);
    if (status != HAL_OK) return status;
    
    status = HAL_I2C_Master_Receive(&hi2c1, OV7670_READ_ADDR, value, 1, 1000);
    return status;
}

// Write register to OV7670
HAL_StatusTypeDef OV7670_WriteReg(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    HAL_StatusTypeDef status;
    
    status = HAL_I2C_Master_Transmit(&hi2c1, OV7670_WRITE_ADDR, data, 2, 1000);
    HAL_Delay(1);
    
    return status;
}

// Minimal camera communication test
void OV7670_MinimalTest(void) {
    uint8_t pid, ver, com7;
    HAL_StatusTypeDef status;
    
    printf("\n=== OV7670 Camera Test ===\n");
    printf("Testing I2C communication...\n\n");
    
    // Test 1: Read Product ID
    status = OV7670_ReadReg(REG_PID, &pid);
    if (status == HAL_OK) {
        printf("Product ID MSB: 0x%02X ", pid);
        if (pid == 0x76) {
            printf("[OK]\n");
        } else {
            printf("[FAIL - Expected 0x76]\n");
        }
    } else {
        printf("FAILED to read Product ID!\n");
        printf("Check connections:\n");
        printf("  - PB6 -> SIOC (SCL)\n");
        printf("  - PB7 -> SIOD (SDA)\n");
        printf("  - 4.7k pull-ups on SDA/SCL\n");
        printf("  - Camera powered (3.3V)\n");
        return;
    }
    
    // Test 2: Read Version
    status = OV7670_ReadReg(REG_VER, &ver);
    if (status == HAL_OK) {
        printf("Product ID LSB: 0x%02X ", ver);
        if (ver == 0x73) {
            printf("[OK]\n\n");
        } else {
            printf("[FAIL - Expected 0x73]\n\n");
        }
    } else {
        printf("FAILED to read version!\n\n");
        return;
    }
    
    // Test 3: Read/Write test
    printf("Testing register write...\n");
    OV7670_ReadReg(REG_COM7, &com7);
    printf("  COM7 before: 0x%02X\n", com7);
    
    OV7670_WriteReg(REG_COM7, 0x80);  // Reset
    HAL_Delay(10);
    
    OV7670_ReadReg(REG_COM7, &com7);
    printf("  COM7 after reset: 0x%02X\n\n", com7);
    
    printf("=== Test Complete ===\n");
    if (pid == 0x76 && ver == 0x73) {
        printf("SUCCESS! Camera communication working!\n");
        printf("Ready for full configuration.\n\n");
    }
}

// Full camera initialization
int OV7670_Init(void) {
    int i = 0;
    
    printf("\n=== Initializing OV7670 ===\n");
    
    // Write configuration registers
    while (ov7670_config[i].reg != 0xFF || ov7670_config[i].value != 0xFF) {
        if (ov7670_config[i].reg == 0xFF) {
            // Delay marker
            printf("Delay %d ms...\n", ov7670_config[i].value);
            HAL_Delay(ov7670_config[i].value);
        } else {
            if (OV7670_WriteReg(ov7670_config[i].reg, 
                               ov7670_config[i].value) != HAL_OK) {
                printf("Failed to write reg 0x%02X\n", ov7670_config[i].reg);
                return -1;
            }
        }
        i++;
    }
    
    printf("Camera configured! (%d registers written)\n", i);
    
    // Verify configuration
    uint8_t com7;
    OV7670_ReadReg(REG_COM7, &com7);
    printf("COM7 = 0x%02X (QVGA YUV mode)\n\n", com7);
    
    return 0;
}

// System Clock Configuration to 80MHz
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        Error_Handler();
    }

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = 0;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;  // 4 MHz
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 40;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {
        // Error: stay here
    }
}

void XCLK_Init(void) {
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_11;                // PA11 -> XCLK
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF1_TIM1;        // TIM1 alternate function
    HAL_GPIO_Init(GPIOA, &gpio);

    TIM_HandleTypeDef htim1 = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 0;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 7;                 // 80MHz / (7+1) = 10MHz
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&htim1);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 4;                   // 50% duty
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4);

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
}

// Main function
int main(void) {
    // Initialize HAL
    HAL_Init();
    
    // Configure system clock to 80MHz
    SystemClock_Config();
    XCLK_Init(); 
    // Initialize peripherals
    UART2_Init();
    I2C1_Init();
    
    // Configure LED for status (PB3)
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef led;
    led.Pin = GPIO_PIN_3;
    led.Mode = GPIO_MODE_OUTPUT_PP;
    led.Pull = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &led);
    
    printf("\n\n");
    printf("========================================\n");
    printf("  OV7670 Camera Test for Line Follower\n");
    printf("  STM32L432KC\n");
    printf("========================================\n");
    
    // Wait for camera to power up
    printf("Waiting for camera...\n");
    HAL_Delay(100);
    
    // Test communication
    OV7670_MinimalTest();
    
    // If test passed, do full configuration
    HAL_Delay(500);
    
    printf("Press 'c' to configure camera or any key to repeat test...\n");
    
    while (1) {
        // Blink LED to show we're alive
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
        HAL_Delay(500);
        
        // Check for user input (optional - for interactive testing)
        // You can comment this out and just call OV7670_Init() directly
        
        // For now, just repeat test every 5 seconds
        static uint32_t last_test = 0;
        if (HAL_GetTick() - last_test > 5000) {
            last_test = HAL_GetTick();
            OV7670_MinimalTest();
        }
    }
}