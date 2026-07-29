/*
 * horn.h — blown-brass / alphorn-inspired voice (Alps), r19.48 prototype.
 *
 * Deliberately a DIFFERENT synthesis family from bowed.c so the worlds do not
 * sound like one instrument re-coloured:
 *   bowed = detuned saw ensemble + continuous bow grain + resonant wood body
 *           + sympathetic string resonators (a stroked string).
 *   horn  = ONE reed saw whose filter BLARES open on the attack and mellows on
 *           the sustain (the brass "blat"), a fixed horn FORMANT vowel, a short
 *           AIR CHIFF at the onset only, a sub octave for body — NO sympathetic
 *           resonators, no continuous grain (a blown lip-reed, not a string).
 *
 * One horn_note() = one complete blown note (attack/hold/release baked in), so
 * sparse triggers overlap into a sustained alpine call. Alias-free
 * (dsp_poly_saw), LUT sines only (control-rate) — hot-path safe.
 */
#ifndef HORN_H
#define HORN_H

void horn_init(void);

/* Start one blown note at freq_hz, peak amplitude amp (0..1). Allocates a
 * voice (steals the quietest if full). The note completes on its own. */
void horn_note(float freq_hz, float amp);

int  horn_active_count(void);

/* Mixes the voices into dry (+ a copy into the reverb send). */
void horn_render_mix(float *dry_L, float *dry_R,
                     float *send_L, float *send_R,
                     int frames, float send_amount);

#endif /* HORN_H */
