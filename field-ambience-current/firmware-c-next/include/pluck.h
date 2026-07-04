#ifndef FAM_PLUCK_H
#define FAM_PLUCK_H

/*
 * pluck.{h,c} — Karplus-Strong plucked-string voices (r18.89).
 *
 * The generative "sparkle" chord tones used to be pad voices — the same
 * timbre as the bed, so they read as "the pad got louder" instead of a
 * second instrument. This module gives them their own colour: a short
 * lowpassed noise burst rings through a damped delay loop → a bell/koto
 * pluck that blooms into the reverb. Classic Karplus-Strong (1983 CMJ
 * paper; SuperCollider's Pluck UGen is the same idea) written fresh here:
 *
 *   buf[N] ring; per sample:  y = lerp-read(buf, pos)
 *                             buf[write] = rho · ((1−damp)·y + damp·y_prev)
 *
 *   - loop length N = SR/f with LINEAR-INTERPOLATED fractional read, so
 *     pitch is exact (integer-N KS is up to ~30 cents off at 1.6 kHz);
 *   - rho from a target T60 (~3 s), pitch-independent: rho = 0.001^(1/(f·T60))
 *   - damp blends in last sample = the classic averaging lowpass; higher
 *     damp = softer/darker pluck.
 *
 * Voices self-decay — there is no note-off. Fixed small pool, round-robin.
 * Hardware-independent, fixed seeds, host-tested (test_sound_upgrades.c).
 */

#include <stdint.h>

#define PLUCK_VOICES 2
#define PLUCK_MIN_HZ 60.0f      /* buffer sized for this floor */

void pluck_init(void);

/* Start a pluck: freq in Hz (clamped ≥ PLUCK_MIN_HZ), amp 0..1 peak-ish.
 * Steals the oldest voice when the pool is full. */
void pluck_note(float freq_hz, float amp);

/* Voices still audibly ringing (energy above ~-72 dBFS). */
int pluck_active_count(void);

/* Mix into the engine's dry + reverb-send accumulators. Stereo: voice 0
 * sits slightly left, voice 1 slightly right (alternating round-robin →
 * call-answer movement). */
void pluck_render_mix(float *dry_L, float *dry_R,
                      float *send_L, float *send_R, int frames);

#endif
