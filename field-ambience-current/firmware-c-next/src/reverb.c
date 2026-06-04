/*
 * famReverbMaster — Schroeder/Freeverb-style algorithmic reverb.
 *
 * Standard Freeverb topology with the well-known tuning constants (designed
 * at 44.1 kHz, which is our sample rate, so the magic numbers transfer
 * directly):
 *
 *   in → pre-drive (tanh shaper, drive-modulated) → fixedGain
 *        → 8 parallel feedback combs (each with a damping 1-pole LP in the
 *          feedback path)
 *        → 4 series allpasses
 *        → wet out
 *
 * L and R use slightly different delay lengths (Freeverb's +23-sample
 * "stereo spread") so the wet field is decorrelated even from a mono input.
 *
 * Coefficient smoothing is per-block (caller can drive size/damp/drive from
 * the UI without zipper).
 */

#include "reverb.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define STEREO_SPREAD   23

/* Classic Freeverb tunings (44.1 kHz). */
static const int COMB_L[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
static const int AP_L  [4] = {  556,  441,  341,  225 };

/* Range / scaling constants (also from the canonical Freeverb). */
#define FIXED_GAIN      0.015f
#define SCALE_DAMP      0.4f
#define SCALE_ROOM      0.28f
#define OFFSET_ROOM     0.7f
#define AP_FEEDBACK     0.5f

#define MAX_COMB        1640        /* >= max(COMB_L)+STEREO_SPREAD */
#define MAX_AP          580         /* >= max(AP_L) +STEREO_SPREAD */

typedef struct {
    float buf[MAX_COMB];
    int   size;                     /* used length */
    int   pos;
    float filterstore;              /* 1-pole LP state for damping */
} comb_t;

typedef struct {
    float buf[MAX_AP];
    int   size;
    int   pos;
} allpass_t;

typedef struct {
    comb_t    comb[8];
    allpass_t ap[4];
} channel_t;

static channel_t L, R;

/* Smoothed control parameters (audio-thread side). */
static float feedback_cur, feedback_tgt;
static float dampval_cur,  dampval_tgt;        /* "damp", 0..SCALE_DAMP */
static float pre_cur, pre_tgt;                 /* drive pre-gain (1+drive*4) */
static float post_cur, post_tgt;               /* drive post-gain 1/(1+drive*0.6) */
static float ctl_coef;

static void comb_init(comb_t *c, int sz) {
    c->size = sz; c->pos = 0; c->filterstore = 0.0f;
    memset(c->buf, 0, sizeof c->buf);
}

static void ap_init(allpass_t *a, int sz) {
    a->size = sz; a->pos = 0;
    memset(a->buf, 0, sizeof a->buf);
}

void reverb_init(void) {
    for (int i = 0; i < 8; ++i) {
        comb_init(&L.comb[i], COMB_L[i]);
        comb_init(&R.comb[i], COMB_L[i] + STEREO_SPREAD);
    }
    for (int i = 0; i < 4; ++i) {
        ap_init(&L.ap[i], AP_L[i]);
        ap_init(&R.ap[i], AP_L[i] + STEREO_SPREAD);
    }
    feedback_cur = feedback_tgt = OFFSET_ROOM + SCALE_ROOM * 0.7f;     /* size = 0.7 */
    dampval_cur  = dampval_tgt  = SCALE_DAMP  * 0.3f;                  /* damp = 0.3 */
    pre_cur = pre_tgt   = 1.0f;
    post_cur = post_tgt = 1.0f;
    /* per-block smoothing toward target, time-constant ~120 ms (audible knob
     * still feels responsive, no zipper noise) */
    ctl_coef = 0.05f;
}

void reverb_set(float size_0_1, float damping_0_1) {
    size_0_1     = dsp_clampf(size_0_1,    0.0f, 1.0f);
    damping_0_1  = dsp_clampf(damping_0_1, 0.0f, 1.0f);
    feedback_tgt = OFFSET_ROOM + SCALE_ROOM * size_0_1;
    dampval_tgt  = SCALE_DAMP  * damping_0_1;
}

void reverb_set_drive(float drive_0_1) {
    drive_0_1 = dsp_clampf(drive_0_1, 0.0f, 1.0f);
    pre_tgt   = 1.0f + drive_0_1 * 4.0f;
    post_tgt  = 1.0f / (1.0f + drive_0_1 * 0.6f);
}

/* One comb sample: out = lp(buf[pos]); buf[pos] = in + out*feedback. */
static inline float comb_process(comb_t *c, float in, float fb, float damp) {
    float y = c->buf[c->pos];
    /* damping lowpass (one-pole) on the feedback path */
    c->filterstore = y * (1.0f - damp) + c->filterstore * damp;
    c->buf[c->pos] = in + c->filterstore * fb;
    if (++c->pos >= c->size) c->pos = 0;
    return y;
}

/* One Schroeder allpass sample. */
static inline float ap_process(allpass_t *a, float in) {
    float y = a->buf[a->pos];
    float v = in + y * AP_FEEDBACK;
    a->buf[a->pos] = v;
    if (++a->pos >= a->size) a->pos = 0;
    return y - in;                               /* allpass-1 output */
}

static inline float drive_shape(float x, float pre, float post) {
    return tanhf(x * pre * 2.2f) * post;        /* tanh(in*(1+d*4)*2.2) / (1+d*0.6) */
}

void reverb_render(const float *inL, const float *inR,
                   float *outL,       float *outR, int frames) {
    /* Smooth coefficients toward targets once per block. */
    feedback_cur += ctl_coef * (feedback_tgt - feedback_cur);
    dampval_cur  += ctl_coef * (dampval_tgt  - dampval_cur);
    pre_cur      += ctl_coef * (pre_tgt      - pre_cur);
    post_cur     += ctl_coef * (post_tgt     - post_cur);

    const float fb   = feedback_cur;
    const float damp = dampval_cur;
    const float pre  = pre_cur;
    const float post = post_cur;

    for (int n = 0; n < frames; ++n) {
        float xl = drive_shape(inL[n], pre, post) * FIXED_GAIN;
        float xr = drive_shape(inR[n], pre, post) * FIXED_GAIN;

        /* Combs in parallel, sum their outputs. */
        float yl = 0.0f, yr = 0.0f;
        for (int i = 0; i < 8; ++i) {
            yl += comb_process(&L.comb[i], xl, fb, damp);
            yr += comb_process(&R.comb[i], xr, fb, damp);
        }

        /* Allpasses in series. */
        for (int i = 0; i < 4; ++i) {
            yl = ap_process(&L.ap[i], yl);
            yr = ap_process(&R.ap[i], yr);
        }

        outL[n] = yl;
        outR[n] = yr;
    }
}
