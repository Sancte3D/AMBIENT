/*
 * worlds.c — the four curated worlds (ADR-0017 Phase 1).
 *
 * The descriptors used to live as four separate tables inside menu.c
 * (WORLD_NAMES, WORLD_SUBTITLE, WORLD_PRESET, WORLD_ACCENT). This module is
 * the single source of truth so future per-world ambience + drums hooks have
 * one place to grow. The table is `const`/static — no state, no init needed.
 *
 * Naming + subtitle rules:
 *   - name <= 13 chars (Helvetica value font fit at 320 px width)
 *   - subtitle is a flavour line, shown dimmed under the name
 *
 * Accent colour rule: at least one channel near 255 so whites stay bright in
 * the grey→RGB565 cast. See ADR-0015 Step 1.
 */

#include "worlds.h"

static const world_t WORLDS[WORLD_COUNT] = {
    {
        .name = "Tokyo City",
        .subtitle = "night . rain",
        .accent_r = 175, .accent_g = 205, .accent_b = 255,   /* cool blue */
        .space_pct = 42, .atmos_pct = 35,
        .motion_pct = 40, .age_pct = 30, .echo_pct = 35, .blur_pct = 15,
        .shimmer_pct = 12,   /* r18.99: a hint of halo over the rain      */
        /* A-major ionian, warm add9 — "DAS IST ES" dreamy reference */
        .key_midi = 57, .mode = 0, .vibe = 0,
    },
    {
        .name = "Crystal Coast",
        .subtitle = "sunset . waves",
        .accent_r = 180, .accent_g = 245, .accent_b = 240,   /* aqua */
        .space_pct = 30, .atmos_pct = 25,
        .motion_pct = 60, .age_pct = 20, .echo_pct = 15, .blur_pct = 25,
        .shimmer_pct = 22,   /* r18.99: the bright world blooms upward    */
        /* D-major ionian, bright maj7 — sunset optimism, open horizon */
        .key_midi = 62, .mode = 0, .vibe = 1,
    },
    {
        .name = "Midnight Drive",
        .subtitle = "highway . wind",
        .accent_r = 220, .accent_g = 180, .accent_b = 255,   /* violet */
        .space_pct = 40, .atmos_pct = 45,
        .motion_pct = 30, .age_pct = 50, .echo_pct = 55, .blur_pct = 10,
        .shimmer_pct = 6,    /* r18.99: the highway stays grounded        */
        /* F#-minor dorian, deep min11 — moody highway, descending arc */
        .key_midi = 54, .mode = 1, .vibe = 2,
    },
    {
        .name = "After Hours",
        .subtitle = "3am . vinyl",
        .accent_r = 255, .accent_g = 205, .accent_b = 150,   /* warm amber */
        .space_pct = 55, .atmos_pct = 50,
        .motion_pct = 20, .age_pct = 70, .echo_pct = 45, .blur_pct = 35,
        .shimmer_pct = 16,   /* r18.99: faded-glory halo for the bar      */
        /* C-minor aeolian, floating sus2 — 3am jazz bar, lonely but okay */
        .key_midi = 60, .mode = 5, .vibe = 3,
    },
};

const world_t *worlds_get(int index) {
    if (index < 0)              index = 0;
    if (index >= WORLD_COUNT)   index = WORLD_COUNT - 1;
    return &WORLDS[index];
}

int worlds_count(void) {
    return WORLD_COUNT;
}
