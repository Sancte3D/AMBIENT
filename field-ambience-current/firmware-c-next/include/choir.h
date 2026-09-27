/*
 * choir.h — damp organ/choir voice (Moss Fields), r19.56 prototype.
 *
 * A soft, breathy, felt-muted "voice in fog": an ADDITIVE sine stack (organ
 * drawbar-ish partials) doubled into two slightly detuned singers for a choir
 * width, a low breath-noise layer (the "hh" of a choir), a formant vowel peak,
 * and a heavy damp lowpass so the highs are absorbed. Slow breathy swell in,
 * long hold, gentle release. Deliberately a DIFFERENT synthesis family from
 * bowed (saw + bow grain) and horn (reed + blare): all LUT sines, no saw.
 *
 * One choir_note() = one complete sung note. Hot-path safe (LUT sines only).
 */
#ifndef CHOIR_H
#define CHOIR_H

void choir_init(void);
/* Played sources sustain until release; note() remains a timed one-shot. */
void choir_note_on(int source, float freq_hz, float amp);
void choir_note_off(int source);
void choir_all_off(void);
void choir_note(float freq_hz, float amp);
int  choir_active_count(void);
void choir_render_mix(float *dry_L, float *dry_R,
                      float *send_L, float *send_R,
                      int frames, float send_amount);

#endif /* CHOIR_H */
