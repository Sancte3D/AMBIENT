/*
 * Host-side tests for Step-12b reverb_presets.
 *
 * Verifies:
 * 1. The mapped Freeverb outputs fall in their declared 0..1 ranges for
 *    every combination of mode/vibe across the space/mood macro grid.
 * 2. Mode preset structure: lydian (longest t60) produces the LARGEST size;
 *    phrygian (shortest t60) produces the SMALLEST.
 * 3. Vibe bias structure: "bright" produces LESS damping than "deep" for the
 *    same mode/space/mood (matches the webapp's intent).
 * 4. Space macro behaves piecewise: 0..0.5 ducks, 0.5..1 expands; wet
 *    amplitude rises monotonically with space; the value at space=0.5 ≈
 *    midpoint, matching webapp linlin behaviour.
 *
 * Build via run_tests.sh, or directly:
 *   cc -std=c11 -I../include test_reverb_presets.c ../src/reverb_presets.c \
 *      -lm -o /tmp/rp_test
 */

#include "reverb_presets.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int in01(float x) { return x >= 0.0f && x <= 1.0f; }

static void test_outputs_in_range(void) {
    /* Sweep mode × vibe × (space, mood) grid; every output must be in 0..1. */
    for (int m = 0; m < RP_MODE_COUNT; ++m)
    for (int v = 0; v < RP_VIBE_COUNT; ++v)
    for (int si = 0; si <= 10; ++si)
    for (int mi = 0; mi <= 10; ++mi) {
        float space = si / 10.0f, mood = mi / 10.0f;
        reverb_settings_t s = reverb_presets_compute(m, v, space, mood);
        CHECK(in01(s.size),    "size out of range m=%d v=%d space=%.1f mood=%.1f -> %g", m,v,space,mood,s.size);
        CHECK(in01(s.damp),    "damp out of range m=%d v=%d -> %g", m,v, s.damp);
        CHECK(in01(s.drive),   "drive out of range m=%d v=%d -> %g", m,v, s.drive);
        CHECK(in01(s.wet_amp), "wet out of range m=%d v=%d space=%.1f -> %g", m,v,space,s.wet_amp);
    }
}

static void test_clamps_on_extreme_inputs(void) {
    /* Out-of-range inputs must clamp, not blow up. */
    reverb_settings_t a = reverb_presets_compute(-5, -5, -1.0f, -1.0f);
    reverb_settings_t b = reverb_presets_compute(99, 99,  2.0f,  2.0f);
    CHECK(in01(a.size) && in01(a.damp) && in01(a.drive) && in01(a.wet_amp), "negative clamp failed");
    CHECK(in01(b.size) && in01(b.damp) && in01(b.drive) && in01(b.wet_amp), "overflow clamp failed");
}

static void test_mode_size_ordering(void) {
    /* At space=0.5/mood=0.5/vibe=warm: lydian (longest tail) should have the
     * largest size; phrygian (shortest) the smallest. */
    float sz[RP_MODE_COUNT];
    for (int m = 0; m < RP_MODE_COUNT; ++m)
        sz[m] = reverb_presets_compute(m, 0, 0.5f, 0.5f).size;

    /* indices: ionian=0 dorian=1 phrygian=2 lydian=3 mixolyd=4 aeolian=5 */
    CHECK(sz[3] > sz[0],  "lydian size %.3f not > ionian %.3f", sz[3], sz[0]);
    CHECK(sz[3] > sz[2],  "lydian size %.3f not > phrygian %.3f", sz[3], sz[2]);
    CHECK(sz[2] < sz[0],  "phrygian size %.3f not < ionian %.3f", sz[2], sz[0]);
    CHECK(sz[2] < sz[5],  "phrygian size %.3f not < aeolian %.3f", sz[2], sz[5]);

    printf("  mode sizes @ warm,space=mood=0.5:\n");
    char b[80];
    for (int m = 0; m < RP_MODE_COUNT; ++m)
        printf("    %s\n", reverb_presets_describe(m, 0, b, sizeof b));
}

static void test_vibe_damp_ordering(void) {
    /* For the same mode, vibe "deep" (high damp bias 1.4) must produce more
     * damping than vibe "bright" (damp bias 0.6). */
    for (int m = 0; m < RP_MODE_COUNT; ++m) {
        float bright = reverb_presets_compute(m, 1, 0.5f, 0.5f).damp;
        float deep   = reverb_presets_compute(m, 2, 0.5f, 0.5f).damp;
        CHECK(deep > bright,
              "vibe damp ordering wrong for mode %d: deep=%.3f vs bright=%.3f", m, deep, bright);
    }
}

static void test_space_macro_shape(void) {
    /* wet_amp must be a monotonic increase with space, 0.40..0.70. */
    float prev = -1.0f;
    for (int i = 0; i <= 10; ++i) {
        float space = i / 10.0f;
        float wet = reverb_presets_compute(0, 0, space, 0.5f).wet_amp;
        CHECK(wet + 1e-5f >= prev, "wet not monotonic at space=%.1f (%.3f < %.3f)", space, wet, prev);
        prev = wet;
    }
    /* endpoints */
    CHECK(fabsf(reverb_presets_compute(0,0,0.0f,0.5f).wet_amp - 0.40f) < 1e-3f, "wet@space=0 != 0.40");
    CHECK(fabsf(reverb_presets_compute(0,0,1.0f,0.5f).wet_amp - 0.70f) < 1e-3f, "wet@space=1 != 0.70");

    /* size piecewise: at space=0 it must be SMALLER than at space=0.5 (duck),
     * at space=1 LARGER than at space=0.5 (expand). */
    float s_lo = reverb_presets_compute(0,0,0.0f,0.5f).size;
    float s_md = reverb_presets_compute(0,0,0.5f,0.5f).size;
    float s_hi = reverb_presets_compute(0,0,1.0f,0.5f).size;
    CHECK(s_lo < s_md, "space=0 size %.3f not < space=0.5 size %.3f", s_lo, s_md);
    CHECK(s_hi > s_md, "space=1 size %.3f not > space=0.5 size %.3f", s_hi, s_md);
    printf("  space curve: s=0 -> size %.3f wet %.3f | s=0.5 -> %.3f / %.3f | s=1 -> %.3f / %.3f\n",
           (double)s_lo, (double)reverb_presets_compute(0,0,0.0f,0.5f).wet_amp,
           (double)s_md, (double)reverb_presets_compute(0,0,0.5f,0.5f).wet_amp,
           (double)s_hi, (double)reverb_presets_compute(0,0,1.0f,0.5f).wet_amp);
}

static void test_deterministic(void) {
    /* Same inputs → same outputs (no global state, no rng). */
    reverb_settings_t a = reverb_presets_compute(3, 2, 0.37f, 0.81f);
    reverb_settings_t b = reverb_presets_compute(3, 2, 0.37f, 0.81f);
    CHECK(a.size == b.size && a.damp == b.damp && a.drive == b.drive && a.wet_amp == b.wet_amp,
          "non-deterministic output");
}

int main(void) {
    printf("== reverb_presets (step12b #1) ==\n");
    test_clamps_on_extreme_inputs();
    test_outputs_in_range();
    test_mode_size_ordering();
    test_vibe_damp_ordering();
    test_space_macro_shape();
    test_deterministic();

    printf("\n%d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n");
    return 0;
}
