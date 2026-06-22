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

#define WORLD_COUNT 4

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
    /* Phase 3 slots are intentionally left out for now — they'll be added
     * when the modules they feed exist, so we don't commit to an interface
     * we can't validate yet. */
} world_t;

/* Get the immutable descriptor for a world index. Index is clamped to
 * [0, WORLD_COUNT) — out-of-range returns world 0. */
const world_t *worlds_get(int index);

/* Total worlds known to the firmware. Always == WORLD_COUNT today, kept as
 * a function so a future "load worlds from flash" can grow it. */
int worlds_count(void);

#endif
