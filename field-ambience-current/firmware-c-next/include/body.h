#ifndef FAM_BODY_H
#define FAM_BODY_H

/*
 * body.{h,c} — modal resonant body for the pluck voice (r18.94).
 *
 * CONCEPT studied from Mutable Rings/Elements and the STK modal models
 * (implementation fresh, no code taken): a physical instrument is not a
 * string alone — the string drives a BODY whose resonant modes are FIXED.
 * The note varies, the body does not; that fixed coloration is what makes
 * a guitar sound like *that* guitar on every fret. Rings' deeper lesson:
 * the mode FREQUENCY RATIOS are the material (harmonic ≈ string, stretched
 * ≈ glass/bars, dense-inharmonic ≈ wood/plates) and the mode DECAYS are
 * its size and damping.
 *
 * Here: a bank of up to 8 two-pole resonators in parallel, one material
 * per WORLD (wood / glass / dark metal / felt), processing ONLY the pluck
 * bus (the PADsynth bed stays clean — one body per instrument, not a
 * body on the room). Stereo: the right channel's modes sit 0.7 % higher —
 * the "you are in front of the instrument, not inside a mono speaker"
 * width trick. Dry/wet fixed per design (0 = bit-exact bypass).
 *
 * CPU: ≤ 8 modes × 2 ch × ~6 ops ≈ 100 ops/sample — trivial.
 * No allocations, fixed state, host-tested (impulse-ring + Goertzel).
 */

#include <stdint.h>

#define BODY_MAX_MODES 8

void body_init(void);

/* Select the material for a world index 0..3 (matches worlds.c). */
void body_set_world(int world_idx);

/* Wet amount 0..1 (0 = exact passthrough). Default 0.38. */
void body_set_amount(float v01);

/* Process the pluck bus in place (stereo). */
void body_process(float *L, float *R, int frames);

#endif
