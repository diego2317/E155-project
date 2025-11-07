/* board_init.c */
#include "board_init.h"

void Board_SystemClock_Config(void) {
  RCC_OscInitTypeDef osc = {0};
  RCC_ClkInitTypeDef clk = {0};
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
  osc.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  osc.MSIState = RCC_MSI_ON;
  osc.MSIClockRange = RCC_MSIRANGE_6;      // 4 MHz
  osc.PLL.PLLState = RCC_PLL_ON;
  osc.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  osc.PLL.PLLM = 1; osc.PLL.PLLN = 40;
  osc.PLL.PLLP = RCC_PLLP_DIV7; osc.PLL.PLLQ = RCC_PLLQ_DIV2; osc.PLL.PLLR = RCC_PLLR_DIV2;
  HAL_RCC_OscConfig(&osc);
  clk.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
  clk.APB1CLKDivider = RCC_HCLK_DIV1;
  clk.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_4);
}

void Board_InitLED(void) {
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef io = { .Pin = GPIO_PIN_3, .Mode = GPIO_MODE_OUTPUT_PP };
  HAL_GPIO_Init(GPIOB, &io);
}

void Board_LED_Toggle(void) { HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3); }
