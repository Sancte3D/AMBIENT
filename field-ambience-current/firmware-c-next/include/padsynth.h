#ifndef FAM_PADSYNTH_H
#define FAM_PADSYNTH_H

/*
 * padsynth.{h,c} — spectral bed wavetable (r18.93).
 *
 * MODEL studied: Paul Nasca's PADsynth (the ZynAddSubFX pad engine; the
 * author documents the algorithm with public-domain pseudo/example code).
 * What the model actually is — and why it is exactly our missing sound:
 *
 *   A pad does not get "wide and alive" from stacking detuned oscillators;
 *   it gets there because every partial is a narrow NOISE BAND instead of
 *   a line. PADsynth builds the sound in the FREQUENCY domain: each
 *   harmonic is drawn as a Gaussian bump (bandwidth growing with harmonic
 *   number), every bin gets a RANDOM PHASE, one inverse FFT renders it to
 *   a wavetable that loops PERFECTLY (built from integer bins) and never
 *   exposes a loop point (the fine structure is noise-like). The ensemble/
 *   chorus is IN the table — playback is one interpolated read per sample.
 *
 * This implementation is written fresh for this instrument (no code taken):
 * own iterative radix-2 complex FFT, own per-world harmonic profiles, own
 * bandwidth tuning. Generation is NOT realtime (runs at init/world change,
 * a few ms); playback IS realtime (pad.c reads the table).
 *
 * Memory: table 16384 floats (64 KB) + complex scratch (128 KB), both in
 * this module's .bss → RAM_D1 (deliberately NOT pad.c, whose .bss is
 * steered to DTCM by the linker script). Loop length 16384/44100 ≈ 0.37 s
 * at unity rate — inaudible as a loop by construction.
 */

#include <stdint.h>

#define PADSYNTH_N       16384          /* table length, power of two      */
#define PADSYNTH_F0      110.0f         /* table fundamental (A2) at rate 1 */

/* (Re)build the table for a world timbre profile 0..3 (matches worlds.c
 * indices). Deterministic per (world, seed). Blocking, ~ms — call from
 * init / world change, never the audio path. */
void padsynth_build(int world_idx, uint32_t seed);

/* True once a table exists (pad.c falls back to silence until then). */
int padsynth_ready(void);

/* Interpolated table read at `phase` in samples [0, PADSYNTH_N). The pitch
 * comes from the caller's phase increment: inc = freq_hz / PADSYNTH_F0. */
float padsynth_read(float phase);

/* Table access for tests. */
const float *padsynth_table(void);

#endif
