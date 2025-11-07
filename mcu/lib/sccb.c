/* sccb.c */
#include "sccb.h"

HAL_StatusTypeDef SCCB_Init_I2C1(sccb_t *bus, uint32_t timing) {
  __HAL_RCC_GPIOB_CLK_ENABLE(); __HAL_RCC_I2C1_CLK_ENABLE();
  GPIO_InitTypeDef g = {0};
  g.Pin = GPIO_PIN_6 | GPIO_PIN_7; g.Mode = GPIO_MODE_AF_OD; g.Pull = GPIO_PULLUP;
  g.Speed = GPIO_SPEED_FREQ_HIGH; g.Alternate = GPIO_AF4_I2C1; HAL_GPIO_Init(GPIOB, &g);
  bus->hi2c.Instance = I2C1;
  bus->hi2c.Init.Timing = timing;
  bus->hi2c.Init.OwnAddress1 = 0;
  bus->hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  bus->hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  bus->hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  bus->hi2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  return HAL_I2C_Init(&bus->hi2c);
}

HAL_StatusTypeDef SCCB_Read8(sccb_t *bus, uint8_t reg, uint8_t *val) {
  HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(&bus->hi2c, (bus->dev7<<1), &reg, 1, 1000);
  if (st != HAL_OK) return st;
  return HAL_I2C_Master_Receive(&bus->hi2c, (bus->dev7<<1) | 1U, val, 1, 1000);
}

HAL_StatusTypeDef SCCB_Write8(sccb_t *bus, uint8_t reg, uint8_t val) {
  uint8_t b[2] = {reg, val};
  HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(&bus->hi2c, (bus->dev7<<1), b, 2, 1000);
  HAL_Delay(1);
  return st;
}
