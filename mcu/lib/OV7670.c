#include "ov7670.h"
#include "sccb.h"   /* low-level I2C driver */

#define OV7670_DELAY_MS(ms)  /* platform delay macro */

/* Write one register over SCCB */
int ov7670_write_reg(uint8_t reg, uint8_t value)
{
    return sccb_write(OV7670_ADDR_WRITE, reg, value);
}

/* Read one register over SCCB */
int ov7670_read_reg(uint8_t reg, uint8_t *value)
{
    return sccb_read(OV7670_ADDR_READ, reg, value);
}

/* Apply a list of reg/value pairs */
int ov7670_write_table(const ov7670_regval_t *table)
{
    int status;
    while (!(table->reg == 0xFF && table->val == 0xFF)) {
        status = ov7670_write_reg(table->reg, table->val);
        if (status != 0) return status;
        table++;
    }
    return 0;
}

/* Initialize camera with given format */
int ov7670_init(ov7670_t *cam)
{
    const ov7670_regval_t *cfg;

    switch (cam->format) {
    case OV7670_FMT_YUV422_QVGA:
        cfg = OV7670_INIT_YUV422_QVGA;
        break;
    case OV7670_FMT_RGB565_QVGA:
        cfg = OV7670_INIT_RGB565_QVGA;
        break;
    default:
        return -1;
    }

    /* reset via COM7 and wait */
    ov7670_write_reg(OV7670_REG_COM7, OV7670_COM7_RESET);
    OV7670_DELAY_MS(10);

    /* write config table */
    return ov7670_write_table(cfg);
}

/* Example control helpers */
int ov7670_set_brightness(uint8_t val)
{
    return ov7670_write_reg(OV7670_REG_BRIGHT, val);
}

int ov7670_set_contrast(uint8_t val)
{
    return ov7670_write_reg(OV7670_REG_CONTR, val);
}
