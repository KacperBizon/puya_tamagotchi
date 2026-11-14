#include "i2c.h"
#include "py32.h"
#include "timebase.h"

#ifndef I2C_FAST
#define I2C_FAST 1                 /* 1: 400 kHz, 0: 100 kHz */
#endif

#define I2C_BASE   0x40005400UL
#define I2C_CR1    REG32(I2C_BASE + 0x00)
#define I2C_CR2    REG32(I2C_BASE + 0x04)
#define I2C_DR     REG32(I2C_BASE + 0x10)
#define I2C_SR1    REG32(I2C_BASE + 0x14)
#define I2C_SR2    REG32(I2C_BASE + 0x18)
#define I2C_CCR    REG32(I2C_BASE + 0x1C)
#define I2C_TRISE  REG32(I2C_BASE + 0x20)

#define CR1_PE     (1u << 0)
#define CR1_START  (1u << 8)
#define CR1_STOP   (1u << 9)
#define CR1_SWRST  (1u << 15)
#define SR1_SB     (1u << 0)
#define SR1_ADDR   (1u << 1)
#define SR1_BTF    (1u << 2)
#define SR1_TXE    (1u << 7)
#define SR1_AF     (1u << 10)
#define SR2_BUSY   (1u << 1)

#define TIMEOUT_MS 5u

void i2c_init(void)
{
    RCC_APBENR1 |= (1u << 21);              /* I2C1EN */
    I2C_CR1 = CR1_SWRST;
    I2C_CR1 = 0;
    I2C_CR2 = 24;                           /* FREQ = PCLK in MHz */
#if I2C_FAST
    I2C_CCR   = (1u << 15) | 20u;           /* fast mode, T_low = 2*T_high: 24 MHz/(3*20) = 400 kHz */
    I2C_TRISE = 8;                          /* 300 ns / 41.7 ns + 1 */
#else
    I2C_CCR   = 120u;                       /* standard mode: 24 MHz/(2*120) = 100 kHz */
    I2C_TRISE = 25;                         /* 1000 ns / 41.7 ns + 1 */
#endif
    I2C_CR1 = CR1_PE;
}

/* Release the bus after an error. */
static void i2c_abort(void)
{
    uint32_t t;
    I2C_SR1 &= ~SR1_AF;
    I2C_CR1 |= CR1_STOP;
    t = millis();
    while ((I2C_CR1 & CR1_STOP) && ((millis() - t) <= TIMEOUT_MS)) { }
    if (I2C_CR1 & CR1_STOP) i2c_init();     /* bus stuck: reset the peripheral */
}

static int wait_sr1(uint32_t mask)
{
    uint32_t t = millis();
    for (;;) {
        uint32_t s = I2C_SR1;
        if (s & mask) return 1;
        if (s & SR1_AF) return 0;                    /* NACK */
        if ((millis() - t) > TIMEOUT_MS) return 0;   /* timeout */
    }
}

int i2c_start_write(uint8_t addr7)
{
    uint32_t t = millis();
    while (I2C_SR2 & SR2_BUSY) {
        if ((millis() - t) > TIMEOUT_MS) { i2c_init(); return 0; }
    }
    I2C_CR1 |= CR1_START;
    if (!wait_sr1(SR1_SB)) goto fail;
    I2C_DR = (uint8_t)(addr7 << 1);
    if (!wait_sr1(SR1_ADDR)) goto fail;
    (void)I2C_SR1;
    (void)I2C_SR2;                          /* read SR1 then SR2 clears ADDR */
    return 1;
fail:
    i2c_abort();
    return 0;
}

int i2c_put(uint8_t byte)
{
    if (!wait_sr1(SR1_TXE)) { i2c_abort(); return 0; }
    I2C_DR = byte;
    return 1;
}

int i2c_stop(void)
{
    uint32_t t;
    if (!wait_sr1(SR1_BTF)) { i2c_abort(); return 0; }
    I2C_CR1 |= CR1_STOP;
    t = millis();
    while ((I2C_CR1 & CR1_STOP) && ((millis() - t) <= TIMEOUT_MS)) { }
    return 1;
}

int i2c_write(uint8_t addr7, const uint8_t *data, unsigned len)
{
    if (!i2c_start_write(addr7)) return 0;
    for (unsigned i = 0; i < len; i++) {
        if (!i2c_put(data[i])) return 0;
    }
    return i2c_stop();
}
