/*
 * test_blur.c — granular cloud / smear macro (Reddit Blur).
 *
 * Invariants:
 *   1. amount=0 must be silent on the wet path (the ring still captures
 *      input, but no grains write back).
 *   2. amount=1 with a steady tonal input must produce ADDITIONAL signal
 *      on top of the dry (cloud is layered, not gated).
 *   3. Output stays bounded (< 1.0) at amount=1.
 *   4. send buffer = dry-wet × send_amount (the cloud is the only thing
 *      written, so we can read the ratio off the test directly).
 */

#include "blur.h"
#include "dsp.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

static int g_checks = 0, g_fails = 0;
#define CHECK(c, ...) do { ++g_checks; if (!(c)) { ++g_fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

#define BLOCK 256

/* Feed a 220 Hz tonal source so the grains have something to capture. */
static void fill_tone(float *L, float *R, int frames, double *phase) {
    const double w = 2.0 * 3.14159265358979 * 220.0 / 44100.0;
    for (int i = 0; i < frames; ++i) {
        float s = (float)sin(*phase) * 0.2f;
        L[i] = s; R[i] = s;
        *phase += w;
    }
}

int main(void) {
    dsp_init();
    blur_init();

    /* 1: amount=0 path. Feed tone, capture pre-mix; after blur the L/R
     * buffers should be unchanged (within tiny epsilon — none of the
     * float traffic should add anything when wet=0). */
    float dL[BLOCK], dR[BLOCK], sL[BLOCK], sR[BLOCK];
    float refL[BLOCK];
    double ph = 0;
    blur_set_amount(0.0f);
    for (int b = 0; b < 200; ++b) {       /* drain macro smoother */
        fill_tone(dL, dR, BLOCK, &ph);
        for (int i = 0; i < BLOCK; ++i) refL[i] = dL[i];
        memset(sL, 0, sizeof sL); memset(sR, 0, sizeof sR);
        blur_render_mix(dL, dR, sL, sR, BLOCK, 0.5f);
        if (b == 199) {
            float max_delta = 0;
            for (int i = 0; i < BLOCK; ++i) {
                float d = fabsf(dL[i] - refL[i]);
                if (d > max_delta) max_delta = d;
            }
            CHECK(max_delta < 1.0e-5f, "amount=0 polluted dry by %g", (double)max_delta);
            double sumsq = 0;
            for (int i = 0; i < BLOCK; ++i) sumsq += (double)sL[i] * sL[i];
            float send_rms = (float)sqrt(sumsq / BLOCK);
            CHECK(send_rms < 1.0e-5f, "amount=0 polluted send (rms=%g)", (double)send_rms);
        }
    }

    /* 2 + 3: amount=1 — must add audible cloud + stay bounded. */
    blur_init();
    blur_set_amount(1.0f);
    /* warm up ring */
    ph = 0;
    for (int b = 0; b < 200; ++b) {
        fill_tone(dL, dR, BLOCK, &ph);
        memset(sL, 0, sizeof sL); memset(sR, 0, sizeof sR);
        blur_render_mix(dL, dR, sL, sR, BLOCK, 0.5f);
    }
    /* now measure the difference: feed the same tone, sample with vs
     * without (re-do with amount=0 fresh init). */
    double sum_with = 0, sum_without = 0;
    float peak_with = 0;
    for (int b = 0; b < 50; ++b) {
        fill_tone(dL, dR, BLOCK, &ph);
        memset(sL, 0, sizeof sL); memset(sR, 0, sizeof sR);
        blur_render_mix(dL, dR, sL, sR, BLOCK, 0.5f);
        for (int i = 0; i < BLOCK; ++i) {
            sum_with += (double)dL[i] * dL[i];
            float a = fabsf(dL[i]);
            if (a > peak_with) peak_with = a;
        }
    }
    /* reference run, amount=0 — only dry */
    blur_init();
    blur_set_amount(0.0f);
    ph = 0;
    for (int b = 0; b < 200; ++b) {
        fill_tone(dL, dR, BLOCK, &ph);
        memset(sL, 0, sizeof sL); memset(sR, 0, sizeof sR);
        blur_render_mix(dL, dR, sL, sR, BLOCK, 0.5f);
    }
    for (int b = 0; b < 50; ++b) {
        fill_tone(dL, dR, BLOCK, &ph);
        memset(sL, 0, sizeof sL); memset(sR, 0, sizeof sR);
        blur_render_mix(dL, dR, sL, sR, BLOCK, 0.5f);
        for (int i = 0; i < BLOCK; ++i) sum_without += (double)dL[i] * dL[i];
    }
    float rms_with    = (float)sqrt(sum_with    / (50 * BLOCK));
    float rms_without = (float)sqrt(sum_without / (50 * BLOCK));
    CHECK(rms_with > rms_without * 1.10f,
          "amount=1 didn't add cloud (with=%g, without=%g)",
          (double)rms_with, (double)rms_without);
    CHECK(peak_with < 1.0f, "amount=1 unbounded (peak=%g)", (double)peak_with);

    /* 4: send is a 0.5× copy of the cloud added to dry. Fresh init,
     * amount=1, sample one block. Subtract reference (dry tone alone)
     * from the wet result, compare to send. */
    blur_init();
    blur_set_amount(1.0f);
    ph = 0;
    for (int b = 0; b < 200; ++b) {
        fill_tone(dL, dR, BLOCK, &ph);
        memset(sL, 0, sizeof sL); memset(sR, 0, sizeof sR);
        blur_render_mix(dL, dR, sL, sR, BLOCK, 0.5f);
    }
    fill_tone(dL, dR, BLOCK, &ph);
    for (int i = 0; i < BLOCK; ++i) refL[i] = dL[i];   /* dry-input baseline */
    memset(sL, 0, sizeof sL); memset(sR, 0, sizeof sR);
    blur_render_mix(dL, dR, sL, sR, BLOCK, 0.5f);
    double cloud_sumsq = 0, send_sumsq = 0;
    for (int i = 0; i < BLOCK; ++i) {
        float cloud = dL[i] - refL[i];      /* what blur added */
        cloud_sumsq += (double)cloud * cloud;
        send_sumsq  += (double)sL[i] * sL[i];
    }
    float cloud_rms = (float)sqrt(cloud_sumsq / BLOCK);
    float send_rms  = (float)sqrt(send_sumsq  / BLOCK);
    if (cloud_rms > 1.0e-5f) {
        float ratio = send_rms / cloud_rms;
        CHECK(fabsf(ratio - 0.5f) < 0.05f,
              "send/cloud ratio = %g, want 0.5", (double)ratio);
    } else {
        CHECK(0, "no audible cloud in send test");
    }

    printf("%d checks, %d failures\n", g_checks, g_fails);
    printf("RESULT: %s\n", g_fails ? "FAIL" : "PASS");
    return g_fails ? 1 : 0;
}
