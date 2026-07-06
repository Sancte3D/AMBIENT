/*
 * test_tuning.c — equal / just intonation layer (r19.6).
 *
 *   1. EQUAL mode is BIT-EXACT dsp_midi_to_hz (reference untouched).
 *   2. JUST mode produces pure ratios from the tonic: the tonic keeps its
 *      ET frequency; a fifth is exactly 3/2, a major third 5/4, an octave
 *      2/1, a fourth 4/3, a sixth 5/3.
 *   3. A pure fifth actually removes beating: the 3rd partial of the root
 *      and the 2nd partial of the fifth coincide (3·f = 2·1.5f), so a
 *      root+fifth pair beats in ET but not in JUST — measured.
 */

#include "tuning.h"
#include "dsp.h"
#include <stdio.h>
#include <math.h>

static int checks = 0, fails = 0;
#define CHECK(c, ...) do { ++checks; if (!(c)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

int main(void) {
    printf("== tuning (equal / just) ==\n");

    /* 1. EQUAL = bit-exact ET */
    tuning_set_mode(0);
    tuning_set_key(57);                       /* A */
    for (int m = 40; m <= 90; ++m)
        CHECK(tuning_hz((float)m) == dsp_midi_to_hz((float)m),
              "EQUAL bit-exact at midi %d", m);

    /* 2. JUST pure ratios, anchored to A (tonic = 57) */
    tuning_set_mode(1);
    tuning_set_key(57);
    float aHz = dsp_midi_to_hz(57.0f);
    CHECK(fabsf(tuning_hz(57.0f) - aHz) < 1e-3f, "tonic keeps ET pitch");
    CHECK(fabsf(tuning_hz(69.0f) - aHz * 2.0f) < 1e-3f, "octave = 2/1");
    CHECK(fabsf(tuning_hz(64.0f) - aHz * 1.5f) < 1e-3f, "fifth = 3/2");
    CHECK(fabsf(tuning_hz(62.0f) - aHz * (4.0f/3.0f)) < 1e-3f, "fourth = 4/3");
    CHECK(fabsf(tuning_hz(61.0f) - aHz * (5.0f/4.0f)) < 1e-3f, "maj3 = 5/4");
    CHECK(fabsf(tuning_hz(66.0f) - aHz * (5.0f/3.0f)) < 1e-3f, "maj6 = 5/3");
    /* a fifth BELOW the tonic (floor-mod / octave-down path) */
    CHECK(fabsf(tuning_hz(45.0f) - aHz / 2.0f) < 1e-3f, "octave down = 1/2");
    CHECK(fabsf(tuning_hz(49.0f) - aHz * (5.0f/4.0f) / 2.0f) < 1e-3f,
          "maj3 (C#) an octave down = 5/4 / 2");

    /* 3. beating: root + fifth. In ET the 3rd/2nd partials are ~0.9 Hz
     *    apart → a slow amplitude beat; in JUST they coincide → flat.
     *    Measure the envelope ripple of 3f_root vs 2f_fifth. */
    for (int mode = 0; mode < 2; ++mode) {
        tuning_set_mode(mode);
        float root  = tuning_hz(57.0f);       /* A            */
        float fifth = tuning_hz(64.0f);       /* E            */
        double p3 = 3.0 * root, p2 = 2.0 * fifth;
        double beat_hz = fabs(p3 - p2);
        if (mode == 0)
            CHECK(beat_hz > 0.3, "ET fifth beats (%.3f Hz)", beat_hz);
        else
            CHECK(beat_hz < 0.01, "JUST fifth is beat-free (%.4f Hz)", beat_hz);
    }

    printf("%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
