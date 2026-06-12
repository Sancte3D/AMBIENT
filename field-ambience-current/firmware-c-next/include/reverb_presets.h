#ifndef FAM_REVERB_PRESETS_H
#define FAM_REVERB_PRESETS_H

/*
 * Per-Mode reverb presets + per-Vibe bias + Space/Mood macros.
 *
 * Direct port of the webapp's REVERB_MODE_PRESETS / VIBE_REVERB_BIAS /
 * computeReverb() (see ../../software/webapp/field_ambience_webapp.html). The webapp computes
 * four abstract params {t60, damp, size, high} suitable for generating a
 * convolution-reverb impulse response. The native Freeverb takes a different
 * shape — size_0_1 (comb feedback), damp_0_1 (LP coefficient in feedback),
 * drive_0_1 (pre-reverb tanh), and a wet/dry mix. The map from one to the
 * other is in reverb_presets_compute(): it preserves the webapp's
 * mode/vibe/space/mood relationships and turns them into the Freeverb
 * parameter set the engine actually drives.
 *
 * Pure data + arithmetic, no audio path. Fully host-testable.
 */

#include <stdbool.h>

/* The 6 modes match brain.h's order (ionian … aeolian = 0..5). */
#define RP_MODE_COUNT  6
/* The 4 vibes match brain.h's order (warm/bright/deep/floating = 0..3). */
#define RP_VIBE_COUNT  4

/* Output of compute(): values ready to pass into the engine setters. */
typedef struct {
    float size;     /* 0..1   → engine_set_reverb_size      */
    float damp;     /* 0..1   → engine_set_reverb_damp      */
    float drive;    /* 0..1   → engine_set_reverb_drive     */
    float wet_amp;  /* 0..1   → engine_set_wet_amp          */
} reverb_settings_t;

/* Compute live Freeverb settings from the current musical state.
 *   mode_idx  : 0..5 (clamped)
 *   vibe_idx  : 0..3 (clamped)
 *   space     : 0..1, user macro — bigger = longer + bigger room
 *   mood      : 0..1, user macro — darker → brighter, also widens slightly
 * Order matches the webapp's computeReverb() so the same controls produce
 * comparable timbres on both ports. */
reverb_settings_t reverb_presets_compute(int mode_idx, int vibe_idx,
                                         float space, float mood);

/* Convenience: pretty-print the eight params for one (mode, vibe) pair at
 * space=0.5/mood=0.5 — used by host tests + debug. Returns the buffer it
 * was given. */
char *reverb_presets_describe(int mode_idx, int vibe_idx, char *buf, int bufsz);

#endif
