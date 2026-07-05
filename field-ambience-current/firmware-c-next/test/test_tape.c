/*
 * test_tape.c — hiss + warm saturation (ADR-0017 Phase 3).
 *
 * Invariants:
 *  1. hiss at amp=0 adds zero (silent path); at default amp produces audible
 *     stereo signal that's decorrelated between L and R.
 *  2. saturation at small signals is approximately linear with the implicit
 *     0.78 makeup attenuation (within tanh's tiny-x approximation).
 *  3. saturation bounds output strictly to ±0.78 regardless of input
 *     magnitude (the tanh ceiling × 0.78).
 *  4. Drive setter clamps to a sane range.
 */

#include "tape.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

static int g_checks = 0, g_fails = 0;
#define CHECK(c, ...) do { ++g_checks; if (!(c)) { ++g_fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

#define N 512

int main(void) {
    tape_init();

    /* 1a: hiss amp 0 must be silent */
    float L[N] = {0}, R[N] = {0};
    tape_set_hiss_amount(0.0f);
    tape_hiss_render_add(L, R, N);
    int nonzero = 0;
    for (int i = 0; i < N; ++i) nonzero += (L[i] != 0.0f) | (R[i] != 0.0f);
    CHECK(nonzero == 0, "hiss amp=0 produced %d non-zero samples", nonzero);

    /* 1b: default hiss amp produces signal and L/R are decorrelated.
     * r18.92: hiss DUCKS with the program follower — feed a full program
     * level first so this measures the un-ducked reference level. */
    tape_init();   /* reset seeds */
    tape_set_program_level(1.0f);
    memset(L, 0, sizeof L); memset(R, 0, sizeof R);
    tape_hiss_render_add(L, R, N);
    int identical = 0;
    for (int i = 0; i < N; ++i) if (L[i] == R[i]) ++identical;
    CHECK(identical < N / 4, "L == R too often (%d/%d) — not decorrelated", identical, N);
    double sumsq = 0;
    for (int i = 0; i < N; ++i) sumsq += (double)L[i] * L[i];
    float rms = (float)sqrt(sumsq / N);
    /* r18.97 default amp 0.0012 → ~rms 0.0007 (uniform white rms = amp/√3;
     * measured 0.00072). The old 0.005 default was the audible noise
     * carpet — ducking opened it fully whenever music played. */
    CHECK(rms > 0.0003f && rms < 0.0025f,
          "hiss rms out of expected band: %g", (double)rms);

    /* 1c: r18.92 ducking — in silence the same hiss must sit ≈ −10.5 dB
     * below the program-present level (floor 0.30), never at zero. */
    tape_init();
    memset(L, 0, sizeof L); memset(R, 0, sizeof R);
    tape_hiss_render_add(L, R, N);          /* duck_env = 0 → floor gain */
    double sumsq_q = 0;
    for (int i = 0; i < N; ++i) sumsq_q += (double)L[i] * L[i];
    float rms_q = (float)sqrt(sumsq_q / N);
    CHECK(rms_q > 0.0001f && rms_q < rms * 0.45f,
          "silence hiss not ducked to the floor: %g vs %g", (double)rms_q,
          (double)rms);

    /* 2: small-signal linearity of saturation: input 0.01 → output ≈ 0.01*1.10*0.78 = 0.00858 */
    for (int i = 0; i < N; ++i) { L[i] = 0.01f; R[i] = 0.01f; }
    tape_saturation_process(L, R, N);
    for (int i = 0; i < N; ++i) {
        CHECK(fabsf(L[i] - 0.00858f) < 1.0e-4f,
              "small signal sat off: got %g want 0.00858", (double)L[i]);
        break;  /* one sample is enough — all identical */
    }

    /* 3: hard ceiling at ±0.78 regardless of input. Use a brute-force
     * stress input of ±10.0, then check abs(output) < 0.78001. */
    for (int i = 0; i < N; ++i) {
        L[i] = (i & 1) ? -10.0f : +10.0f;
        R[i] = (i & 1) ? +10.0f : -10.0f;
    }
    tape_saturation_process(L, R, N);
    float maxL = 0, maxR = 0;
    for (int i = 0; i < N; ++i) {
        if (fabsf(L[i]) > maxL) maxL = fabsf(L[i]);
        if (fabsf(R[i]) > maxR) maxR = fabsf(R[i]);
    }
    CHECK(maxL <= 0.78001f, "L ceiling broken: %g", (double)maxL);
    CHECK(maxR <= 0.78001f, "R ceiling broken: %g", (double)maxR);
    /* At drive=1.1 and input=±10, tanh(11)*0.78 ≈ 0.78. So output should be
     * very close to ±0.78. */
    CHECK(maxL > 0.77f, "L saturation didn't reach the ceiling: %g", (double)maxL);

    /* 4: drive clamps */
    tape_set_saturation_drive(-1.0f);   /* clamp to 0.1 */
    /* (no direct read; assert via behavior — at drive 0.1, signal 0.5
     * should give tanh(0.05) * 0.78 ≈ 0.0389) */
    for (int i = 0; i < N; ++i) { L[i] = 0.5f; R[i] = 0.5f; }
    tape_saturation_process(L, R, N);
    CHECK(L[0] > 0.03f && L[0] < 0.05f,
          "low-drive clamp behaviour off: %g", (double)L[0]);

    tape_set_saturation_drive(1000.0f);  /* clamp to 4.0 */
    for (int i = 0; i < N; ++i) { L[i] = 1.0f; R[i] = 1.0f; }
    tape_saturation_process(L, R, N);
    CHECK(L[0] > 0.77f && L[0] <= 0.78001f,
          "high-drive clamp behaviour off: %g", (double)L[0]);

    /* restore defaults so nothing leaks if linked alongside other suites */
    tape_init();

    printf("%d checks, %d failures\n", g_checks, g_fails);
    printf("RESULT: %s\n", g_fails ? "FAIL" : "PASS");
    return g_fails ? 1 : 0;
}
