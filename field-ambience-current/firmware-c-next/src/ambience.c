/*
 * ambience.c — per-world atmospheric layer (ADR-0017 Phase 2a).
 *
 * WIND generator lifted from tools/render_worlds.c. Pink noise feeds a
 * resonant bandpass (Q≈2.5) whose centre is slowly swept between 350–900 Hz
 * over ~14 s, with random gust amplitude envelopes every 4–8 s for the
 * breathing feel. L/R use independent noise streams + a slight BP centre
 * offset for stereo width.
 *
 * Phase 2b/c/d will add rain (Tokyo), waves (Coast), and vinyl (After
 * Hours) generators alongside.
 */

#include "ambience.h"
#include "dsp.h"
#include <stdint.h>

#define SR            ((float)DSP_SAMPLE_RATE_HZ)
#define SILENCE_EPS   1.0e-5f
#define LEVEL_COEF    0.04f       /* per-block one-pole, ~50 ms */

static int   world_i = 0;
static float level_cur = 0.0f, level_tgt = 0.0f;

/* --- wind state (lifted from tools/render_worlds.c) ---------------------- */

static uint32_t wnd_rng_L = 0xACE12345u, wnd_rng_R = 0x7B19F88Au;
static dsp_svf_t wnd_bpL, wnd_bpR;
static float wnd_lfo = 0.0f;
static float wnd_pink_L_b0 = 0, wnd_pink_L_b1 = 0, wnd_pink_L_b2 = 0;
static float wnd_pink_R_b0 = 0, wnd_pink_R_b1 = 0, wnd_pink_R_b2 = 0;
static float wnd_gust_env  = 0.5f;
static int   wnd_gust_until = 0;

static inline float wnd_white(uint32_t *r) {
    *r = (*r) * 1664525u + 1013904223u;
    return (float)((int32_t)*r) * (1.0f / 2147483648.0f);
}

/* 3-pole pink-noise filter (Paul Kellet approximation) — gives the low-mid
 * weight that makes the source sound like moving air instead of static. */
static inline float wnd_pink(uint32_t *rng, float *b0, float *b1, float *b2) {
    float w = wnd_white(rng);
    *b0 = 0.99765f * (*b0) + w * 0.0990460f;
    *b1 = 0.96300f * (*b1) + w * 0.2965164f;
    *b2 = 0.57000f * (*b2) + w * 1.0526913f;
    return (*b0 + *b1 + *b2 + w * 0.1848f) * 0.18f;
}

void ambience_init(void) {
    world_i   = 0;
    level_cur = 0.0f;
    level_tgt = 0.0f;
    dsp_svf_reset(&wnd_bpL);
    dsp_svf_reset(&wnd_bpR);
    wnd_lfo        = 0.0f;
    wnd_gust_env   = 0.5f;
    wnd_gust_until = (int)(SR * 5.0f);
    wnd_pink_L_b0 = wnd_pink_L_b1 = wnd_pink_L_b2 = 0.0f;
    wnd_pink_R_b0 = wnd_pink_R_b1 = wnd_pink_R_b2 = 0.0f;
}

void ambience_set_world(int idx) {
    if (idx < 0) idx = 0;
    /* Upper-bound clamping is the caller's job (worlds_get clamps); we just
     * stash the index for the future per-world dispatch in Phase 2b/c/d. */
    world_i = idx;
}

void ambience_set_level(float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    level_tgt = v;
}

void ambience_render_mix(float *dry_L, float *dry_R,
                         float *send_L, float *send_R,
                         int frames, float send_amount) {
    /* Smooth target → current at block rate so user macro changes don't zipper. */
    level_cur += LEVEL_COEF * (level_tgt - level_cur);

    /* Cheap exit when effectively silent — saves the per-sample work. */
    if (level_cur < SILENCE_EPS && level_tgt < SILENCE_EPS) return;
    /* Suppress unused-var until Phase 2b/c/d pick per-world generators. */
    (void)world_i;

    for (int n = 0; n < frames; ++n) {
        /* 14 s sweep: BP centre 350..900 Hz */
        wnd_lfo += (1.0f / 14.0f) / SR;
        if (wnd_lfo >= 1.0f) wnd_lfo -= 1.0f;
        float s = dsp_sin(wnd_lfo);
        float centre = 625.0f + s * 275.0f;
        dsp_svf_set(&wnd_bpL, centre,         2.5f);
        dsp_svf_set(&wnd_bpR, centre * 1.07f, 2.5f);   /* slight stereo offset */

        /* random gusts every 4..8 s */
        if (--wnd_gust_until <= 0) {
            wnd_gust_env   = 0.85f + (wnd_white(&wnd_rng_L) * 0.5f + 0.5f) * 0.5f;
            float rrange   = (wnd_white(&wnd_rng_R) * 0.5f + 0.5f) * 4.0f;
            wnd_gust_until = (int)(SR * (4.0f + rrange));
        }
        wnd_gust_env *= 0.99996f;                       /* slow attack/decay */
        float gust = 0.40f + wnd_gust_env * 0.6f;

        float pL = wnd_pink(&wnd_rng_L, &wnd_pink_L_b0, &wnd_pink_L_b1, &wnd_pink_L_b2);
        float pR = wnd_pink(&wnd_rng_R, &wnd_pink_R_b0, &wnd_pink_R_b1, &wnd_pink_R_b2);
        float bpL = dsp_svf_bp(&wnd_bpL, pL);
        float bpR = dsp_svf_bp(&wnd_bpR, pR);

        float outL = bpL * level_cur * gust;
        float outR = bpR * level_cur * gust;

        dry_L[n]  += outL;
        dry_R[n]  += outR;
        send_L[n] += outL * send_amount;
        send_R[n] += outR * send_amount;
    }
}
