/* xclk.c */
#include "xclk.h"
static TIM_HandleTypeDef htim1;

HAL_StatusTypeDef XCLK_Start(uint32_t freq_hz, float duty) {
  __HAL_RCC_TIM1_CLK_ENABLE(); __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef gpio = { .Pin=GPIO_PIN_11, .Mode=GPIO_MODE_AF_PP, .Pull=GPIO_NOPULL,
                            .Speed=GPIO_SPEED_FREQ_VERY_HIGH, .Alternate=GPIO_AF1_TIM1 };
  HAL_GPIO_Init(GPIOA, &gpio);

  uint32_t timclk = HAL_RCC_GetPCLK2Freq(); // TIM1 on APB2
  // prescaler=0, compute ARR for freq
  uint32_t arr = (timclk / freq_hz) - 1;
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0; htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = arr; htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  HAL_TIM_PWM_Init(&htim1);
  TIM_OC_InitTypeDef oc = {0};
  oc.OCMode = TIM_OCMODE_PWM1;
  oc.Pulse = (uint32_t)((arr + 1) * duty);
  oc.OCPolarity = TIM_OCPOLARITY_HIGH;
  HAL_TIM_PWM_ConfigChannel(&htim1, &oc, TIM_CHANNEL_4);
  return HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
}
