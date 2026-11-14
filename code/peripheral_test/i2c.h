/*
 * I2C1 master driver (blocking, 7-bit addressing, write only).
 * PB3 = SCL and PB4 = SDA
 */
#ifndef I2C_H
#define I2C_H

#include <stdint.h>

void i2c_init(void);

/* Streaming write: start_write, any number of put, then stop. */
int  i2c_start_write(uint8_t addr7);
int  i2c_put(uint8_t byte);
int  i2c_stop(void);

/* One-shot write of a buffer. */
int  i2c_write(uint8_t addr7, const uint8_t *data, unsigned len);

#endif
