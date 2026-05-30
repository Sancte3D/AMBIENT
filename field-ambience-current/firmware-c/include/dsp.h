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

#endif
