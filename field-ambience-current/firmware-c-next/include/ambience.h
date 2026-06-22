#ifndef FAM_AMBIENCE_H
#define FAM_AMBIENCE_H

/*
 * Ambience — per-world atmospheric layer (ADR-0017 Phase 2).
 *
 * Adds a stereo atmospheric bed (wind / rain / waves / vinyl crackle,
 * depending on the active world) into the engine mix bus. Sits *next to*
 * pad/texture/bass/drone/reverb, not replacing any of them — texture is a
 * static brown-noise bed (always on, world-independent); ambience is the
 * narrative layer that makes Tokyo feel like night-rain and Crystal Coast
 * feel like sunset-waves.
 *
 * Phase 2a (this commit): WIND only — universal resonant-BP-on-pink-noise
 * with slow centre sweep + amplitude gusts. Lifted verbatim from
 * tools/render_worlds.c. The other three generators (rain, waves, vinyl)
 * are stubbed silent and arrive in Phase 2b/c/d.
 *
 * Level is the user-facing "atmos" macro, smoothed inside this module so
 * the engine block-rate setter doesn't zipper.
 */

#include <stdint.h>

void ambience_init(void);

/* Pick which world's ambience generator(s) to drive. Today (Phase 2a) wind
 * runs unconditionally; the world index will start gating which generator
 * type plays in Phase 2b. Out-of-range clamps to 0. */
void ambience_set_world(int world_idx);

/* Overall mix level (0..1). Smoothed per-block. 0 = silent (renderer is
 * still called but adds nothing). */
void ambience_set_level(float level_0_1);

/* Add the ambient layer into the engine's dry + reverb-send buses.
 * Mirrors texture_render_mix's signature. send_amount routes a copy of the
 * ambience signal into the reverb input. */
void ambience_render_mix(float *dry_L, float *dry_R,
                         float *send_L, float *send_R,
                         int frames, float send_amount);

#endif
