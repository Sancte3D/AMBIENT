/*
 * test_worlds.c — single source of truth for the 4 curated worlds
 * (ADR-0017 Phase 1).
 */

#include "worlds.h"

#include <stdio.h>
#include <string.h>

static int g_checks = 0, g_fails = 0;
#define CHECK(c, ...) do { ++g_checks; if (!(c)) { ++g_fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

int main(void) {
    /* exactly four worlds, in order Tokyo → Coast → Drive → After Hours */
    CHECK(worlds_count() == 4, "worlds_count = %d", worlds_count());

    const char *expect_names[4]    = { "Tokyo City", "Crystal Coast",
                                       "Midnight Drive", "After Hours" };
    const char *expect_subtitles[4] = { "night . rain", "sunset . waves",
                                        "highway . wind", "3am . vinyl" };
    /* the accent table from the original menu.c — these values must be
     * preserved byte-for-byte by the lift, otherwise the UI tint shifts */
    uint8_t expect_accent[4][3] = {
        { 175, 205, 255 },
        { 180, 245, 240 },
        { 220, 180, 255 },
        { 255, 205, 150 },
    };
    /* same preservation requirement for macro presets:
     * space, atmos, motion, age, echo. Tone dropped (duplicate of
     * Brightness encoder); Drums dropped (adaptive drums = own can of
     * worms). Echo added in the post-Phase-4 perform-macros pass. */
    uint8_t expect_preset[4][5] = {
        /* space, atmos, motion, age, echo */
        { 42, 35, 40, 30, 35 },
        { 30, 25, 60, 20, 15 },
        { 40, 45, 30, 50, 55 },
        { 55, 50, 20, 70, 45 },
    };

    for (int i = 0; i < 4; ++i) {
        const world_t *w = worlds_get(i);
        CHECK(w != NULL, "worlds_get(%d) NULL", i);
        CHECK(strcmp(w->name, expect_names[i]) == 0,
              "name[%d] = %s, want %s", i, w->name, expect_names[i]);
        CHECK(strcmp(w->subtitle, expect_subtitles[i]) == 0,
              "subtitle[%d] = %s, want %s", i, w->subtitle, expect_subtitles[i]);
        CHECK(w->accent_r == expect_accent[i][0] &&
              w->accent_g == expect_accent[i][1] &&
              w->accent_b == expect_accent[i][2],
              "accent[%d] = (%d,%d,%d), want (%d,%d,%d)", i,
              w->accent_r, w->accent_g, w->accent_b,
              expect_accent[i][0], expect_accent[i][1], expect_accent[i][2]);
        CHECK(w->space_pct  == expect_preset[i][0] &&
              w->atmos_pct  == expect_preset[i][1] &&
              w->motion_pct == expect_preset[i][2] &&
              w->age_pct    == expect_preset[i][3] &&
              w->echo_pct   == expect_preset[i][4],
              "preset[%d] = (%d,%d,%d,%d,%d), want (%d,%d,%d,%d,%d)", i,
              w->space_pct, w->atmos_pct, w->motion_pct, w->age_pct, w->echo_pct,
              expect_preset[i][0], expect_preset[i][1],
              expect_preset[i][2], expect_preset[i][3], expect_preset[i][4]);
        /* every accent must keep at least one channel near full so whites
         * stay bright in the grey→RGB565 cast (ADR-0015) */
        int max_ch = w->accent_r;
        if (w->accent_g > max_ch) max_ch = w->accent_g;
        if (w->accent_b > max_ch) max_ch = w->accent_b;
        CHECK(max_ch >= 240, "accent[%d] too dark (max=%d)", i, max_ch);
    }

    /* out-of-range index must clamp, not crash */
    CHECK(worlds_get(-1) == worlds_get(0), "clamp -1 → 0");
    CHECK(worlds_get(99) == worlds_get(3), "clamp 99 → 3");

    /* every world must have a non-empty name + subtitle */
    for (int i = 0; i < worlds_count(); ++i) {
        const world_t *w = worlds_get(i);
        CHECK(w->name[0] != '\0',    "name[%d] empty", i);
        CHECK(w->subtitle[0] != '\0', "subtitle[%d] empty", i);
    }

    printf("%d checks, %d failures\n", g_checks, g_fails);
    printf("RESULT: %s\n", g_fails ? "FAIL" : "PASS");
    return g_fails ? 1 : 0;
}
