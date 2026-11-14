/* 1 ms time base on SysTick. */
#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <stdint.h>

void     timebase_init(void);          /* start SysTick, 1 ms tick */
uint32_t millis(void);                 /* ms since timebase_init(), wraps after ~49 days */
void     delay_ms(uint32_t ms);        /* busy wait */

#endif
