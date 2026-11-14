/* Shared chip definitions for the PY32F002B (see PY32F002B Reference Manual V1.0). */
#ifndef PY32_H
#define PY32_H

#include <stdint.h>

#define SYSCLK_HZ     24000000UL          /* HSI, no divider, no PLL */

#ifndef REG32
#define REG32(a)      (*(volatile uint32_t *)(a))
#endif

#define RCC_BASE      0x40021000UL
#define RCC_ICSCR     REG32(RCC_BASE + 0x04)
#define RCC_IOPENR    REG32(RCC_BASE + 0x34)
#define RCC_APBENR1   REG32(RCC_BASE + 0x3C)
#define RCC_APBENR2   REG32(RCC_BASE + 0x40)

/* Short critical sections: save PRIMASK and disable interrupts / restore it. */
#if defined(__arm__)
static inline uint32_t irq_save(void)
{
    uint32_t primask;
    __asm volatile ("mrs %0, primask\n\tcpsid i" : "=r" (primask) :: "memory");
    return primask;
}
static inline void irq_restore(uint32_t primask)
{
    __asm volatile ("msr primask, %0" :: "r" (primask) : "memory");
}
#else   /* host builds (unit tests) */
static inline uint32_t irq_save(void) { return 0; }
static inline void irq_restore(uint32_t primask) { (void)primask; }
#endif

#endif
