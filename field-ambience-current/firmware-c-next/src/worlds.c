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
        .motion_pct = 40, .age_pct = 30, .echo_pct = 35,
    },
    {
        .name = "Crystal Coast",
        .subtitle = "sunset . waves",
        .accent_r = 180, .accent_g = 245, .accent_b = 240,   /* aqua */
        .space_pct = 30, .atmos_pct = 25,
        .motion_pct = 60, .age_pct = 20, .echo_pct = 15,
    },
    {
        .name = "Midnight Drive",
        .subtitle = "highway . wind",
        .accent_r = 220, .accent_g = 180, .accent_b = 255,   /* violet */
        .space_pct = 40, .atmos_pct = 45,
        .motion_pct = 30, .age_pct = 50, .echo_pct = 55,
    },
    {
        .name = "After Hours",
        .subtitle = "3am . vinyl",
        .accent_r = 255, .accent_g = 205, .accent_b = 150,   /* warm amber */
        .space_pct = 55, .atmos_pct = 50,
        .motion_pct = 20, .age_pct = 70, .echo_pct = 45,
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
