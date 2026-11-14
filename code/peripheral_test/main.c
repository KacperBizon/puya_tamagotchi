/*
 * PY32F002BF15U6 - bare-metal peripheral test (no HAL, no libc)
 *
 *   PA0 RED LED (high = on)      PA1 WHITE LED (high = on)
 *   PA3 LDR_DRIVE (high = LDR powered)
 *   PA4..PA7 BUTTON_1..4
 *   PB0 TIM1_CH2 (AF2) -> passive buzzer, PWM
 *   PB1 ADC_IN0        -> LDR output
 *   PB3 I2C1 SCL (AF6), PB4 I2C1 SDA (AF6) -> SH1107 128x128 OLED
 *   PA2 (SWCLK) and PB6 (SWDIO) are never touched.
 *
 * What it does:
 *   BUTTON_1 held     -> red LED on
 *   BUTTON_2 held     -> white LED on
 *   BUTTON_3 held     -> buzzer tone; frequency steps 1k/2k/3k/4k Hz on each release
 *   BUTTON_4 pressed  -> toggles LDR power (PA3)
 *   OLED shows button states, raw pin levels, LED states, LDR power,
 *   ADC value / percent / bar, buzzer state and frequency.
 *
 */
#include <stdint.h>
#include "py32.h"
#include "timebase.h"
#include "i2c.h"
#include "oled.h"

/* ------------------------- board configuration -------------------------- */
#define DEBOUNCE_MS      20u

/* ------------------------------ registers ------------------------------- */
#define GPIOA_BASE       0x50000000UL
#define GPIOB_BASE       0x50000400UL
#define GPIO_MODER(b)    REG32((b) + 0x00)
#define GPIO_OTYPER(b)   REG32((b) + 0x04)
#define GPIO_PUPDR(b)    REG32((b) + 0x0C)
#define GPIO_IDR(b)      REG32((b) + 0x10)
#define GPIO_BSRR(b)     REG32((b) + 0x18)
#define GPIO_AFRL(b)     REG32((b) + 0x20)

#define TIM1_BASE        0x40012C00UL
#define TIM1_CR1         REG32(TIM1_BASE + 0x00)
#define TIM1_EGR         REG32(TIM1_BASE + 0x14)
#define TIM1_CCMR1       REG32(TIM1_BASE + 0x18)
#define TIM1_CCER        REG32(TIM1_BASE + 0x20)
#define TIM1_PSC         REG32(TIM1_BASE + 0x28)
#define TIM1_ARR         REG32(TIM1_BASE + 0x2C)
#define TIM1_CCR2        REG32(TIM1_BASE + 0x38)
#define TIM1_BDTR        REG32(TIM1_BASE + 0x44)

#define ADC_BASE         0x40012400UL
#define ADC_ISR          REG32(ADC_BASE + 0x00)
#define ADC_CR           REG32(ADC_BASE + 0x08)
#define ADC_CFGR2        REG32(ADC_BASE + 0x10)
#define ADC_SMPR         REG32(ADC_BASE + 0x14)
#define ADC_CHSELR       REG32(ADC_BASE + 0x28)
#define ADC_DR           REG32(ADC_BASE + 0x40)

#define HSI_TRIM_WORD    (*(volatile uint32_t *)0x1FFF0100UL)  /* factory HSI 24 MHz trim */

static void clock_init(void)
{
    /* Load the factory HSI trim exactly like Puya's SystemInit() does. */
    RCC_ICSCR = (RCC_ICSCR & 0xFFFF0000u) | HSI_TRIM_WORD;
}

/* ------------------------------- GPIO ----------------------------------- */
#define MODE_IN 0u
#define MODE_OUT 1u
#define MODE_AF 2u
#define MODE_ANALOG 3u
#define PULL_NONE 0u
#define PULL_UP 1u
#define PULL_DOWN 2u

static void gpio_mode(uintptr_t b, unsigned pin, uint32_t mode)
{
    uint32_t v = GPIO_MODER(b);
    v &= ~(3u << (pin * 2u));
    v |= (mode << (pin * 2u));
    GPIO_MODER(b) = v;
}

static void gpio_pull(uintptr_t b, unsigned pin, uint32_t pull)
{
    uint32_t v = GPIO_PUPDR(b);
    v &= ~(3u << (pin * 2u));
    v |= (pull << (pin * 2u));
    GPIO_PUPDR(b) = v;
}

static void gpio_af(uintptr_t b, unsigned pin, uint32_t af)   /* pin 0..7 */
{
    uint32_t v = GPIO_AFRL(b);
    v &= ~(0xFu << (pin * 4u));
    v |= (af << (pin * 4u));
    GPIO_AFRL(b) = v;
}

static void gpio_opendrain(uintptr_t b, unsigned pin) { GPIO_OTYPER(b) |= (1u << pin); }

static void pin_write(uintptr_t b, unsigned pin, int high)
{
    GPIO_BSRR(b) = high ? (1u << pin) : (1u << (pin + 16u));
}

#define PIN_RED     0u
#define PIN_WHITE   1u
#define PIN_LDR_DRV 3u

static void led_red(int on)   { pin_write(GPIOA_BASE, PIN_RED, on); }     /* high = on */
static void led_white(int on) { pin_write(GPIOA_BASE, PIN_WHITE, on); }
static void ldr_power(int on) { pin_write(GPIOA_BASE, PIN_LDR_DRV, on); }

static void gpio_init(void)
{
    RCC_IOPENR |= (1u << 0) | (1u << 1);          /* GPIOA, GPIOB clocks */
    (void)RCC_IOPENR;

    /* Preload output levels before the pins become outputs: LEDs off, LDR powered. */
    led_red(0);
    led_white(0);
    ldr_power(1);
    gpio_mode(GPIOA_BASE, PIN_RED, MODE_OUT);
    gpio_mode(GPIOA_BASE, PIN_WHITE, MODE_OUT);
    gpio_mode(GPIOA_BASE, PIN_LDR_DRV, MODE_OUT);

    /* Buttons PA4..PA7 */
    for (unsigned p = 4; p <= 7; p++) {
        gpio_pull(GPIOA_BASE, p, PULL_UP);
        gpio_mode(GPIOA_BASE, p, MODE_IN);
    }

    /* PB0 -> TIM1_CH2 (AF2) */
    gpio_af(GPIOB_BASE, 0, 2);
    gpio_mode(GPIOB_BASE, 0, MODE_AF);

    /* PB1 -> ADC_IN0, analog */
    gpio_mode(GPIOB_BASE, 1, MODE_ANALOG);

    /* PB3 SCL / PB4 SDA -> I2C1 (AF6), open drain + internal pull-up as a fallback */
    gpio_af(GPIOB_BASE, 3, 6);
    gpio_af(GPIOB_BASE, 4, 6);
    gpio_opendrain(GPIOB_BASE, 3);
    gpio_opendrain(GPIOB_BASE, 4);
    gpio_pull(GPIOB_BASE, 3, PULL_UP);
    gpio_pull(GPIOB_BASE, 4, PULL_UP);
    gpio_mode(GPIOB_BASE, 3, MODE_AF);
    gpio_mode(GPIOB_BASE, 4, MODE_AF);
}

/* ------------------------------- buzzer --------------------------------- */
/* TIM1 clock 24 MHz, PSC = 23 -> 1 MHz tick. ARR = 1e6/f - 1 (precomputed: no division). */
static const uint16_t tone_hz[4]  = { 1000, 2000, 3000, 4000 };
static const uint16_t tone_arr[4] = {  999,  499,  332,  249 };

static void buzzer_init(void)
{
    RCC_APBENR2 |= (1u << 11);              /* TIM1EN */
    TIM1_PSC   = 23;
    TIM1_ARR   = 999;
    TIM1_CCR2  = 0;                         /* duty 0 = silent (pin stays low) */
    TIM1_CCMR1 = (6u << 12) | (1u << 11);   /* OC2M = PWM mode 1, OC2PE preload */
    TIM1_CCER  = (1u << 4);                 /* CC2E: channel 2 output on */
    TIM1_BDTR  = (1u << 15);                /* MOE: main output enable (advanced timer) */
    TIM1_CR1   = (1u << 7);                 /* ARPE */
    TIM1_EGR   = 1u;                        /* UG: load PSC/ARR/CCR */
    TIM1_CR1  |= 1u;                        /* CEN */
}

static void buzzer_set(unsigned idx, int on)
{
    if (!on) { TIM1_CCR2 = 0; return; }
    TIM1_ARR  = tone_arr[idx];
    TIM1_CCR2 = (uint32_t)(tone_arr[idx] + 1u) >> 1;   /* 50 % duty */
}

/* --------------------------------- ADC ---------------------------------- */
static int adc_init(void)
{
    uint32_t t;

    RCC_APBENR2 |= (1u << 20);              /* ADCEN */
    ADC_CFGR2 = (2u << 28);                 /* CKMODE = PCLK/4 -> 6 MHz (max 8 MHz at 1.7 V) */

    ADC_CR = (1u << 31);                    /* ADCAL */
    t = millis();
    while (ADC_CR & (1u << 31)) {
        if ((millis() - t) > 20u) return 0;
    }
    delay_ms(1);

    ADC_CHSELR = 1u;                        /* channel 0 = PB1 */
    ADC_CR = 1u;                            /* ADEN */
    delay_ms(1);
    ADC_SMPR = 7u;                          /* 239.5 cycles: LDR divider is high impedance */
    return 1;
}

static int adc_read_once(uint16_t *out)
{
    uint32_t t;

    ADC_ISR = (1u << 1) | (1u << 2) | (1u << 3) | (1u << 4);   /* clear EOSMP/EOC/EOSEQ/OVR */
    ADC_CR = (1u << 2) | 1u;                                  /* ADSTART (ADEN stays set) */
    t = millis();
    while (!(ADC_ISR & (1u << 2))) {                          /* EOC */
        if ((millis() - t) > 5u) return 0;
    }
    *out = (uint16_t)(ADC_DR & 0xFFFu);
    return 1;
}

static int adc_read_avg(uint16_t *out)       /* average of 8 samples */
{
    uint32_t sum = 0;
    uint16_t v;
    for (int i = 0; i < 8; i++) {
        if (!adc_read_once(&v)) return 0;
        sum += v;
    }
    *out = (uint16_t)(sum >> 3);
    return 1;
}

/* ------------------------- tiny string formatting ----------------------- */
static char *put_str(char *p, const char *s)
{
    while (*s) *p++ = *s++;
    return p;
}

/* 4 characters wide, right aligned, 0..9999. No division (no libgcc needed). */
static char *put_dec4(char *p, uint32_t v)
{
    static const uint16_t pw[4] = { 1000, 100, 10, 1 };
    int started = 0;
    for (unsigned i = 0; i < 4; i++) {
        unsigned d = 0;
        while (v >= pw[i]) { v -= pw[i]; d++; }
        if (d || started || i == 3) { *p++ = (char)('0' + d); started = 1; }
        else                        { *p++ = ' '; }
    }
    return p;
}

/* -------------------------------- screen -------------------------------- */
static int ui_static(void)
{
    return oled_text(0,  "PY32F002B PERIPH TEST") &&
           oled_text(2,  "BUTTONS  # = PRESSED") &&
           oled_text(14, "B1 RED    B2 WHITE") &&
           oled_text(15, "B3 BUZZ   B4 LDR PWR");
}

/* -------------------------------- main ---------------------------------- */
int main(void)
{
    uint8_t  last_down = 0, stable = 0;
    uint32_t t_change = 0, t_adc = 0, t_retry = 0;
    uint16_t adc_val = 0;
    int      adc_ok = 0, adc_have = 0, oled_ok = 0;
    int      ldr_on = 1, red = 0, white = 0, buzz = 0;
    unsigned tone_idx = 0;
    char     line[OLED_COLS + 1];
    char    *p;

    clock_init();
    timebase_init();
    gpio_init();
    buzzer_init();
    adc_ok = adc_init();
    i2c_init();

    delay_ms(100);                              /* let the OLED power up */
    oled_ok = oled_init() && ui_static();
    t_retry = millis();

    for (;;) {
        uint32_t now = millis();

        /* ---- buttons: debounce + actions ---- */
        uint8_t raw  = (uint8_t)((GPIO_IDR(GPIOA_BASE) >> 4) & 0xFu);
        uint8_t down = (uint8_t)(~raw & 0xFu);
        if (down != last_down) { last_down = down; t_change = now; }
        if (down != stable && (now - t_change) >= DEBOUNCE_MS) {
            uint8_t prev = stable;
            uint8_t pressed  = (uint8_t)(down & ~prev);
            uint8_t released = (uint8_t)(~down & prev & 0xFu);
            stable = down;

            red   = (stable & 0x1) != 0;                 /* BUTTON_1 */
            white = (stable & 0x2) != 0;                 /* BUTTON_2 */
            led_red(red);
            led_white(white);

            if (released & 0x4) tone_idx = (tone_idx + 1u) & 3u;   /* BUTTON_3 release: next tone */
            buzz = (stable & 0x4) != 0;
            buzzer_set(tone_idx, buzz);

            if (pressed & 0x8) { ldr_on = !ldr_on; ldr_power(ldr_on); }   /* BUTTON_4 */
        }

        /* ---- LDR ADC, 10 Hz ---- */
        if ((now - t_adc) >= 100u) {
            t_adc = now;
            adc_have = adc_ok && adc_read_avg(&adc_val);
        }

        /* ---- OLED ---- */
        if (oled_ok) {
            int ok = 1;

            p = line;                                    /* row 3: debounced state */
            for (unsigned i = 0; i < 4; i++) {
                *p++ = (char)('1' + i); *p++ = ':';
                *p++ = (stable & (1u << i)) ? '#' : '-'; *p++ = ' ';
            }
            *p = 0;
            ok = ok && oled_text(3, line);

            p = put_str(line, "RAW PA4-7: ");           /* row 4: raw pin levels */
            for (unsigned i = 0; i < 4; i++) *p++ = (raw & (1u << i)) ? '1' : '0';
            *p = 0;
            ok = ok && oled_text(4, line);

            p = put_str(line, "RED:");                   /* row 6: LEDs */
            p = put_str(p, red ? "ON " : "OFF");
            p = put_str(p, " WHITE:");
            p = put_str(p, white ? "ON" : "OFF");
            *p = 0;
            ok = ok && oled_text(6, line);

            p = put_str(line, "LDR DRIVE: ");           /* row 8 */
            p = put_str(p, ldr_on ? "ON" : "OFF");
            *p = 0;
            ok = ok && oled_text(8, line);

            if (adc_have) {                              /* rows 9, 10: ADC */
                uint32_t pct = ((uint32_t)adc_val * 100u + 2048u) >> 12;
                p = put_str(line, "ADC IN0: ");
                p = put_dec4(p, adc_val);
                p = put_str(p, " ");
                p = put_dec4(p, pct);
                p = put_str(p, "%");
                *p = 0;
                ok = ok && oled_text(9, line);
                ok = ok && oled_bar(10, (uint8_t)(((uint32_t)adc_val * (OLED_BAR_MAX + 1u)) >> 12));
            } else {
                ok = ok && oled_text(9, "ADC ERROR");
            }

            p = put_str(line, "BUZZER:");              /* row 12 */
            p = put_str(p, buzz ? "ON " : "OFF");
            p = put_str(p, " ");
            p = put_dec4(p, tone_hz[tone_idx]);
            p = put_str(p, "HZ");
            *p = 0;
            ok = ok && oled_text(12, line);

            if (!ok) { oled_ok = 0; t_retry = now; }
        } else if ((now - t_retry) >= 1000u) {
            t_retry = now;
            oled_ok = oled_init() && ui_static();
        }
    }
}
