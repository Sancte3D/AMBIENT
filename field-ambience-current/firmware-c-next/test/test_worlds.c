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
    /* r19.44: five LANDSCAPE worlds, in order Alps → Open Sea → Fjords →
     * Moss Fields → Desert (was the four nocturnal-city worlds). */
    CHECK(worlds_count() == 5, "worlds_count = %d", worlds_count());

    const char *expect_names[5]    = { "Alps", "Open Sea", "Fjords",
                                       "Moss Fields", "Desert" };
    const char *expect_subtitles[5] = { "high . clear", "wide . swell",
                                        "deep . mist", "damp . fog",
                                        "heat . stone" };
    /* accent tints (ADR-0015: at least one channel near full) */
    uint8_t expect_accent[5][3] = {
        { 255, 238, 205 },   /* Alps — warm cream   */
        { 140, 225, 255 },   /* Open Sea — aqua     */
        { 150, 195, 255 },   /* Fjords — cold blue  */
        { 180, 255, 195 },   /* Moss — green        */
        { 255, 215, 150 },   /* Desert — ochre      */
    };
    /* macro presets: space, atmos, motion, age, echo, blur */
    uint8_t expect_preset[5][6] = {
        { 55, 28, 35, 12, 42,  8 },   /* Alps       */
        { 58, 35, 65, 20, 25, 28 },   /* Open Sea   */
        { 65, 40, 25, 25, 55, 12 },   /* Fjords     */
        { 45, 45, 18, 35, 30, 40 },   /* Moss       */
        { 45, 30, 15, 40, 50, 20 },   /* Desert     */
    };

    for (int i = 0; i < 5; ++i) {
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
              w->echo_pct   == expect_preset[i][4] &&
              w->blur_pct   == expect_preset[i][5],
              "preset[%d] = (%d,%d,%d,%d,%d,%d), want (%d,%d,%d,%d,%d,%d)", i,
              w->space_pct, w->atmos_pct, w->motion_pct, w->age_pct,
              w->echo_pct, w->blur_pct,
              expect_preset[i][0], expect_preset[i][1], expect_preset[i][2],
              expect_preset[i][3], expect_preset[i][4], expect_preset[i][5]);
        /* every accent must keep at least one channel near full so whites
         * stay bright in the grey→RGB565 cast (ADR-0015) */
        int max_ch = w->accent_r;
        if (w->accent_g > max_ch) max_ch = w->accent_g;
        if (w->accent_b > max_ch) max_ch = w->accent_b;
        CHECK(max_ch >= 240, "accent[%d] too dark (max=%d)", i, max_ch);

        /* musical identity in valid ranges */
        CHECK(w->key_midi <= 127, "key[%d] out of MIDI range", i);
        CHECK(w->mode < 6, "mode[%d] out of range (%d)", i, w->mode);
        CHECK(w->vibe < 4, "vibe[%d] out of range (%d)", i, w->vibe);
    }

    /* per-world musical identity (key, mode, vibe): Alps G-lydian-bright,
     * Open Sea D-mixolydian-floating, Fjords F#-dorian-deep, Moss C-aeolian-
     * warm, Desert F-phrygian-deep. */
    uint8_t expect_key [5] = { 55, 62, 54, 60, 53 };
    uint8_t expect_mode[5] = {  3,  4,  1,  5,  2 };
    uint8_t expect_vibe[5] = {  1,  3,  2,  0,  2 };
    for (int i = 0; i < 5; ++i) {
        const world_t *w = worlds_get(i);
        CHECK(w->key_midi == expect_key[i],  "key[%d] = %d, want %d",  i, w->key_midi, expect_key[i]);
        CHECK(w->mode     == expect_mode[i], "mode[%d] = %d, want %d", i, w->mode,     expect_mode[i]);
        CHECK(w->vibe     == expect_vibe[i], "vibe[%d] = %d, want %d", i, w->vibe,     expect_vibe[i]);
    }
    /* all five worlds should differ harmonically — no two identical triples */
    for (int a = 0; a < 5; ++a)
        for (int b = a + 1; b < 5; ++b) {
            const world_t *wa = worlds_get(a), *wb = worlds_get(b);
            CHECK(!(wa->key_midi == wb->key_midi && wa->mode == wb->mode && wa->vibe == wb->vibe),
                  "worlds %d and %d are harmonically identical", a, b);
        }

    /* out-of-range index must clamp, not crash */
    CHECK(worlds_get(-1) == worlds_get(0), "clamp -1 → 0");
    CHECK(worlds_get(99) == worlds_get(4), "clamp 99 → 4");

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
