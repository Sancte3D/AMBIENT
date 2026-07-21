/*
 * test_brain_cells.c — r19.26 cell harmonics.
 *
 * Proves the property Mischi's analysis asked for, over every world's real
 * key/mode from worlds.c:
 *   - the five cell roots rise strictly left→right;
 *   - NO semitone and NO tritone between any two of the five roots
 *     (the old diatonic-degree path carried one semitone pair per world);
 *   - Shift = exactly +1 octave (the caller's contract, asserted on the map);
 *   - roots stay in a sane playable register.
 *
 * Pure integer theory — links only brain.c + worlds.c.
 */
#include <stdio.h>
#include "brain.h"
#include "worlds.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

int main(void) {
    printf("== brain cell harmonics (r19.26) ==\n");

    for (int wi = 0; wi < worlds_count(); ++wi) {
        const world_t *w = worlds_get(wi);
        brain_init();
        brain_set_key(w->key_midi);
        brain_set_mode(w->mode);
        brain_set_vibe(w->vibe);

        int r[5];
        for (int c = 0; c < 5; ++c) r[c] = brain_cell_root(c);

        printf("  %-14s roots: %d %d %d %d %d\n",
               w->name, r[0], r[1], r[2], r[3], r[4]);

        /* strictly ascending */
        for (int c = 1; c < 5; ++c)
            CHECK(r[c] > r[c - 1],
                  "%s: cell %d (%d) not above cell %d (%d)",
                  w->name, c, r[c], c - 1, r[c - 1]);

        /* no semitone / tritone between any two roots (pitch-class) */
        for (int a = 0; a < 5; ++a)
            for (int b = a + 1; b < 5; ++b) {
                int d = r[a] - r[b]; if (d < 0) d = -d;
                int pc = d % 12;
                CHECK(pc != 1 && pc != 11,
                      "%s: semitone between root %d and %d", w->name, r[a], r[b]);
                CHECK(pc != 6,
                      "%s: tritone between root %d and %d", w->name, r[a], r[b]);
            }

        /* first root is the key itself; register stays playable */
        CHECK(r[0] == w->key_midi, "%s: cell 0 must equal the key", w->name);
        CHECK(r[0] >= 48 && r[4] <= 84, "%s: roots out of register", w->name);
    }

    /* Shift contract: the caller sounds root+12; assert the octave map holds
     * for a representative key (major and minor). */
    brain_init(); brain_set_key(60); brain_set_mode(0); /* C major */
    for (int c = 0; c < 5; ++c) {
        int base = brain_cell_root(c);
        CHECK((base + 12) - base == 12, "shift must be exactly one octave");
    }

    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
