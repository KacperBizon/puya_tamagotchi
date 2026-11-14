#include "buzzer.h"
#include "py32.h"

#define TIM1_BASE   0x40012C00UL
#define TIM1_CR1    REG32(TIM1_BASE + 0x00)
#define TIM1_EGR    REG32(TIM1_BASE + 0x14)
#define TIM1_CCMR1  REG32(TIM1_BASE + 0x18)
#define TIM1_CCER   REG32(TIM1_BASE + 0x20)
#define TIM1_PSC    REG32(TIM1_BASE + 0x28)
#define TIM1_ARR    REG32(TIM1_BASE + 0x2C)
#define TIM1_CCR2   REG32(TIM1_BASE + 0x38)
#define TIM1_BDTR   REG32(TIM1_BASE + 0x44)

#define GAP_MS      12u                     /* silent gap at the end of a note */

/* Timer tick is 4 MHz (24 MHz / (PSC 5 + 1)). ARR = 4 MHz / f - 1, rounded.
 * One entry per note from NOTE_C3 (index 0) to NOTE_B7, equal temperament, A4 = 440 Hz. */
static const uint16_t note_arr[NOTE_COUNT - 1] = {
    /* octave 3 */ 30577,28861,27241,25712,24269,22907,21621,20407,19262,18181,17160,16197,
    /* octave 4 */ 15288,14430,13620,12855,12134,11453,10810,10203, 9630, 9090, 8580, 8098,
    /* octave 5 */  7644, 7214, 6809, 6427, 6066, 5726, 5404, 5101, 4815, 4544, 4289, 4049,
    /* octave 6 */  3821, 3607, 3404, 3213, 3033, 2862, 2702, 2550, 2407, 2272, 2144, 2024,
    /* octave 7 */  1910, 1803, 1702, 1606, 1516, 1431, 1350, 1275, 1203, 1135, 1072, 1011,
};

/* Fade-out: PWM duty in 1/256 steps for the four quarters of a note (50 %, 38 %, 22 %, 9 %). */
static const uint16_t duty256[4] = { 128, 96, 56, 24 };

static const buzzer_step_t *g_song;
static unsigned             g_len;
static unsigned             g_pos;
static uint16_t             g_dur;          /* length of the current step, ms */
static uint16_t             g_elapsed;      /* ms into the current step */
static uint16_t             g_arr;          /* 0 = current step is a rest */
static uint8_t              g_level;        /* 0..3 = fade level, 4 = gap */
static volatile uint8_t     g_playing;

static void set_level(uint8_t level)
{
    g_level = level;
    TIM1_CCR2 = (level >= 4u) ? 0u : (((uint32_t)g_arr + 1u) * duty256[level]) >> 8;
}

static void start_step(void)
{
    const buzzer_step_t *s = &g_song[g_pos];

    g_dur     = s->ms ? s->ms : 1u;
    g_elapsed = 0;
    if (s->note == NOTE_REST || s->note >= NOTE_COUNT) {
        g_arr = 0;
        TIM1_CCR2 = 0;
        return;
    }
    g_arr = note_arr[s->note - 1u];
    TIM1_ARR = g_arr;
    set_level(0);
    TIM1_EGR = 1u;                          /* latch ARR/CCR2 now: the note starts exactly on time */
}

void buzzer_init(void)
{
    RCC_APBENR2 |= (1u << 11);              /* TIM1EN */
    TIM1_PSC   = 5;                         /* 24 MHz / 6 = 4 MHz tick */
    TIM1_ARR   = 999;
    TIM1_CCR2  = 0;                         /* duty 0 = silent (pin stays low) */
    TIM1_CCMR1 = (6u << 12) | (1u << 11);   /* OC2M = PWM mode 1, OC2PE preload */
    TIM1_CCER  = (1u << 4);                 /* CC2E: channel 2 output on */
    TIM1_BDTR  = (1u << 15);                /* MOE: main output enable (advanced timer) */
    TIM1_CR1   = (1u << 7);                 /* ARPE */
    TIM1_EGR   = 1u;                        /* UG: load PSC/ARR/CCR */
    TIM1_CR1  |= 1u;                        /* CEN */
    g_playing = 0;
}

void buzzer_play(const buzzer_step_t *song, unsigned steps)
{
    uint32_t irq;

    if (!song || steps == 0) return;
    irq = irq_save();
    g_song = song;
    g_len  = steps;
    g_pos  = 0;
    start_step();
    g_playing = 1;
    irq_restore(irq);
}

void buzzer_stop(void)
{
    uint32_t irq = irq_save();
    g_playing = 0;
    TIM1_CCR2 = 0;
    irq_restore(irq);
}

int buzzer_playing(void) { return g_playing; }

/* Shape the current note: fade through four levels, then go silent for GAP_MS. */
static void update_envelope(void)
{
    uint32_t body, e4;
    uint8_t  level;

    if (g_arr == 0) return;                                 /* rest */
    body = (g_dur > 2u * GAP_MS) ? (uint32_t)(g_dur - GAP_MS) : g_dur;
    if (body < g_dur && g_elapsed >= body) {
        level = 4;
    } else {
        e4 = (uint32_t)g_elapsed << 2;
        level = (uint8_t)((e4 >= body) + (e4 >= 2u * body) + (e4 >= 3u * body));
    }
    if (level != g_level) set_level(level);
}

void buzzer_tick(void)
{
    if (!g_playing) return;
    g_elapsed++;
    if (g_elapsed >= g_dur) {
        if (++g_pos >= g_len) {
            TIM1_CCR2 = 0;
            g_playing = 0;
            return;
        }
        start_step();
        return;
    }
    update_envelope();
}
