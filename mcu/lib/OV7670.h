#ifndef OV7670_H
#define OV7670_H

#include <stdint.h>
#include "ov7670_regs.h"
#include "ov7670_init.h"

/* camera format selector */
typedef enum {
    OV7670_FMT_YUV422_QVGA,
    OV7670_FMT_RGB565_QVGA
} ov7670_format_t;

/* camera configuration structure */
typedef struct {
    ov7670_format_t format;
    uint8_t i2c_addr;   /* usually 0x42 */
} ov7670_t;

/* initialize camera over SCCB/I2C */
int ov7670_init(ov7670_t *cam);

/* write a single register */
int ov7670_write_reg(uint8_t reg, uint8_t value);

/* read a single register */
int ov7670_read_reg(uint8_t reg, uint8_t *value);

/* apply a register table (terminated by 0xFF,0xFF) */
int ov7670_write_table(const ov7670_regval_t *table);

/* control helpers, idk if I'll implement */
int ov7670_set_brightness(uint8_t val);
int ov7670_set_contrast(uint8_t val);

#endif /* OV7670_H */
