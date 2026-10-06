#ifndef FAM_PLUCK_H
#define FAM_PLUCK_H

/*
 * pluck.{h,c} — Karplus-Strong plucked-string voices (r18.89).
 *
 * Dry source candidate for WOODLAND: a rounded string onset with a clear
 * fundamental and a natural ring. Up to eight harmonic displacement modes,
 * small seeded pluck-position variation, zero-mean/peak-bounded excitation
 * and a 4..32 ms smooth attack replace the old arbitrary noise burst.
 * dsp_init() must run before starting notes. No samples/SD or new delay pool.
 * The existing Karplus-Strong feedback remains:
 *
 *   buf[N] ring; per sample:  y = lerp-read(buf, pos)
 *                             buf[write] = rho · (a·y + (1−2a)·y_prev + a·y_prev2)
 *
 *   - read delay SR/f - 1 compensates the symmetric filter's fixed delay;
 *     linear interpolation retains a small high-frequency tuning residual;
 *   - rho sets nominal loss from T60 (~3 s); damping/interpolation shorten
 *     the actual decay, especially in the upper register;
 *   - damp 0..0.9 maps to a 0..0.25; increasing it consistently removes upper
 *     modes without moving the read head or brightening again at the end;
 *   - an output-only ~10 Hz DC tracker removes residual startup bias.
 *
 * Voices self-decay or take a 20 ms owned stop. Fixed pool; no hard stealing.
 * Hardware-independent, fixed seeds, host-tested (test_sound_upgrades.c).
 */

#include <stdint.h>
#include <stdbool.h>

#define PLUCK_VOICES 2
#define PLUCK_MIN_HZ 60.0f      /* buffer sized for this floor */

void pluck_init(void);

/* Start a pluck: freq in Hz (clamped ≥ PLUCK_MIN_HZ), amp 0..1 excitation peak.
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
 * BRIGHTNESS macro controls the loss of upper modes, not an added hiss layer.
 * Live edits use 80 ms smoothing; the compensated delay stays fixed. */
void pluck_set_damp(float damp);

/* Mix into the engine's dry + reverb-send accumulators. Stereo: voice 0
 * sits slightly left, voice 1 slightly right (alternating round-robin →
 * call-answer movement). */
void pluck_render_mix(float *dry_L, float *dry_R,
                      float *send_L, float *send_R, int frames);

#endif
