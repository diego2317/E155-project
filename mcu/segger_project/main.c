// main.c
#include "main.h"
#include "uart.h"
#include "i2c.h"
#include "ov7670.h"
#include "xclk.h"
#include "spi.h"
#include "spi_control_handshake.h"

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

    XCLK_Init();
    UART2_Init();
    I2C1_Init();
    LED_Init();
    SPI1_Init();
    HAL_Delay(100);
    printf("Initializing camera...\n");
    int i = OV7670_QVGA_YUV_Init();
    while (i != 0) {
        i = OV7670_QVGA_YUV_Init(); // keep trying lmao
    }

    

    printf("Waiting for camera...\n");
    HAL_Delay(100);

    SpiControlHandshake_Init(&spi_handshake, frame_buffer, BUFFER_SIZE);

    if (SpiControlHandshake_BeginCapture(&spi_handshake) != HAL_OK) {
        Error_Handler();
    }
    //OV7670_Init();

    HAL_Delay(500);
    printf("Press 'c' to configure camera or any key to repeat test...\n");

    uint32_t last_test = 0;
    uint32_t last_led_toggle = 0;

    while (1) {
        if (HAL_GetTick() - last_led_toggle > 250) {
            last_led_toggle = HAL_GetTick();
            HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
        }

        if (HAL_GetTick() - last_test > 5000) {
            last_test = HAL_GetTick();
            printf("Frames captured: %lu\n", (unsigned long)completed_frames);
            //OV7670_MinimalTest();
        }

        SpiControlHandshake_Service(&spi_handshake);

        uint8_t latest_midpoint = 0U;
        if (SpiControlHandshake_TakeMidpoint(&spi_handshake, &latest_midpoint)) {
            midpoint_pixel = latest_midpoint;
        }

        if (SpiControlHandshake_FrameReady(&spi_handshake)) {
            ProcessFrame(frame_buffer, BUFFER_SIZE);
            SpiControlHandshake_ReleaseFrame(&spi_handshake);

            if (SpiControlHandshake_BeginCapture(&spi_handshake) != HAL_OK) {
                Error_Handler();
            }
        }

        if (SpiControlHandshake_InError(&spi_handshake)) {
            Error_Handler();
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
