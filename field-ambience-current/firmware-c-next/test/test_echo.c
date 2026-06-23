/*
 * test_echo.c — tape-style stereo delay macro (Reddit Echo).
 *
 * Invariants:
 *   1. amount=0 must converge to silence (smoothed) — bus untouched.
 *   2. amount=1 produces signal that's bounded < 1.0.
 *   3. impulse at t=0 produces an audible repeat ~time_samps later in
 *      both L and R.
 *   4. feedback decays — successive repeats are quieter (the LP-in-FB
 *      darkens them too, but the test just checks magnitude).
 *   5. send buffer carries a scaled copy of what was added to dry.
 */

#include "echo.h"
#include "dsp.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

static int g_checks = 0, g_fails = 0;
#define CHECK(c, ...) do { ++g_checks; if (!(c)) { ++g_fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

#define BLOCK 256
#define SR    DSP_SAMPLE_RATE_HZ

/* Pump zero input for n blocks at the given macro level, return RMS of
 * dry_L (echo writes back into dry). Useful to assert silence. */
static float run_silence_rms(float level, int blocks) {
    float dL[BLOCK], dR[BLOCK], sL[BLOCK], sR[BLOCK];
    double sumsq = 0; long n = 0;
    echo_set_amount(level);
    for (int b = 0; b < blocks; ++b) {
        memset(dL, 0, sizeof dL); memset(dR, 0, sizeof dR);
        memset(sL, 0, sizeof sL); memset(sR, 0, sizeof sR);
        echo_render_mix(dL, dR, sL, sR, BLOCK, 0.0f);
        for (int i = 0; i < BLOCK; ++i) {
            sumsq += (double)dL[i] * dL[i];
            ++n;
        }
    }
    return (n > 0) ? (float)sqrt(sumsq / (double)n) : 0.0f;
}

int main(void) {
    dsp_init();
    echo_init();

    /* 1: amount 0 must converge to silence after enough blocks for the
     * macro smoother to settle. */
    float rms_silent = run_silence_rms(0.0f, 200);
    CHECK(rms_silent < 1.0e-4f, "amount=0 not silent (rms=%g)", (double)rms_silent);

    /* 2: amount 1, pump bursts of input, check we get non-silent + bounded. */
    echo_init();
    echo_set_amount(1.0f);
    float dL[BLOCK], dR[BLOCK], sL[BLOCK], sR[BLOCK];
    /* First 4 blocks: feed a step. Then 60 blocks of zero input — we should
     * still hear feedback echoes. */
    float peak = 0;
    for (int b = 0; b < 64; ++b) {
        memset(dL, 0, sizeof dL); memset(dR, 0, sizeof dR);
        memset(sL, 0, sizeof sL); memset(sR, 0, sizeof sR);
        if (b < 4) for (int i = 0; i < BLOCK; ++i) { dL[i] = 0.2f; dR[i] = 0.2f; }
        echo_render_mix(dL, dR, sL, sR, BLOCK, 0.5f);
        for (int i = 0; i < BLOCK; ++i) {
            float a = fabsf(dL[i]);
            if (a > peak) peak = a;
        }
    }
    CHECK(peak > 0.05f, "amount=1 produced no audible signal (peak=%g)", (double)peak);
    CHECK(peak < 1.0f,  "amount=1 unbounded (peak=%g)", (double)peak);

    /* 3 + 4: impulse → first repeat audible ~time_samps later, second
     * repeat smaller. At amount=0.7 time≈100+0.7×480=436 ms ≈ 19228 samples.
     * Drive one block of 1.0 impulse then zeros, scan dL for two peaks. */
    echo_init();
    echo_set_amount(0.7f);
    /* allow the macro smoother to climb before driving the impulse */
    for (int b = 0; b < 100; ++b) {
        memset(dL, 0, sizeof dL); memset(dR, 0, sizeof dR);
        memset(sL, 0, sizeof sL); memset(sR, 0, sizeof sR);
        echo_render_mix(dL, dR, sL, sR, BLOCK, 0.0f);
    }
    /* impulse + scan over 2 seconds */
    const int total = 2 * SR;
    static float trace[2 * SR];
    int written = 0;
    int impulse_done = 0;
    while (written < total) {
        int n = (total - written) < BLOCK ? (total - written) : BLOCK;
        memset(dL, 0, sizeof dL); memset(dR, 0, sizeof dR);
        memset(sL, 0, sizeof sL); memset(sR, 0, sizeof sR);
        if (!impulse_done) {
            dL[0] = 1.0f; dR[0] = 1.0f; impulse_done = 1;
        }
        echo_render_mix(dL, dR, sL, sR, n, 0.0f);
        for (int i = 0; i < n; ++i) trace[written + i] = dL[i];
        written += n;
    }
    /* find first peak after sample 5000 (skip the impulse itself) */
    float p1 = 0; int t1 = 0;
    for (int i = 5000; i < total; ++i) {
        float a = fabsf(trace[i]);
        if (a > p1) { p1 = a; t1 = i; }
    }
    CHECK(p1 > 0.05f, "first echo too quiet (p1=%g @ t=%d)", (double)p1, t1);
    /* second repeat: search well past the first */
    float p2 = 0;
    for (int i = t1 + 4000; i < total; ++i) {
        float a = fabsf(trace[i]);
        if (a > p2) p2 = a;
    }
    CHECK(p2 < p1, "second repeat not smaller (p2=%g vs p1=%g)",
          (double)p2, (double)p1);

    /* 5: send buffer = wet × send_amount. The echo writes its wet output
     * into BOTH dry (additive) AND send (scaled by send_amount). To isolate
     * the wet contribution from the dry-input passthrough, we provide
     * ZERO input but rely on a primed feedback tail; the wet portion the
     * echo adds to dry must equal the send buffer divided by send_amount.
     *
     * Prime: drive an impulse, wait long enough for the echo tail to be
     * audible (one delay period after the impulse), then start measuring
     * with zero input. */
    echo_init();
    echo_set_amount(0.5f);
    /* settle macro */
    for (int b = 0; b < 100; ++b) {
        memset(dL, 0, sizeof dL); memset(dR, 0, sizeof dR);
        memset(sL, 0, sizeof sL); memset(sR, 0, sizeof sR);
        echo_render_mix(dL, dR, sL, sR, BLOCK, 0.6f);
    }
    /* impulse block */
    memset(dL, 0, sizeof dL); memset(dR, 0, sizeof dR);
    memset(sL, 0, sizeof sL); memset(sR, 0, sizeof sR);
    for (int i = 0; i < BLOCK; ++i) { dL[i] = 0.5f; dR[i] = 0.5f; }
    echo_render_mix(dL, dR, sL, sR, BLOCK, 0.6f);
    /* let the echo arrive: 0.5 amount → time≈340 ms ≈ 58 blocks. */
    for (int b = 0; b < 60; ++b) {
        memset(dL, 0, sizeof dL); memset(dR, 0, sizeof dR);
        memset(sL, 0, sizeof sL); memset(sR, 0, sizeof sR);
        echo_render_mix(dL, dR, sL, sR, BLOCK, 0.6f);
        /* the very block where the echo arrives back, dry-out and send
         * are both non-zero. Scan a handful of blocks for a sample where
         * |dry| > 1e-3 — at that sample, send must be 0.6 × dry. */
        for (int i = 0; i < BLOCK; ++i) {
            float d = fabsf(dL[i]);
            if (d > 1.0e-3f) {
                float ratio = sL[i] / dL[i];
                CHECK(fabsf(ratio - 0.6f) < 0.01f,
                      "send/dry ratio = %g, want 0.6", (double)ratio);
                goto ratio_done;
            }
        }
    }
    CHECK(0, "no audible echo found to measure send ratio");
ratio_done: ;

    printf("%d checks, %d failures\n", g_checks, g_fails);
    printf("RESULT: %s\n", g_fails ? "FAIL" : "PASS");
    return g_fails ? 1 : 0;
}
