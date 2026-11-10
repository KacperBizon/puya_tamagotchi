#include <stdint.h>

#define REG32(a)      (*(volatile uint32_t *)(a))
#define RCC_IOPENR    REG32(0x40021034UL)
#define GPIOA_MODER   REG32(0x50000000UL)
#define GPIOA_BSRR    REG32(0x50000018UL)

#define RED           0u
#define WHITE         1u

static void led(unsigned pin, int on)
{
    GPIOA_BSRR = on ? (1u << pin) : (1u << (pin + 16));
}

static void delay(volatile uint32_t n) { while (n--) __asm volatile ("nop"); }

int main(void)
{
    RCC_IOPENR |= 1u;
    led(RED, 0); led(WHITE, 0);
    GPIOA_MODER = (GPIOA_MODER & ~((3u << 0) | (3u << 2))) | (1u << 0) | (1u << 2);

    for (;;) {
        led(RED, 1);   delay(1000000);
        led(RED, 0);   delay(1000000);
        led(WHITE, 1); delay(1000000);
        led(WHITE, 0); delay(1000000);
    }
}
