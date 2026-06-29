#ifndef FAM_HARMONIC_BASS_H
#define FAM_HARMONIC_BASS_H

/*
 * harmonic_bass — "5th-harmonic" / Exceeder-style electro synth bass.
 * Root oscillator + a second oscillator +28 semitones (≈ the 5th overtone) for
 * the aggressive singing bite, a sub sine for depth, tanh drive + portamento
 * glide for the bendy slides. Mono voice, ADDS into the dry/send bus.
 * See harmonic_bass.c.
 */

void harmonic_bass_init(void);

/* Note-on / retrigger. If sounding, the pitch glides (bendy slide); from
 * silence the pitch snaps. freq_hz is the root (the +28 osc is derived). */
void harmonic_bass_note(float freq_hz);
void harmonic_bass_release(void);

/* Parameters, all user-facing 0..1 (mapped to internal ranges). */
void harmonic_bass_set_bite(float v);    /* +28 harmonic-osc level (the bite) */
void harmonic_bass_set_drive(float v);   /* tanh saturation amount */
void harmonic_bass_set_tone(float v);    /* lowpass brightness */
void harmonic_bass_set_glide(float v);   /* portamento time (bendy slides) */
void harmonic_bass_set_level(float v);   /* master voice level */

/* Per-block render: ADDS into dry + reverb-send buses (mono → both L/R). */
void harmonic_bass_render_mix(float *dry_L, float *dry_R,
                              float *send_L, float *send_R, int frames);

#endif /* FAM_HARMONIC_BASS_H */
