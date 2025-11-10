#ifndef SCCB_H
#define SCCB_H

#include <stdint.h>

/* Initialize I2C peripheral for SCCB timing (typically 100–400 kHz). */
void sccb_init(void);

/* Write one 8-bit value to an SCCB device register. 
 * dev_addr: 7-bit address + write bit handled by low-level I2C driver
 */
int sccb_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);

/* Read one 8-bit value from an SCCB device register. */
int sccb_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data);

/* Optional bulk write for register tables. */
int sccb_write_multi(uint8_t dev_addr, const uint8_t *data, uint16_t len);

#endif /* SCCB_H */
