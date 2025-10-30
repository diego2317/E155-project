#include "sccb.h"
#include "stm32l432.h"   /* replace with your MCU’s HAL, e.g. stm32l4xx_hal_i2c.h */

/* Example: assumes HAL_I2C_HandleTypeDef hi2c1 exists */

void sccb_init(void)
{
    /* Configure and enable I2C peripheral.
       For STM32 HAL:
       hi2c1.Instance = I2C1;
       hi2c1.Init.ClockSpeed = 100000;
       hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
       hi2c1.Init.OwnAddress1 = 0;
       hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
       HAL_I2C_Init(&hi2c1);
    */
}

/* Single register write */
int sccb_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t data)
{
    uint8_t buf[2] = { reg_addr, data };
    /* Replace with your platform's I2C write routine */
    if (HAL_I2C_Master_Transmit(&hi2c1, dev_addr, buf, 2, 100) != HAL_OK)
        return -1;
    return 0;
}

/* Single register read */
int sccb_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data)
{
    if (HAL_I2C_Master_Transmit(&hi2c1, dev_addr, &reg_addr, 1, 100) != HAL_OK)
        return -1;
    if (HAL_I2C_Master_Receive(&hi2c1, dev_addr, data, 1, 100) != HAL_OK)
        return -1;
    return 0;
}

/* Optional: burst write helper */
int sccb_write_multi(uint8_t dev_addr, const uint8_t *data, uint16_t len)
{
    if (HAL_I2C_Master_Transmit(&hi2c1, dev_addr, (uint8_t *)data, len, 100) != HAL_OK)
        return -1;
    return 0;
}
