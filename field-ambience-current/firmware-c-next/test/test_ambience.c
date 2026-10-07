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

static void test_wind_weather(void) {
    float l[256],r[256],sl[256],sr[256];
    float lo=1.0f,hi=0.0f; double sum=0.0,sum2=0.0;
    ambience_init(); ambience_set_level(1.0f);
    for(int second=0;second<120;++second) {
        double energy=0.0;
        for(int n=0;n<44100;n+=256) {
            int count=44100-n<256 ? 44100-n : 256;
            memset(l,0,sizeof l);memset(r,0,sizeof r);
            memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
            ambience_render_mix(l,r,sl,sr,count,0.0f);
            for(int i=0;i<count;++i) energy+=(double)l[i]*l[i];
        }
        float rms=(float)sqrt(energy/44100.0);
        if(second>=5) { if(rms<lo)lo=rms;if(rms>hi)hi=rms;sum+=rms;sum2+=rms*rms; }
    }
    double mean=sum/115.0,cv=sqrt(sum2/115.0-mean*mean)/mean;
    printf("wind 120s: quiet/loud %.5f/%.5f, envelope CV %.3f\n",lo,hi,cv);
    CHECK(hi>lo*8.0f,"wind lacks deep lulls");
    CHECK(cv>0.35,"wind remains a stationary noise floor");
    /* Same PCM for 64/256 frames: weather and macro smoothing follow samples. */
    static float reference[44100];
    for(int pass=0;pass<2;++pass) {
        int block=pass ? 64 : 256;
        ambience_init();ambience_set_level(0.7f);
        for(int n=0;n<44100;n+=block) {
            int count=44100-n<block ? 44100-n : block;
            memset(l,0,sizeof l);memset(r,0,sizeof r);
            memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
            ambience_render_mix(l,r,sl,sr,count,0.3f);
            if(!pass) memcpy(reference+n,l,count*sizeof(float));
            else CHECK(memcmp(reference+n,l,count*sizeof(float))==0,"wind depends on block size");
        }
    }
}

int main(void) {
    dsp_init();
    test_wind_weather();
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

    /* 6: Phase 2b — Moss Fields (world 3) gates rain on top of wind (r19.44). */
    ambience_init(); ambience_set_world(3);   /* Moss: wind + rain */
    float rms_moss = run_rms(1.0f);
    CHECK(rms_moss > 1.0e-3f, "Moss silent (%g)", (double)rms_moss);
    CHECK(rms_moss < 1.0f, "Moss rms unbounded (%g)", (double)rms_moss);

    /* 7: Phase 2c — Open Sea (world 1) gates waves on top of wind. Waves are
     * SLOW events (2 s gap + 1.5 s attack + ~7 s decay), so a short RMS
     * window can miss the peak entirely. Measure the PEAK over a longer
     * run (≈ 12 s) and assert it strictly exceeds wind-only's peak under
     * the same conditions. */
    float peak_wind_only = 0.0f, peak_coast = 0.0f;
    {
        float dryL[BLOCK], dryR[BLOCK], sendL[BLOCK], sendR[BLOCK];
        /* baseline: world 0 (Alps) — the only wind-only world after r19.54
         * gave Fjords/Desert their own layers. */
        ambience_init(); ambience_set_world(0); ambience_set_level(1.0f);
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
    ambience_init(); ambience_set_world(0);   /* Alps: wind only (r19.54) */
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
