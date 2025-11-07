#include "sccb.h"

#include "stm32l4xx_hal.h"

extern I2C_HandleTypeDef hi2c1;

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

/* Check if we configured the registers properly */
int sccb_probe(void) {
	// TODO: IMplement
	return 0;
}
