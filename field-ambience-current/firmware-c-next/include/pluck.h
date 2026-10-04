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
 *   - read delay SR/f - damp compensates the loop filter's phase delay;
 *     linear interpolation retains a small high-frequency tuning residual;
 *   - rho from a target T60 (~3 s), pitch-independent: rho = 0.001^(1/(f·T60))
 *   - damp blends in last sample = the classic averaging lowpass; higher
 *     damp = softer/darker pluck.
 *
 * Voices self-decay or take a 20 ms owned stop. Fixed pool; no hard stealing.
 * Hardware-independent, fixed seeds, host-tested (test_sound_upgrades.c).
 */

#include <stdint.h>
#include <stdbool.h>

#define PLUCK_VOICES 2
#define PLUCK_MIN_HZ 60.0f      /* buffer sized for this floor */

void pluck_init(void);

/* Start a pluck: freq in Hz (clamped ≥ PLUCK_MIN_HZ), amp 0..1 peak-ish.
 * Legacy unowned one-shot. A full pool declines it without truncating tails. */
void pluck_note(float freq_hz, float amp);

/* Source-owned start. False means invalid input, source still ringing, or
 * no free slot; the caller must not register a successful onset in that case. */
bool pluck_note_on(uint8_t source, float freq_hz, float amp);
void pluck_note_off(uint8_t source);
void pluck_all_off(void); /* affects owned and legacy one-shots */

/* Voices still audibly ringing (energy above ~-72 dBFS). */
int pluck_active_count(void);

/* r18.90 — loop-filter damping 0..0.9 (0.42 default). Driven by the
 * BRIGHTNESS macro: dark = softer, rounder plucks; bright = glassier.
 * Live edits use 80 ms smoothing with matching delay compensation. */
void pluck_set_damp(float damp);

/* Mix into the engine's dry + reverb-send accumulators. Stereo: voice 0
 * sits slightly left, voice 1 slightly right (alternating round-robin →
 * call-answer movement). */
void pluck_render_mix(float *dry_L, float *dry_R,
                      float *send_L, float *send_R, int frames);

#endif
