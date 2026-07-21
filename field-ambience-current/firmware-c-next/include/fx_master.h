/*
 * fx_master.h — product adapter around the ambient effects engine
 * (ambient_effects.h). Owns the control storage and the hot delay arena in
 * ONE translation unit so the target linker can steer this object's .bss
 * into internal RAM_D2 — the space reclaimed from the legacy master
 * echo/blur delay lines the engine replaces.
 *
 * Contract (REALTIME_AUDIO_RULES.md): fx_master_init() runs once at boot,
 * OUTSIDE the audio callback. fx_master_process() is hot-path safe (no
 * heap, bounded work, LUT-only transcendentals — verified by the engine's
 * property suite). If init fails the adapter FAILS CLOSED: process becomes
 * a bit-exact dry pass-through and every setter is a no-op.
 */
#ifndef FX_MASTER_H
#define FX_MASTER_H

#include <stdbool.h>
#include <stdint.h>

void fx_master_init(void);
bool fx_master_ok(void);

/* In-place stereo master processing on the final float mix (pre-limiter). */
void fx_master_process(float *outL, float *outR, int frames);

/* Product-macro routing (0..1 each). Each updates one engine parameter and
 * leaves the rest of the cached parameter set untouched. */
void fx_master_set_space(float v);
void fx_master_set_atmosphere(float v);
void fx_master_set_echo(float v);
void fx_master_set_motion(float v);
void fx_master_set_age(float v);
void fx_master_set_shimmer(float v);
void fx_master_set_blur(float v);
void fx_master_set_tone(float v);

/* World voicing: product world index 0..3 maps 1:1 onto the engine worlds
 * (Tokyo City / Crystal Coast / Midnight Drive / After Hours). Loads the
 * engine's per-world parameter set; the menu's macro pushes then overwrite
 * the user-facing fields on top (same order as a manual world load). */
void fx_master_set_world(int idx);

/* Effect-page selection: 0=Bypass .. 8=Dream Chain (AmbientFxMode order).
 * Dream Chain is the boot default. */
void fx_master_set_mode(int idx);
int  fx_master_mode(void);
const char *fx_master_mode_name(int idx);
int  fx_master_mode_count(void);

/* Reverse swell for notes the composer has scheduled in advance. Call
 * lead_seconds before the scheduled note (control rate, main loop) with the
 * note's frequency and a modest level. */
void fx_master_trigger_swell(float freq_hz, float level_0_1,
                             float lead_seconds);

#endif /* FX_MASTER_H */
