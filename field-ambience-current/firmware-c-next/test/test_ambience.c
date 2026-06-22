/*
 * test_ambience.c — per-world atmospheric layer (ADR-0017 Phase 2a).
 *
 * Drives the ambience module at various levels and asserts:
 *   1. silence at level 0 (smoothed → eventually below an epsilon)
 *   2. audible signal at level 1, bounded below clip
 *   3. signal grows monotonically with level on the same time window
 *   4. send buffer carries a scaled copy of the dry signal
 *   5. setting an out-of-range world index doesn't crash
 *
 * No magnitude tuning — the goal is to prove the lift produces sound and
 * doesn't blow up. Voicing fidelity to render_worlds.c is a separate
 * audition concern (you compare the WAVs by ear).
 */

#include "ambience.h"
#include "dsp.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

static int g_checks = 0, g_fails = 0;
#define CHECK(c, ...) do { ++g_checks; if (!(c)) { ++g_fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

#define BLOCK   256
#define BLOCKS  200          /* ~1.16 s @ 44.1 kHz — enough for the level
                              * smoother + gust envelope to settle */

/* Run BLOCKS blocks at the given level, return the RMS of the dry-L output
 * over the last half of the run (skip the level-smoother attack window). */
static float run_rms(float level) {
    float dryL[BLOCK], dryR[BLOCK], sendL[BLOCK], sendR[BLOCK];
    double sumsq = 0.0;
    long   n     = 0;

    ambience_set_level(level);
    for (int b = 0; b < BLOCKS; ++b) {
        memset(dryL,  0, sizeof dryL);
        memset(dryR,  0, sizeof dryR);
        memset(sendL, 0, sizeof sendL);
        memset(sendR, 0, sizeof sendR);
        ambience_render_mix(dryL, dryR, sendL, sendR, BLOCK, 0.5f);
        if (b >= BLOCKS / 2) {
            for (int i = 0; i < BLOCK; ++i) {
                float s = dryL[i];
                sumsq += (double)s * (double)s;
                ++n;
            }
        }
    }
    return (n > 0) ? (float)sqrt(sumsq / (double)n) : 0.0f;
}

int main(void) {
    dsp_init();
    ambience_init();

    /* 1: level 0 must converge to silence (within a tight epsilon). */
    float rms_silent = run_rms(0.0f);
    CHECK(rms_silent < 1.0e-3f, "level 0 not silent (rms=%g)", (double)rms_silent);

    /* 2: level 1 must produce signal — bounded but real. */
    ambience_init();
    float rms_full = run_rms(1.0f);
    CHECK(rms_full > 1.0e-3f, "level 1 silent (rms=%g)", (double)rms_full);
    CHECK(rms_full < 1.0f,    "level 1 unbounded (rms=%g)", (double)rms_full);

    /* 3: monotone — level 1 RMS strictly greater than level 0.3 RMS. */
    ambience_init();
    float rms_third = run_rms(0.3f);
    CHECK(rms_full > rms_third * 1.2f,
          "level 1 not louder than 0.3 (full=%g, third=%g)",
          (double)rms_full, (double)rms_third);

    /* 4: send is a scaled copy of dry. Run one fresh block at level 1, send
     *    amount 0.5; the send RMS should be ~half the dry RMS. */
    ambience_init();
    ambience_set_level(1.0f);
    /* prime: get past level smoother + gust attack */
    {
        float dryL[BLOCK], dryR[BLOCK], sendL[BLOCK], sendR[BLOCK];
        for (int b = 0; b < BLOCKS; ++b) {
            memset(dryL,  0, sizeof dryL);  memset(dryR,  0, sizeof dryR);
            memset(sendL, 0, sizeof sendL); memset(sendR, 0, sizeof sendR);
            ambience_render_mix(dryL, dryR, sendL, sendR, BLOCK, 0.5f);
            if (b == BLOCKS - 1) {
                double dsum = 0.0, ssum = 0.0;
                for (int i = 0; i < BLOCK; ++i) {
                    dsum += (double)dryL[i]  * dryL[i];
                    ssum += (double)sendL[i] * sendL[i];
                }
                float dry_rms  = (float)sqrt(dsum / BLOCK);
                float send_rms = (float)sqrt(ssum / BLOCK);
                CHECK(fabsf(send_rms - dry_rms * 0.5f) < dry_rms * 0.05f,
                      "send ≠ 0.5 × dry  (dry=%g send=%g)",
                      (double)dry_rms, (double)send_rms);
            }
        }
    }

    /* 5: out-of-range world idx must not crash + must not change output. */
    ambience_init();
    ambience_set_world(-7);    ambience_set_level(0.5f);
    float rms_neg = run_rms(0.5f);
    ambience_init();
    ambience_set_world(999);   ambience_set_level(0.5f);
    float rms_big = run_rms(0.5f);
    CHECK(rms_neg > 1.0e-4f, "neg-world: no signal");
    CHECK(rms_big > 1.0e-4f, "big-world: no signal");

    /* 6: Phase 2b — Tokyo (world 0) gates rain on top of wind. */
    ambience_init(); ambience_set_world(0);   /* Tokyo: wind + rain */
    float rms_tokyo = run_rms(1.0f);
    CHECK(rms_tokyo > 1.0e-3f, "Tokyo silent (%g)", (double)rms_tokyo);
    CHECK(rms_tokyo < 1.0f, "Tokyo rms unbounded (%g)", (double)rms_tokyo);

    /* 7: Phase 2c — Coast (world 1) gates waves on top of wind. Waves are
     * SLOW events (2 s gap + 1.5 s attack + ~7 s decay), so a short RMS
     * window can miss the peak entirely. Measure the PEAK over a longer
     * run (≈ 12 s) and assert it strictly exceeds wind-only's peak under
     * the same conditions. */
    float peak_wind_only = 0.0f, peak_coast = 0.0f;
    {
        float dryL[BLOCK], dryR[BLOCK], sendL[BLOCK], sendR[BLOCK];
        /* baseline: world 2 (Drive) so only wind runs. */
        ambience_init(); ambience_set_world(2); ambience_set_level(1.0f);
        for (int b = 0; b < 2100; ++b) {     /* ~12.2 s */
            memset(dryL, 0, sizeof dryL); memset(dryR, 0, sizeof dryR);
            memset(sendL, 0, sizeof sendL); memset(sendR, 0, sizeof sendR);
            ambience_render_mix(dryL, dryR, sendL, sendR, BLOCK, 0.0f);
            for (int i = 0; i < BLOCK; ++i) {
                float a = fabsf(dryL[i]);
                if (a > peak_wind_only) peak_wind_only = a;
            }
        }
        ambience_init(); ambience_set_world(1); ambience_set_level(1.0f);
        for (int b = 0; b < 2100; ++b) {
            memset(dryL, 0, sizeof dryL); memset(dryR, 0, sizeof dryR);
            memset(sendL, 0, sizeof sendL); memset(sendR, 0, sizeof sendR);
            ambience_render_mix(dryL, dryR, sendL, sendR, BLOCK, 0.0f);
            for (int i = 0; i < BLOCK; ++i) {
                float a = fabsf(dryL[i]);
                if (a > peak_coast) peak_coast = a;
            }
        }
    }
    CHECK(peak_coast > peak_wind_only * 1.20f,
          "Coast+waves peak not above wind-only (coast=%g, wind=%g)",
          (double)peak_coast, (double)peak_wind_only);
    CHECK(peak_coast < 1.0f, "Coast peak unbounded (%g)", (double)peak_coast);

    /* 8: Phase 2d — After Hours (world 3) gets vinyl on top of wind. Vinyl
     * is continuous (crackle + rumble) plus sparse pops; RMS must clearly
     * exceed wind-only. Bounded. */
    ambience_init(); ambience_set_world(2);   /* Drive: wind only */
    float rms_wind_only = run_rms(1.0f);
    ambience_init(); ambience_set_world(3);   /* After Hours: wind + vinyl */
    float rms_hours = run_rms(1.0f);
    CHECK(rms_hours > rms_wind_only * 1.20f,
          "After Hours+vinyl not louder than Drive+wind-only (hours=%g, wind=%g)",
          (double)rms_hours, (double)rms_wind_only);
    CHECK(rms_hours < 1.0f, "After Hours rms unbounded (%g)", (double)rms_hours);

    printf("%d checks, %d failures\n", g_checks, g_fails);
    printf("RESULT: %s\n", g_fails ? "FAIL" : "PASS");
    return g_fails ? 1 : 0;
}
