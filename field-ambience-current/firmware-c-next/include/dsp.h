#ifndef FAM_DSP_H
#define FAM_DSP_H

/*
 * Shared DSP helpers for the native engine.
 *
 * Embedded-friendly: a sine lookup table (no per-sample sinf in the audio
 * path) plus small math utilities used across voices, bass, pad, texture,
 * and reverb in later steps.
 *
 * Conventions:
 *   - "phase" is in TURNS, range [0,1). 0.25 = 90°, 0.5 = 180°. Wrap with
 *     fractional part. This keeps phase accumulation cheap (add freq/SR,
 *     subtract 1.0 when >= 1.0) and LUT indexing trivial.
 */

#include <stdint.h>
#include <math.h>     /* tanhf in the r18.89 drive-shaper inlines */

#define DSP_SAMPLE_RATE_HZ 44100

/* Build the sine LUT. Call once at boot before any dsp_sin(). */
void dsp_init(void);

/* Sine of a phase in turns [0,1) (values outside are wrapped). Linear-
 * interpolated 1024-point table — smooth enough for clean tones, far
 * cheaper than sinf in the render loop. Returns -1.0..+1.0. */
float dsp_sin(float phase_turns);

/* MIDI note number → frequency in Hz (A4=69=440 Hz). Uses powf — call at
 * note-on, not per sample. */
float dsp_midi_to_hz(float midi);

/* Clamp helper. */
static inline float dsp_clampf(float x, float lo, float hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

/* ------------------------------------------------------------------ Step 9
 * Oscillators + filter + smoothing for famPadCore (and the bass/texture
 * steps that follow). All phase arguments are in TURNS [0,1) like dsp_sin;
 * `dt` is the per-sample phase increment (freq / SR), needed by the band-
 * limiting (polyBLEP) so the saw/pulse edges don't alias into a harsh buzz —
 * the Sound Constitution forbids that aliasing fizz.
 */

/* Band-limited sawtooth, rising −1→+1 over the cycle. polyBLEP-corrected so
 * the wrap discontinuity is anti-aliased. */
float dsp_poly_saw(float phase_turns, float dt);

/* Band-limited square (50% duty), ±1. Two polyBLEPs (rising + falling edge). */
float dsp_poly_square(float phase_turns, float dt);

/* One-pole smoothing coefficient for an exponential approach to a target with
 * time-constant `tau_s` (the WebAudio setTargetAtTime model). Use as:
 *   y += coef * (target - y);   // per sample
 * Returns a value in (0,1]; larger tau → smaller coef → slower glide. */
float dsp_smooth_coef(float tau_s);

/* TPT (topology-preserving transform) state-variable filter — the Cytomic /
 * Zavalishin form. Chosen over a biquad because the pad sweeps its cutoff
 * continuously (LFO + filter-envelope + brightness); this structure stays
 * stable and zipper-free under per-block coefficient updates where a Direct-
 * Form biquad would click. One instance gives a 2-pole lowpass with Q. */
typedef struct {
    float ic1eq, ic2eq;     /* integrator state */
    float a1, a2, a3;       /* derived coefficients */
    float k;                /* 1/Q — kept so the bandpass output can normalise */
} dsp_svf_t;

/* Reset filter state to silence. */
void dsp_svf_reset(dsp_svf_t *s);

/* (Re)compute coefficients for cutoff `fc_hz` and quality `q`. Cheap-ish
 * (one tanf) — call at control rate (e.g. every 16–32 samples), not per
 * sample. fc is clamped to a safe (0, Nyquist) band internally. */
void dsp_svf_set(dsp_svf_t *s, float fc_hz, float q);

/* Process one sample, returning the lowpass output. */
float dsp_svf_lp(dsp_svf_t *s, float x);

/* Process one sample, returning the bandpass output, normalised to ≈unity
 * peak gain at the centre frequency (the Web-Audio bandpass convention).
 * Use a DIFFERENT filter instance than dsp_svf_lp — each instance advances
 * its own state, so one instance produces one output type. */
float dsp_svf_bp(dsp_svf_t *s, float x);

/* Process one sample, returning the highpass output. As with bp, use a
 * dedicated instance (one output type per instance). */
float dsp_svf_hp(dsp_svf_t *s, float x);

/* Naive triangle from a phase in turns [0,1): +1 at 0, −1 at 0.5. Harmonics
 * fall off as 1/n² so aliasing is negligible at the low frequencies the bass
 * uses — no band-limiting needed. */
static inline float dsp_tri(float phase_turns) {
    float p = phase_turns - (float)(int)phase_turns;
    if (p < 0.0f) p += 1.0f;
    return 4.0f * (p < 0.5f ? (0.25f - p) : (p - 0.75f)) ;  /* +1→−1→+1 */
}

/* --- r18.89 noise + drive primitives ---------------------------------------
 * Concepts studied from SuperCollider's PinkNoise/Dust/Crackle UGens and the
 * standard DSP literature (Kellet pink filter, logistic-map chaos, waveshaper
 * saturation); implementations written fresh for this engine — fixed-seed
 * LCGs so every host test stays bit-reproducible. */

/* Pink (1/f) noise — Paul Kellet's "economy" 3-pole filter over white noise.
 * ≈ ±0.6 peak output; -3 dB/oct within ±0.5 dB across the audible band,
 * which is what makes it sound "flat" to the ear where white sounds hissy
 * and brown sounds muddy. */
typedef struct { uint32_t lcg; float b0, b1, b2; } dsp_pink_t;
void  dsp_pink_seed(dsp_pink_t *p, uint32_t seed);
float dsp_pink(dsp_pink_t *p);

/* Dust — random unipolar impulses, `density` expected impulses per second
 * (SuperCollider Dust concept). Returns 0 almost always; on a hit returns a
 * random amplitude in (0..1]. Feed a resonator/BP for vinyl ticks, rain, …*/
typedef struct { uint32_t lcg; float thresh; } dsp_dust_t;
void  dsp_dust_seed(dsp_dust_t *d, uint32_t seed);
void  dsp_dust_set_density(dsp_dust_t *d, float per_second);
float dsp_dust(dsp_dust_t *d);

/* Crackle — chaotic logistic map x' = param·x·(1−x), param 1.0..1.99
 * (SuperCollider Crackle concept). Output roughly ±1, gets noisier/denser
 * toward 1.99. Nice as a very quiet fire/vinyl base layer. */
typedef struct { float y1, y2; float param; } dsp_crackle_t;
void  dsp_crackle_init(dsp_crackle_t *c, float param);
float dsp_crackle(dsp_crackle_t *c);

/* Asymmetric soft saturator for the master DRIVE stage.
 *   y = tanh(g·x + bias) − tanh(bias)
 * The bias term skews the transfer curve → even harmonics (tube-ish warmth);
 * subtracting tanh(bias) re-centres it so silence stays silence. Small-signal
 * gain is g·(1−tanh²(bias)) — divide by dsp_drive_makeup() to keep perceived
 * level steady while the harmonics build. */
static inline float dsp_drive_shape(float x, float g, float bias) {
    return tanhf(g * x + bias) - tanhf(bias);
}
static inline float dsp_drive_makeup(float g, float bias) {
    float t = tanhf(bias);
    float small_gain = g * (1.0f - t * t);
    return small_gain > 1.0f ? 1.0f / small_gain : 1.0f;
}

#endif
