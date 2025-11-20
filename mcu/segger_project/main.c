// main.c
#include "main.h"
#include "uart.h"
#include "i2c.h"
#include "ov7670.h"
#include "xclk.h"
#include "spi.h"
#include "spi_control_handshake.h"
#include "gpio.h"

static void LED_Init(void);
static void ProcessFrame(uint8_t *buffer, uint16_t length);

#define BUFFER_SIZE SPI_RX_BUFFER_BYTES
ALIGN_32BYTES(uint8_t frame_buffer[BUFFER_SIZE]);

static volatile uint32_t completed_frames = 0;
static uint8_t midpoint_pixel = 0;
static SpiControlHandshake spi_handshake;

int main(void) {
    HAL_Init();
    SystemClock_Config();

    UART2_Init();
    I2C1_Init();
    GPIO_Capture_Init();
    HAL_Delay(100);
    // Start XCLK FIRST
    printf("Starting XCLK (10MHz on PA11)...\n");
    XCLK_Init();
    HAL_Delay(300);
    printf("XCLK running\n\n");

    

    
    // Check camera
    printf("Checking camera...\n");
    uint8_t pid, ver;
    int retry = 0;
    while (retry < 3) {
        if (OV7670_ReadReg(0x0A, &pid) == HAL_OK && 
            OV7670_ReadReg(0x0B, &ver) == HAL_OK &&
            pid == 0x76 && ver == 0x73) {
            printf("!! Camera detected (PID=0x%02X, VER=0x%02X)\n\n", pid, ver);
            break;
        }
        retry++;
        printf("  Retry %d...\n", retry);
        HAL_Delay(200);
    }
    
    if (retry >= 3) {
        printf("X Camera not responding!\n");
        while(1) { 
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3); 
            HAL_Delay(100); 
        }
    }
    
    // Configure camera
    printf("Configuring camera...\n");
    HAL_Delay(500);
    
    if (OV7670_Init_QVGA() == 0) {
        printf("Waiting for video output (1 second)...\n");
        HAL_Delay(1000);
        
        Camera_ShowCurrentConfig();
        Camera_SignalTest();
        Camera_MeasureFrameRate();
        Camera_VerifyFormat();
        Camera_VerifyFormat();
    } else {
        printf("Configuration failed!\n\n");
    }
    
    printf("------------------------------------------\n");
    printf("|  Monitoring Loop Active                |\n");
    printf("|  LED blinks = MCU alive                |\n");
    printf("|  Tests every 20 seconds                |\n");
    printf("------------------------------------------\n\n");
    
    while (1) {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
        HAL_Delay(500);
        
        static uint32_t last = 0;
        if (HAL_GetTick() - last > 20000) {
            last = HAL_GetTick();
            printf("\n Periodic Check \n\n");
            Camera_ShowCurrentConfig();
            //Camera_SignalTest();
            Camera_MeasureFrameRate();
        }
    }
}

// System clock + error handler + LED init

void SystemClock_Config(void) {
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

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        Error_Handler();
    }
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {
        // stay here
    }
}

static void LED_Init(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef led = {0};
    led.Pin = GPIO_PIN_3;
    led.Mode = GPIO_MODE_OUTPUT_PP;
    led.Pull = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &led);
}

static void ProcessFrame(uint8_t *buffer, uint16_t length)
{
    (void)buffer;
    (void)length;

    completed_frames++;

    /* Example hook: make LED reflect the mid-pixel brightness */
    if (midpoint_pixel > 0x80U) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
    }
}
