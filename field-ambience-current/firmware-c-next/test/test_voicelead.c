/*
 * test_voicelead.c — r19.29 minimal-movement voicing.
 *
 * The canonical case (chat's example): Cmaj7 → Am7 keeps C, E, G on the same
 * pitch and moves only B two semitones down to A. Plus register sanity and
 * the no-previous spread.
 */
#include <stdio.h>
#include "voicelead.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

static int total_move(const int *a, const int *b, int n) {
    int s = 0; for (int i = 0; i < n; ++i) { int d = a[i]-b[i]; s += d<0?-d:d; } return s;
}

int main(void) {
    printf("== voicelead (r19.29) ==\n");

    /* 1. no previous chord → ascending spread near the anchor */
    int cmaj7_pc[4] = { 0, 4, 7, 11 };      /* C E G B */
    int v0[4];
    int n = voicelead(0, 0, cmaj7_pc, 4, 64, v0);
    CHECK(n == 4, "returns 4 voices");
    CHECK(v0[0] < v0[1] && v0[1] < v0[2] && v0[2] < v0[3], "ascending initial voicing");
    for (int i = 0; i < 4; ++i) CHECK(v0[i] >= 48 && v0[i] <= 84, "in register: %d", v0[i]);

    /* 2. Cmaj7 (as an explicit voicing) → Am7: keep C E G, move B→A only */
    int cmaj7[4] = { 60, 64, 67, 71 };      /* C4 E4 G4 B4 */
    int am7_pc[4] = { 9, 0, 4, 7 };         /* A C E G */
    int v1[4];
    voicelead(cmaj7, 4, am7_pc, 4, 64, v1);
    /* the three common tones C(0) E(4) G(7) must stay on their exact pitches */
    int haveC = 0, haveE = 0, haveG = 0, haveA = 0;
    for (int i = 0; i < 4; ++i) {
        if (v1[i] == 60) haveC = 1;
        if (v1[i] == 64) haveE = 1;
        if (v1[i] == 67) haveG = 1;
        if (v1[i] == 69) haveA = 1;         /* B4(71) → A4(69), the min move  */
    }
    CHECK(haveC && haveE && haveG, "common tones C/E/G kept on the same pitch");
    CHECK(haveA, "B moved to the nearest A (69)");
    CHECK(total_move(cmaj7, v1, 4) == 2, "only 2 semitones of total movement (%d)",
          total_move(cmaj7, v1, 4));

    /* 3. all pitch classes are represented in the output */
    for (int t = 0; t < 4; ++t) {
        int found = 0;
        for (int i = 0; i < 4; ++i) if (((v1[i] % 12) + 12) % 12 == am7_pc[t]) found = 1;
        CHECK(found, "target pc %d present in the voicing", am7_pc[t]);
    }

    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
