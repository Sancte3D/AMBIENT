#ifndef FAM_WORLDS_H
#define FAM_WORLDS_H

/*
 * Worlds — single source of truth for the 4 curated worlds (ADR-0017 Phase 1).
 *
 * A "world" is a high-level preset the user thinks in pictures (night city,
 * sunset coast, night highway, jazz bar) rather than synth-engine parameters.
 * Each world bundles:
 *   - human-facing identity (name, flavour subtitle)
 *   - the display accent colour (UI tint, used by oled_color)
 *   - the default values for the four global macros (space / tone / atmos)
 *   - (header slots reserved for Phase 2/3 ambience + drums config)
 *
 * Why this module exists: until r18.47 the four per-world tables
 * (WORLD_NAMES, WORLD_SUBTITLE, WORLD_PRESET, WORLD_ACCENT) were scattered
 * across menu.c. Future per-world ambience (Phase 2) and drums (Phase 3)
 * need a sane place to live; that's this file.
 */

#include <stdint.h>

#define WORLD_COUNT 5   /* r19.44: 5 landscape worlds (was 4 city worlds) */

typedef struct {
    const char *name;              /* short display name (<=13 chars)         */
    const char *subtitle;          /* flavour line under the value text       */
    uint8_t     accent_r;          /* UI accent — Grau→RGB565-Tint            */
    uint8_t     accent_g;
    uint8_t     accent_b;
    uint8_t     space_pct;         /* macro defaults (0..100) — loaded on    */
    uint8_t     atmos_pct;         /* world-change; user can then nudge from  */
    uint8_t     motion_pct;        /* LFO-Depth / Pad-Movement (Reddit Motion)*/
    uint8_t     age_pct;           /* Tape-Hiss + Saturation (Reddit Age)     */
    uint8_t     echo_pct;          /* Tape-style stereo delay (Reddit Echo)   */
    uint8_t     blur_pct;          /* Granular cloud / smear (Reddit Blur)    */
    uint8_t     shimmer_pct;       /* r18.99: octave-up hall regeneration      */
    /* Musical identity (harmonic brain). On world change the engine applies
     * key + mode + vibe so each world sounds harmonically distinct, not just
     * texturally. Values from the tools/render_worlds.c audition. */
    uint8_t     key_midi;          /* tonic MIDI note (brain_set_key)         */
    uint8_t     mode;              /* 0..5 ionian..aeolian (brain_set_mode)   */
    uint8_t     vibe;              /* 0..3 warm/bright/deep/floating (vibe)   */
    /* r19.36: curated HARMONY character per world — the chord colour + bass
     * mode loaded on world-change (like the macros; the user can nudge after).
     * Makes the four worlds feel harmonically distinct, not just texturally. */
    uint8_t     chord_color;       /* 0 Pure / 1 Open / 2 Warm / 3 Deep       */
    uint8_t     bass_mode;         /* 0 Off / 1 Root / 2 Fifth / 3 Drift      */
    /* r19.45: per-world brightness (pad filter cutoff + fx tone + reverb
     * damping), Hz offset in [-600, +800]. THE strongest timbral lever — dark
     * worlds go negative, bright/open worlds positive. Loaded on world-change
     * like the macros; the BRIGHT encoder then nudges from here. */
    int16_t     brightness_hz;
} world_t;

/* Get the immutable descriptor for a world index. Index is clamped to
 * [0, WORLD_COUNT) — out-of-range returns world 0. */
const world_t *worlds_get(int index);

/* Total worlds known to the firmware. Always == WORLD_COUNT today, kept as
 * a function so a future "load worlds from flash" can grow it. */
int worlds_count(void);

#endif
