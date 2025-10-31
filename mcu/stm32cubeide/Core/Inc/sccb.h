#ifndef SCCB_H
#define SCCB_H

#include <stdint.h>
/* Write one 8-bit value to an SCCB device register.
 * dev_addr: 7-bit address + write bit handled by low-level I2C driver
 */
int sccb_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t data);

/* Read one 8-bit value from an SCCB device register. */
int sccb_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data);

/* Uses HAl_I2C_IsDeviceReady */
int sccb_probe(void);

#endif /* SCCB_H */
