/*
 * worlds.c — the five curated landscape worlds (r19.44, ADR-0017 Phase 1).
 *
 * r19.44 rename+retune: the four nocturnal-city worlds (Tokyo City / Crystal
 * Coast / Midnight Drive / After Hours) became five globally-recognisable
 * LANDSCAPES. Only the levers the engine already has are used — key, mode,
 * vibe, the seven macros, chord colour, bass mode, accent. No new synthesis:
 * the per-world "instrument DNA" (alphorn/Hardanger/lyra/langspil/guembri)
 * from the location brief is a separate roadmap item, not a preset change.
 *
 * Order is deliberate — each world sits on the legacy per-world timbre slot
 * whose character already matches it (body.c MATERIALS, PADsynth seed, and the
 * master-fx per-world voicing are indexed by this order):
 *   0 ALPS        ← warm-wood body   (was Tokyo)   — alpine wood/horn, warm
 *   1 OPEN SEA    ← glass/bright body (was Coast)  — wide, blue, open
 *   2 FJORDS      ← dark-metal body   (was Drive)  — deep, bowed, vertical
 *   3 MOSS FIELDS ← felt/soft body    (was Hours)  — absorbed, muted
 *   4 DESERT      ← new low body      (r19.44)     — monumental, dry, warm
 *
 * Naming + subtitle rules:
 *   - name <= 13 chars (Helvetica value font fit at 320 px width)
 *   - subtitle is a flavour line, shown dimmed under the name
 *
 * Accent colour rule: at least one channel near 255 so whites stay bright in
 * the grey→RGB565 cast. See ADR-0015 Step 1.
 *
 * NB on modal identity: the generative core reads only tonality (major vs
 * minor pentatonic — mode_is_minor), so lydian/mixolydian/phrygian collapse to
 * major/minor in the auto-melody. The per-world distinction therefore comes
 * from key + macros + chord colour + bass + accent, NOT from true modal
 * harmony. Honest limitation; documented so nobody expects "real dorian".
 */

#include "worlds.h"

static const world_t WORLDS[WORLD_COUNT] = {
    {
        .name = "Alps",
        .subtitle = "high . clear",
        .accent_r = 255, .accent_g = 238, .accent_b = 205,   /* warm sunlit cream */
        .space_pct = 55, .atmos_pct = 28,
        .motion_pct = 35, .age_pct = 12, .echo_pct = 42, .blur_pct = 8,
        .shimmer_pct = 20,   /* sunlit halo over clear air */
        /* G lydian, clear+open — long natural valley echo, open fifths/6/add9 */
        .key_midi = 55, .mode = 3, .vibe = 1,
        .chord_color = 0, .bass_mode = 2,   /* Pure open fifths, drone fifth */
    },
    {
        .name = "Open Sea",
        .subtitle = "wide . swell",
        .accent_r = 140, .accent_g = 225, .accent_b = 255,   /* aqua blue */
        .space_pct = 58, .atmos_pct = 35,
        .motion_pct = 65, .age_pct = 20, .echo_pct = 25, .blur_pct = 28,
        .shimmer_pct = 20,   /* light off the water */
        /* D mixolydian, blue+suspended — slow swell, sixths/ninths, warm not cheery */
        .key_midi = 62, .mode = 4, .vibe = 3,
        .chord_color = 1, .bass_mode = 2,   /* Open 1-5-9, tidal fifth */
    },
    {
        .name = "Fjords",
        .subtitle = "deep . mist",
        .accent_r = 150, .accent_g = 195, .accent_b = 255,   /* cold blue (not hostile) */
        .space_pct = 65, .atmos_pct = 40,
        .motion_pct = 25, .age_pct = 25, .echo_pct = 55, .blur_pct = 12,
        .shimmer_pct = 6,    /* grounded — no bright halo, long vertical reflection */
        /* F# dorian, deep+vertical — bowed tension, long reflection paths */
        .key_midi = 54, .mode = 1, .vibe = 2,
        .chord_color = 3, .bass_mode = 1,   /* Deep 7th, grounded root */
    },
    {
        .name = "Moss Fields",
        .subtitle = "damp . fog",
        .accent_r = 180, .accent_g = 255, .accent_b = 195,   /* moss green */
        .space_pct = 45, .atmos_pct = 45,
        .motion_pct = 18, .age_pct = 35, .echo_pct = 30, .blur_pct = 40,
        .shimmer_pct = 8,    /* muted upper spectrum — soft absorption */
        /* C aeolian, muted+horizontal — warm add9, damp organ/choir body */
        .key_midi = 60, .mode = 5, .vibe = 0,
        .chord_color = 2, .bass_mode = 1,   /* Warm 6/9, grounded root */
    },
    {
        .name = "Desert",
        .subtitle = "heat . stone",
        .accent_r = 255, .accent_g = 215, .accent_b = 150,   /* ochre / sand */
        .space_pct = 45, .atmos_pct = 30,
        .motion_pct = 15, .age_pct = 40, .echo_pct = 50, .blur_pct = 20,
        .shimmer_pct = 5,    /* restrained — heavy stillness, dry foreground */
        /* F phrygian, monumental+dry — slow modal tension, warm low-mid body,
         * long distant reflection (echo high, but space dry-ish) */
        .key_midi = 53, .mode = 2, .vibe = 2,
        .chord_color = 2, .bass_mode = 1,   /* Warm low body, strong root */
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
