#include <stdint.h>

extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
extern int main(void);

void Reset_Handler(void);
void Default_Handler(void);

void NMI_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)    __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)   __attribute__((weak, alias("Default_Handler")));

__attribute__((section(".isr_vector"), used))
void (* const vector_table[])(void) = {
    (void (*)(void))&_estack,   /* 0x00 initial SP */
    Reset_Handler,              /* 0x04 */
    NMI_Handler,                /* 0x08 */
    HardFault_Handler,          /* 0x0C */
    0, 0, 0, 0, 0, 0, 0,        /* 0x10-0x28 reserved */
    SVC_Handler,                /* 0x2C */
    0, 0,                       /* 0x30-0x34 reserved */
    PendSV_Handler,             /* 0x38 */
    SysTick_Handler,            /* 0x3C */
};

void Reset_Handler(void)
{
    uint32_t *src = &_sidata, *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;
    for (dst = &_sbss; dst < &_ebss; ) *dst++ = 0;
    main();
    for (;;) {}
}

void Default_Handler(void) { for (;;) {} }
