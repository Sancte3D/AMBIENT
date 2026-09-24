/*
 * horn.h — rounded breath-shaped Alps source; Horn is its legacy internal ID.
 *
 * Phase-aligned fundamental + restrained saw partials, gentle non-resonant
 * breath filter and a short quiet air onset. No sub octave or fixed vowel.
 * Distinct from Bowed's detuned strings and continuous bow grain.
 * note_on/off owns a sustained source; note() is a timed one-shot.
 * Three voices with bounded handovers. LUT oscillator work, no audio allocation.
 */
#ifndef HORN_H
#define HORN_H

void horn_init(void);

/* Start one blown note at freq_hz, peak amplitude amp (0..1). Allocates a
 * voice (steals the quietest if full). The note completes on its own. */
/* Played sources sustain until release; note() remains a timed one-shot. */
void horn_note_on(int source, float freq_hz, float amp);
void horn_note_off(int source);
void horn_all_off(void);
void horn_note(float freq_hz, float amp);

int  horn_active_count(void);

/* Mixes the voices into dry (+ a copy into the reverb send). */
void horn_render_mix(float *dry_L, float *dry_R,
                     float *send_L, float *send_R,
                     int frames, float send_amount);

#endif /* HORN_H */
