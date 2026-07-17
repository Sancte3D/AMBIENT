/*
 * Host-side tests for the Step-12a harmonic brain.
 *
 * Reference values hand-derived from the webapp's chordAtDegree /
 * voiceCentered for key 60. Pure integer theory — fully deterministic.
 *
 * Build via run_tests.sh, or:
 *   cc -std=c11 -I../include test_brain.c ../src/brain.c -o /tmp/brain_test
 */

#include "brain.h"

#include <stdio.h>
#include <stdlib.h>

static int g_checks = 0;
static int g_fails  = 0;

#define CHECK(cond, ...)                                                   \
    do {                                                                   \
        ++g_checks;                                                        \
        if (!(cond)) {                                                     \
            ++g_fails;                                                     \
            fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__);           \
            fprintf(stderr, __VA_ARGS__);                                  \
            fprintf(stderr, "\n");                                         \
        }                                                                  \
    } while (0)

static int chord_eq(const int *got, int n, const int *exp, int en) {
    if (n != en) return 0;
    for (int i = 0; i < n; ++i) if (got[i] != exp[i]) return 0;
    return 1;
}

static void print_chord(const char *tag, const int *c, int n) {
    printf("  %s = [", tag);
    for (int i = 0; i < n; ++i) printf("%d%s", c[i], i+1<n ? " " : "");
    printf("]\n");
}

static void test_default_ionian_add9(void) {
    brain_init();                                  /* key 60, ionian, warm/add9 */
    int c[BRAIN_MAX_CHORD], n;

    n = brain_chord(1, c, BRAIN_MAX_CHORD);
    print_chord("ionian I add9", c, n);
    int exp1[] = { 60, 64, 67, 74 };
    CHECK(chord_eq(c, n, exp1, 4), "ionian degree 1 add9 wrong");
    CHECK(brain_cell_root(0) == 60, "cell 0 root != 60: %d", brain_cell_root(0));

    n = brain_chord(5, c, BRAIN_MAX_CHORD);
    print_chord("ionian V add9", c, n);
    int exp5[] = { 55, 59, 62, 69 };               /* voiced down an octave */
    CHECK(chord_eq(c, n, exp5, 4), "ionian degree 5 add9 wrong");
    /* r19.26: cell roots are an ascending pentatonic above the key, not the
     * degree-5 chord root. C major → cell 4 = 60 + 9 = A4 (69). */
    CHECK(brain_cell_root(4) == 69, "cell 4 root != 69: %d", brain_cell_root(4));
}

static void test_mode_changes_thirds(void) {
    brain_init();
    brain_set_mode(5);                             /* aeolian — minor third */
    int c[BRAIN_MAX_CHORD];
    int n = brain_chord(1, c, BRAIN_MAX_CHORD);
    print_chord("aeolian I add9", c, n);
    int exp[] = { 60, 63, 67, 74 };                /* 63 = Eb (minor 3rd) */
    CHECK(chord_eq(c, n, exp, 4), "aeolian degree 1 wrong (mode not applied)");
    /* ionian had 64 (major 3rd) here — confirm they differ */
    brain_set_mode(0);
    int ci[BRAIN_MAX_CHORD];
    brain_chord(1, ci, BRAIN_MAX_CHORD);
    CHECK(ci[1] == 64 && c[1] == 63, "mode third not distinguished");
}

static void test_vibe_changes_family(void) {
    brain_init();
    int c[BRAIN_MAX_CHORD], n;

    brain_set_vibe(2);                             /* deep → min11 (6 notes) */
    n = brain_chord(1, c, BRAIN_MAX_CHORD);
    print_chord("ionian I min11", c, n);
    int exp[] = { 60, 64, 67, 71, 74, 77 };
    CHECK(chord_eq(c, n, exp, 6), "min11 family wrong");

    brain_set_vibe(3);                             /* floating → sus2 (3 notes) */
    n = brain_chord(1, c, BRAIN_MAX_CHORD);
    print_chord("ionian I sus2", c, n);
    int exps[] = { 60, 62, 67 };
    CHECK(chord_eq(c, n, exps, 3), "sus2 family wrong");
}

static void test_key_moves_cell_root(void) {
    /* r19.26: cell 0 is the key itself, so a key change moves the root by
     * exactly that interval — an octave key change moves it a full octave
     * (no voiceCentered fold on the cell path any more). */
    brain_init();
    int root60 = brain_cell_root(0);
    CHECK(root60 == 60, "cell 0 must be the key (60): %d", root60);
    brain_set_key(48);                             /* C3 — octave below */
    int root48 = brain_cell_root(0);
    CHECK(root48 == root60 - 12, "octave key change should move root an octave: %d vs %d",
          root48, root60);
    brain_set_key(62);                             /* D — different pitch class */
    int root62 = brain_cell_root(0);
    CHECK(root62 == 62, "cell 0 should follow the key: %d", root62);
    printf("  cell0 root: C=%d  C3=%d  D=%d\n", root60, root48, root62);
}

static void test_all_degrees_centered_and_sane(void) {
    brain_init();
    for (int mode = 0; mode < BRAIN_MODE_COUNT; ++mode) {
        brain_set_mode(mode);
        for (int vibe = 0; vibe < BRAIN_VIBE_COUNT; ++vibe) {
            brain_set_vibe(vibe);
            for (int deg = 1; deg <= 7; ++deg) {
                int c[BRAIN_MAX_CHORD];
                int n = brain_chord(deg, c, BRAIN_MAX_CHORD);
                CHECK(n >= 3 && n <= BRAIN_MAX_CHORD, "bad chord size %d", n);
                int sum = 0, lo = c[0], hi = c[0];
                for (int i = 0; i < n; ++i) {
                    sum += c[i];
                    if (c[i] < lo) lo = c[i];
                    if (c[i] > hi) hi = c[i];
                    CHECK(c[i] >= 24 && c[i] <= 108, "note out of range: %d", c[i]);
                }
                float mean = (float)sum / n;
                CHECK(mean >= 58.0f && mean <= 70.0f,
                      "voicing not centred (mean %.1f) mode=%d vibe=%d deg=%d",
                      mean, mode, vibe, deg);
                (void)lo;   /* r19.26: cell roots no longer track chord min */
            }
        }
    }
}

/* r19.32: chord colours build the intended interval shape, all in the scale. */
static int pc_in_scale_cmaj(int m) {
    int pc = ((m % 12) + 12) % 12;
    /* C major pitch classes */
    return pc==0||pc==2||pc==4||pc==5||pc==7||pc==9||pc==11;
}
static int has_pc(const int *c, int n, int pc) {
    for (int i = 0; i < n; ++i) if ((((c[i]%12)+12)%12) == pc) return 1;
    return 0;
}
static void test_color_chords(void) {
    brain_init(); brain_set_key(60); brain_set_mode(0);   /* C major */
    int c[BRAIN_MAX_CHORD], n;

    n = brain_color_chord(1, 0, c, BRAIN_MAX_CHORD);       /* PURE 1-3-5 */
    CHECK(n == 3, "PURE is a triad (%d)", n);
    CHECK(has_pc(c,n,0)&&has_pc(c,n,4)&&has_pc(c,n,7), "PURE = C E G");

    n = brain_color_chord(1, 1, c, BRAIN_MAX_CHORD);       /* OPEN 1-5-9 */
    CHECK(has_pc(c,n,0)&&has_pc(c,n,7)&&has_pc(c,n,2), "OPEN = C G D");
    CHECK(!has_pc(c,n,4), "OPEN has no third");

    n = brain_color_chord(1, 3, c, BRAIN_MAX_CHORD);       /* DEEP 1-3-5-7 */
    CHECK(n == 4, "DEEP has four notes (%d)", n);
    CHECK(has_pc(c,n,11), "DEEP includes the major 7th (B)");

    /* every colour, every degree: only scale tones (no accidental chromatics) */
    for (int col = 0; col < BRAIN_COLOR_COUNT; ++col)
        for (int deg = 1; deg <= 7; ++deg) {
            n = brain_color_chord(deg, col, c, BRAIN_MAX_CHORD);
            for (int i = 0; i < n; ++i)
                CHECK(pc_in_scale_cmaj(c[i]), "color %d deg %d note %d in scale",
                      col, deg, c[i]);
        }
}

int main(void) {
    printf("== harmonic brain ==\n");
    test_default_ionian_add9();
    test_mode_changes_thirds();
    test_vibe_changes_family();
    test_key_moves_cell_root();
    test_color_chords();
    test_all_degrees_centered_and_sane();

    printf("\n%d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n");
    return 0;
}
