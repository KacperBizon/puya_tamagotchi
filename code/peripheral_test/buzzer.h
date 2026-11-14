/*
 * Passive buzzer on TIM1_CH2 (PB0), PWM square wave, with a tiny melody player.
 *
 * Writing music: a song is an array of {note, duration in ms} steps.
 *
 *     static const buzzer_step_t jingle[] = {
 *         NOTE(D4, 150), NOTE(A4, 150), NOTE(A3, 300), NOTE(D5, 450),
 *         NOTE(REST, 100),
 *         NOTE(FS4, 200),
 *     };
 *     buzzer_play_song(jingle);
 *
 * Note names: letter + octave, C3..B7. Sharps use S: CS4 = C#4 (= Db4), AS5 = A#5.
 * NOTE(REST, ms) is a pause. Equal temperament, A4 = 440 Hz.
 *
 * Playback is non-blocking and driven by buzzer_tick(), which must be called
 * once per millisecond (hook it to the SysTick, see timebase_set_tick_hook()).
 * Each note starts loud and fades out like a struck bell, with a short silent
 * gap at its end so repeated notes stay distinct.
 *
 * Pin muxing (PB0 = AF2 for TIM1_CH2) is board specific and done by the caller.
 */
#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

typedef enum {
    NOTE_REST = 0,
    NOTE_C3, NOTE_CS3, NOTE_D3, NOTE_DS3, NOTE_E3, NOTE_F3, NOTE_FS3, NOTE_G3, NOTE_GS3, NOTE_A3, NOTE_AS3, NOTE_B3,
    NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
    NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,
    NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,
    NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7,
    NOTE_COUNT
} buzzer_note_t;

typedef struct {
    uint8_t  note;          /* buzzer_note_t */
    uint16_t ms;            /* duration of this step */
} buzzer_step_t;

#define NOTE(n, duration_ms)  { NOTE_##n, (duration_ms) }

void buzzer_init(void);
void buzzer_play(const buzzer_step_t *song, unsigned steps);  /* starts at once, restarts if already playing */
void buzzer_stop(void);
int  buzzer_playing(void);
void buzzer_tick(void);                                       /* call every 1 ms */

#define buzzer_play_song(song)  buzzer_play((song), sizeof(song) / sizeof((song)[0]))

#endif
