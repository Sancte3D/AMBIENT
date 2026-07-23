/*
 * bowed.h — bowed-string voice (lyra / Hardanger-inspired), r19.46.
 *
 * A warm, sustained, self-completing "bow stroke": swells in, sings, fades —
 * band-limited saw string body + continuous bow-noise grain + a resonant wood
 * body + two sympathetic resonators + slow bow vibrato. Deliberately NOT a
 * plucked "ding" and NOT a friction-model scrape (both forbidden by the
 * location brief) — it is a new synth voice *influenced* by bowed instruments.
 *
 * One bowed_note() call = one complete stroke (attack/hold/release baked in),
 * so sparse triggers overlap into a continuous bowed texture. Alias-free
 * (dsp_poly_saw), LUT sines only (control-rate) — hot-path safe.
 */
#ifndef BOWED_H
#define BOWED_H

void bowed_init(void);

/* Start one bow stroke at freq_hz, peak amplitude amp (0..1). Allocates a
 * voice (steals the quietest if full). The stroke completes on its own. */
void bowed_note(float freq_hz, float amp);

/* Optional "colour" per world: 0 = Open Sea lyra (warm, mid body), 1 = Fjords
 * Hardanger (darker, more sympathetic ring). Set before bowed_note(). */
void bowed_set_colour(int colour);

int  bowed_active_count(void);

/* Mixes the voices into dry (+ a copy into the reverb send). */
void bowed_render_mix(float *dry_L, float *dry_R,
                      float *send_L, float *send_R,
                      int frames, float send_amount);

#endif /* BOWED_H */
