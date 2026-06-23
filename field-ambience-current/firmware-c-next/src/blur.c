/*
 * blur.c — granular smear (Reddit Blur macro).
 *
 * Ring-buffer-based granular cloud, lifted in CONCEPT from the
 * Dumumub Granular (JUCE) repo and the Mutable Instruments
 * Beads/Clouds family. From-scratch C, libm-free.
 *
 * Per sample:
 *   - write current dry input to ring buffer
 *   - for each ACTIVE grain: read ring at (writePos − grain_offset_now)
 *     with linear interpolation, apply Hann-like envelope, add to L/R
 *     via per-grain pan, advance grain phase
 *   - if grain finished → mark idle
 *   - decrement until_next_grain; on zero → spawn a new grain in an idle
 *     slot (random position-jitter + pitch-jitter + pan, all controlled
 *     by the user macro)
 *
 * The envelope is a cheap raised-cosine approximation: e(t) = sin(π t)
 * for t∈[0,1] → bounded smooth bell, zero at edges (no clicks).
 */

#include "blur.h"
#include "dsp.h"
#include <stdint.h>
#include <string.h>

#define SR              ((float)DSP_SAMPLE_RATE_HZ)
#define BLUR_RING_S     0.20f
/* Integer-only so the ring is a real compile-time constant: 0.2 s × 44100 = 8820. */
#define BLUR_RING_LEN   ((2 * DSP_SAMPLE_RATE_HZ) / 10)   /* 8820 samples */
#define BLUR_MAX_GRAINS 16
#define SILENCE_EPS     1.0e-5f
#define SMOOTH_COEF     0.04f

typedef struct {
    int   active;
    int   age_samps;       /* counts from 0 to len_samps */
    int   len_samps;       /* grain length in samples (≥ 1) */
    float read_pos;        /* fractional sample-index within the ring
                            * relative to wr_idx at spawn time */
    float pitch;           /* playback rate (1.0 = original) */
    float panL, panR;
} grain_t;

static float    ringL[BLUR_RING_LEN];
static float    ringR[BLUR_RING_LEN];
static int      wr_idx = 0;
static grain_t  grains[BLUR_MAX_GRAINS];
static int      until_next = 1;
static uint32_t lcg = 0xDEC1ACAFu;

static float    amount_cur = 0.0f, amount_tgt = 0.0f;
/* Live macro-derived params (recomputed per block from amount_cur) */
static int      grain_min_len, grain_max_len;
static int      iso_min_samps, iso_max_samps;   /* gap between grains */
static float    pitch_jitter_max;               /* in semitones */
static float    pos_scatter_samps;
static float    wet;

static inline float lcg_unit(void) {
    /* uniform in [-1, +1] */
    lcg = lcg * 1664525u + 1013904223u;
    return (float)((int32_t)lcg) * (1.0f / 2147483648.0f);
}
static inline float lcg_uniform(void) {
    /* uniform in [0, 1] */
    return lcg_unit() * 0.5f + 0.5f;
}

static void recompute_params(float a) {
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    /* grain size 30..90 ms */
    int len_lo = (int)(0.030f * SR);
    int len_hi = (int)((0.030f + a * 0.060f) * SR);
    grain_min_len = len_lo;
    grain_max_len = len_hi > len_lo ? len_hi : len_lo + 1;
    /* density 6..35 grains/s → inter-onset 28..167 ms */
    float density = 6.0f + a * 29.0f;
    float ioi_mean_s = 1.0f / density;
    iso_min_samps = (int)(ioi_mean_s * 0.5f * SR);
    iso_max_samps = (int)(ioi_mean_s * 1.5f * SR);
    if (iso_max_samps <= iso_min_samps) iso_max_samps = iso_min_samps + 1;
    /* pitch jitter 0..1.5 semitones at amount 1 */
    pitch_jitter_max = a * 1.5f;
    /* position scatter 20..90 ms back from the write head */
    pos_scatter_samps = (0.020f + a * 0.070f) * SR;
    /* wet mix 0..0.7 */
    wet = a * 0.70f;
}

/* 2^(s/12) without libm — cheap polynomial over -1.5..+1.5 semitones */
static inline float semitones_to_ratio(float s) {
    /* good fit for s∈[-1.5,1.5]: 1 + 0.0578·s + 0.00167·s² */
    return 1.0f + 0.0578f * s + 0.00167f * s * s;
}

void blur_init(void) {
    memset(ringL, 0, sizeof ringL);
    memset(ringR, 0, sizeof ringR);
    memset(grains, 0, sizeof grains);
    wr_idx = 0;
    until_next = 1;
    amount_cur = amount_tgt = 0.0f;
    recompute_params(0.0f);
}

void blur_set_amount(float a) {
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    amount_tgt = a;
}

static void spawn_grain(void) {
    int slot = -1;
    for (int g = 0; g < BLUR_MAX_GRAINS; ++g) {
        if (!grains[g].active) { slot = g; break; }
    }
    if (slot < 0) return;   /* all busy, skip this onset */
    grain_t *gr = &grains[slot];

    /* length within the macro-set range */
    int range = grain_max_len - grain_min_len + 1;
    gr->len_samps = grain_min_len + (int)(lcg_uniform() * (float)range);
    gr->age_samps = 0;
    /* position back from the write head, jittered. Cap so we stay in ring. */
    float offset = pos_scatter_samps + lcg_uniform() * pos_scatter_samps;
    if (offset > (float)(BLUR_RING_LEN - gr->len_samps - 2))
        offset = (float)(BLUR_RING_LEN - gr->len_samps - 2);
    if (offset < 1.0f) offset = 1.0f;
    /* read_pos is the position INSIDE the ring at grain start (not the
     * abs ring idx — we track it as float and advance by pitch each
     * sample, modding with ring length at read time) */
    gr->read_pos = (float)wr_idx - offset;
    while (gr->read_pos < 0.0f) gr->read_pos += (float)BLUR_RING_LEN;
    /* pitch jitter */
    float st = lcg_unit() * pitch_jitter_max;
    gr->pitch = semitones_to_ratio(st);
    /* random pan, equal-power-ish */
    float p = lcg_unit();   /* [-1, +1] */
    /* simple equal-power lerp: cos/sin from polynomial */
    float t = (p + 1.0f) * 0.5f;           /* 0..1 */
    gr->panL = 1.0f - t * 0.7f;
    gr->panR = 0.3f + t * 0.7f;
}

void blur_render_mix(float *dry_L, float *dry_R,
                     float *send_L, float *send_R,
                     int frames, float send_amount) {
    amount_cur += SMOOTH_COEF * (amount_tgt - amount_cur);
    if (amount_cur < SILENCE_EPS && amount_tgt < SILENCE_EPS) {
        /* still capture input so when the user re-opens Blur there's
         * something in the ring to grain from */
        for (int n = 0; n < frames; ++n) {
            ringL[wr_idx] = dry_L[n];
            ringR[wr_idx] = dry_R[n];
            if (++wr_idx >= BLUR_RING_LEN) wr_idx = 0;
        }
        return;
    }
    recompute_params(amount_cur);

    const int ring = BLUR_RING_LEN;

    for (int n = 0; n < frames; ++n) {
        /* 1. capture this sample into the ring */
        ringL[wr_idx] = dry_L[n];
        ringR[wr_idx] = dry_R[n];

        /* 2. sum all active grains into a stereo accumulator */
        float gL = 0.0f, gR = 0.0f;
        for (int g = 0; g < BLUR_MAX_GRAINS; ++g) {
            grain_t *gr = &grains[g];
            if (!gr->active) continue;

            /* envelope: e(t) = sin(π t) where t = age/len, ∈[0,1].
             * sin(π t) ≈ 4t(1-t) for t∈[0,1] (peak ≈ 1 at t=0.5) — cheap
             * raised-bump that's zero at edges, no libm. */
            float t  = (float)gr->age_samps / (float)gr->len_samps;
            float env = 4.0f * t * (1.0f - t);

            /* linear-interp read at gr->read_pos in the ring */
            int   i0 = (int)gr->read_pos;
            int   i1 = i0 + 1;
            float fr = gr->read_pos - (float)i0;
            if (i0 >= ring) i0 -= ring;
            if (i1 >= ring) i1 -= ring;
            float sL = ringL[i0] + (ringL[i1] - ringL[i0]) * fr;
            float sR = ringR[i0] + (ringR[i1] - ringR[i0]) * fr;

            gL += sL * env * gr->panL;
            gR += sR * env * gr->panR;

            /* advance grain */
            gr->read_pos += gr->pitch;
            if (gr->read_pos >= (float)ring) gr->read_pos -= (float)ring;
            ++gr->age_samps;
            if (gr->age_samps >= gr->len_samps) gr->active = 0;
        }

        /* 3. schedule next grain */
        if (--until_next <= 0) {
            grains[0].active = grains[0].active;   /* (no-op, just a marker) */
            /* Spawn into the first idle slot. */
            int slot = -1;
            for (int g = 0; g < BLUR_MAX_GRAINS; ++g) {
                if (!grains[g].active) { slot = g; break; }
            }
            if (slot >= 0) {
                grains[slot].active = 1;
                grains[slot].age_samps = 0;
                /* (rest filled by spawn_grain — but for tighter
                 * control we inline-spawn here so we don't double the
                 * idle-slot scan) */
                int range = grain_max_len - grain_min_len + 1;
                grains[slot].len_samps = grain_min_len + (int)(lcg_uniform() * (float)range);
                float offset = pos_scatter_samps + lcg_uniform() * pos_scatter_samps;
                if (offset > (float)(ring - grains[slot].len_samps - 2))
                    offset = (float)(ring - grains[slot].len_samps - 2);
                if (offset < 1.0f) offset = 1.0f;
                grains[slot].read_pos = (float)wr_idx - offset;
                while (grains[slot].read_pos < 0.0f) grains[slot].read_pos += (float)ring;
                float st = lcg_unit() * pitch_jitter_max;
                grains[slot].pitch = semitones_to_ratio(st);
                float p = lcg_unit();
                float tt = (p + 1.0f) * 0.5f;
                grains[slot].panL = 1.0f - tt * 0.7f;
                grains[slot].panR = 0.3f + tt * 0.7f;
            }
            int span = iso_max_samps - iso_min_samps;
            until_next = iso_min_samps + (int)(lcg_uniform() * (float)span);
        }

        /* 4. write the wet cloud into dry + send */
        float wL = gL * wet, wR = gR * wet;
        dry_L[n]  += wL;
        dry_R[n]  += wR;
        send_L[n] += wL * send_amount;
        send_R[n] += wR * send_amount;

        if (++wr_idx >= ring) wr_idx = 0;
    }

    /* Suppress "defined but not used" if the spawn_grain helper isn't
     * inlined out (we kept it for readability / tests). */
    (void)spawn_grain;
}
