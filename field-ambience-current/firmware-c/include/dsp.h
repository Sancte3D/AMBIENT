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
} dsp_svf_t;

/* Reset filter state to silence. */
void dsp_svf_reset(dsp_svf_t *s);

/* (Re)compute coefficients for cutoff `fc_hz` and quality `q`. Cheap-ish
 * (one tanf) — call at control rate (e.g. every 16–32 samples), not per
 * sample. fc is clamped to a safe (0, Nyquist) band internally. */
void dsp_svf_set(dsp_svf_t *s, float fc_hz, float q);

/* Process one sample, returning the lowpass output. */
float dsp_svf_lp(dsp_svf_t *s, float x);

#endif
