#include "timebase.h"
#include "py32.h"

#define SYST_CSR  REG32(0xE000E010UL)
#define SYST_RVR  REG32(0xE000E014UL)
#define SYST_CVR  REG32(0xE000E018UL)

static volatile uint32_t g_ms;
static void (* volatile g_hook)(void);

/* Overrides the weak handler in startup.c */
void SysTick_Handler(void)
{
    void (*hook)(void) = g_hook;
    g_ms++;
    if (hook) hook();
}

void timebase_set_tick_hook(void (*hook)(void)) { g_hook = hook; }

void timebase_init(void)
{
    SYST_RVR = (SYSCLK_HZ / 1000u) - 1u;   /* 1 ms */
    SYST_CVR = 0;
    SYST_CSR = 7u;                         /* enable | interrupt | core clock */
}

uint32_t millis(void) { return g_ms; }

void delay_ms(uint32_t ms)
{
    uint32_t t = g_ms;
    while ((g_ms - t) < ms) { }
}
