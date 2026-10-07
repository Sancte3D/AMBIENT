/*
 * bowed.h — bowed-string voice (lyra / Hardanger-inspired), r19.46.
 *
 * A warm, sustained, self-completing "bow stroke": swells in, sings, fades —
 * band-limited saw string body + restrained bow-noise grain + a resonant wood
 * body + two sympathetic resonators. Stable pitch; slow body-colour breath
 * without a detuned companion. Deliberately NOT a
 * plucked "ding" and NOT a friction-model scrape (both forbidden by the
 * location brief) — it is a new synth voice *influenced* by bowed instruments.
 *
 * One bowed_note() call = one complete stroke (attack/hold/release baked in),
 * so sparse triggers overlap into a continuous bowed texture. Alias-free
 * (dsp_poly_saw), LUT sines only (control-rate) — hot-path safe.
 */
#ifndef BOWED_H
#define BOWED_H

#include <stdbool.h>
#include <stdint.h>

void bowed_init(void);

/* Start one bow stroke at freq_hz, peak amplitude amp (0..1). Allocates a
 * voice (steals the quietest if full). The stroke completes on its own. */
/* Played sources sustain until release; note() remains a timed one-shot. */
void bowed_note_on(int source, float freq_hz, float amp);
/* Strict World admission: idle slot only, releases/queued voices stay occupied.
 * False leaves every live voice untouched. Legacy note_on keeps its handover. */
bool bowed_try_note_on(int source, float freq_hz, float amp);
void bowed_note_off(int source);
void bowed_all_off(void);
#ifdef FAM_SOUND_PRODUCT
/* Audio-context only; natural tail remains in the shared room. */
void bowed_quiet_source(int source,int frames);
#endif
void bowed_note(float freq_hz, float amp);

/* Optional "colour" per world: 0 = Open Sea lyra (warm, mid body), 1 = Fjords
 * Hardanger (darker, more sympathetic ring). Set before bowed_note(). */
void bowed_set_colour(int colour);

int  bowed_active_count(void);
/* Audio-owned ticket query, including released sources and prepared starts. */
uint16_t bowed_active_sources(void);
void bowed_set_tone(float tone_0_1);

/* Mixes the voices into dry (+ a copy into the reverb send). */
void bowed_render_mix(float *dry_L, float *dry_R,
                      float *send_L, float *send_R,
                      int frames, float send_amount);

#endif /* BOWED_H */
