/* Shared chip definitions for the PY32F002B (see PY32F002B Reference Manual V1.0). */
#ifndef PY32_H
#define PY32_H

#include <stdint.h>

#define SYSCLK_HZ     24000000UL          /* HSI, no divider, no PLL */

#define REG32(a)      (*(volatile uint32_t *)(a))

#define RCC_BASE      0x40021000UL
#define RCC_ICSCR     REG32(RCC_BASE + 0x04)
#define RCC_IOPENR    REG32(RCC_BASE + 0x34)
#define RCC_APBENR1   REG32(RCC_BASE + 0x3C)
#define RCC_APBENR2   REG32(RCC_BASE + 0x40)

#endif
