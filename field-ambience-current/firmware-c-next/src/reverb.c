/*
 * famReverbMaster — LIQUID modulated-FDN reverb (r18.42, default), with a
 * compile-time fallback to the historical Schroeder/Freeverb topology.
 *
 *   FAM_REVERB_MODE = 1 (default, LIQUID):
 *       pre-delay → 2 modulated allpass input diffusers
 *                 → 8 parallel modulated combs (per-line slow LFO, fractional
 *                   read, 1-pole damping LP in feedback)
 *                 → 4 modulated allpass output diffusers
 *                 → wet out
 *
 *   FAM_REVERB_MODE = 0 (legacy Freeverb fallback):
 *       same Freeverb topology this file used to ship with — static comb
 *       delays, static allpasses. Kept for A/B + emergency-revert. Identical
 *       tuning numbers as before.
 *
 * Picked LIQUID after a 3-way audition (tools/render_reverb_ab.c) against
 * the accepted dreamy/warm-pop reference. The static Freeverb tail reads as
 * a fixed/slightly-metallic comb ring on long ambient washes; the modulated
 * version detunes the modes continuously into a lush 3D wash; adding
 * Greyhole-style modulated allpass diffusion (the SuperCollider Greyhole
 * idea, reimplemented clean-room in C — algorithms aren't copyrighted)
 * smears the early reflections into the dense, liquid ambient tail. All
 * three libraries the user pointed at (Surge, DaisySP, SuperCollider) were
 * checked first: DaisySP (MIT) ships no reverb; Surge sst-effects (GPL3)
 * and SC's GVerb/JPverb/Greyhole (GPL3) can't be linked into closed
 * firmware. So this is implemented from the techniques, not copied.
 *
 * Public API unchanged (reverb_init / reverb_set / reverb_set_drive /
 * reverb_render); coefficient smoothing per-block also unchanged.
 *
 * Memory: L+R total ≈ 166 KB at FAM_REVERB_MODE=1 (was ≈ 124 KB at mode 0).
 * Trivial on STM32H7 (1 MB RAM) and the RP2350 bench (520 KB SRAM).
 */

#include "reverb.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#ifndef FAM_REVERB_MODE
#define FAM_REVERB_MODE 1               /* 0 = Freeverb (legacy), 1 = LIQUID */
#endif

#define STEREO_SPREAD   23
#define FIXED_GAIN      0.015f
#define SCALE_DAMP      0.4f
#define SCALE_ROOM      0.28f
#define OFFSET_ROOM     0.7f
#define AP_FEEDBACK     0.5f

/* Smoothed control parameters (audio-thread side; same shape as before). */
static float feedback_cur, feedback_tgt;
static float dampval_cur,  dampval_tgt;
static float pre_cur,  pre_tgt;
static float post_cur, post_tgt;
static float ctl_coef;

static inline float drive_shape(float x, float pre, float post) {
    return tanhf(x * pre * 2.2f) * post;
}

/* =========================================================================
 * Common: classic Freeverb 44.1 kHz delay lengths reused by both modes.
 * ========================================================================= */
static const int COMB_LEN[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
static const int AP_LEN  [4] = {  556,  441,  341,  225 };

#if FAM_REVERB_MODE == 0
/* ===== Legacy Freeverb (static delays) ===================================== */

#define MAX_COMB        1640
#define MAX_AP          580

typedef struct { float buf[MAX_COMB]; int size, pos; float filterstore; } comb_t;
typedef struct { float buf[MAX_AP];   int size, pos; } allpass_t;
typedef struct { comb_t comb[8]; allpass_t ap[4]; } channel_t;
static channel_t L, R;

static inline float comb_process(comb_t *c, float in, float fb, float damp) {
    float y = c->buf[c->pos];
    c->filterstore = y * (1.0f - damp) + c->filterstore * damp;
    c->buf[c->pos] = in + c->filterstore * fb;
    if (++c->pos >= c->size) c->pos = 0;
    return y;
}
static inline float ap_process(allpass_t *a, float in) {
    float y = a->buf[a->pos];
    a->buf[a->pos] = in + y * AP_FEEDBACK;
    if (++a->pos >= a->size) a->pos = 0;
    return y - in;
}

void reverb_init(void) {
    memset(&L, 0, sizeof L); memset(&R, 0, sizeof R);
    for (int i = 0; i < 8; ++i) { L.comb[i].size = COMB_LEN[i]; R.comb[i].size = COMB_LEN[i] + STEREO_SPREAD; }
    for (int i = 0; i < 4; ++i) { L.ap[i].size   = AP_LEN[i];   R.ap[i].size   = AP_LEN[i]   + STEREO_SPREAD; }
    feedback_cur = feedback_tgt = OFFSET_ROOM + SCALE_ROOM * 0.7f;
    dampval_cur  = dampval_tgt  = SCALE_DAMP  * 0.3f;
    pre_cur = pre_tgt = 1.0f;  post_cur = post_tgt = 1.0f;
    ctl_coef = 0.05f;
}

void reverb_render(const float *inL, const float *inR,
                   float *outL,       float *outR, int frames) {
    feedback_cur += ctl_coef * (feedback_tgt - feedback_cur);
    dampval_cur  += ctl_coef * (dampval_tgt  - dampval_cur);
    pre_cur      += ctl_coef * (pre_tgt      - pre_cur);
    post_cur     += ctl_coef * (post_tgt     - post_cur);
    const float fb = feedback_cur, damp = dampval_cur, pre = pre_cur, post = post_cur;

    for (int n = 0; n < frames; ++n) {
        float xl = drive_shape(inL[n], pre, post) * FIXED_GAIN;
        float xr = drive_shape(inR[n], pre, post) * FIXED_GAIN;
        float yl = 0.0f, yr = 0.0f;
        for (int i = 0; i < 8; ++i) {
            yl += comb_process(&L.comb[i], xl, fb, damp);
            yr += comb_process(&R.comb[i], xr, fb, damp);
        }
        for (int i = 0; i < 4; ++i) {
            yl = ap_process(&L.ap[i], yl);
            yr = ap_process(&R.ap[i], yr);
        }
        outL[n] = yl; outR[n] = yr;
    }
}

#else
/* ===== LIQUID modulated FDN + diffusion (default, mode 1) =================
 *
 * The differences from Freeverb that matter audibly:
 *   1. Each comb reads at a FRACTIONAL position pos − lfo·depth (linear
 *      interpolation). Per-comb LFO rates are spread 0.50…1.44 Hz, depths
 *      6…10 samples. → tail detunes continuously, modes never park.
 *   2. Per-channel PRE-DELAY (≈25 ms) before the wash. → dry note speaks
 *      first, the room blooms behind it.
 *   3. Two MODULATED ALLPASS input diffusers (Greyhole-style). → smears
 *      input transients into the wash so the early reflections aren't
 *      audible as discrete echoes.
 *   4. The four output allpasses are also modulated (gentle 3.5-sample
 *      depth). → "liquid" character on the tail rather than fixed comb.
 *
 * Everything else (silent-in→silent-out, the API, the smoothed targets,
 * the drive shaper, FIXED_GAIN, AP_FEEDBACK) is identical. */

#define LQ_MAX_COMB    1800     /* room for the spread + the modulation depth */
#define LQ_MAX_AP      1024
#define LQ_PRE_DELAY   1100     /* ≈ 25 ms at 44.1 kHz */
#define LQ_AP_MOD      3.5f     /* output-AP modulation depth (samples) */
#define LQ_DIFF_MOD    2.5f     /* input-diffuser modulation depth (samples) */
static const int LQ_DIFF_LEN[2] = { 142, 379 };
static const float LQ_LFO_HZ[8] =
    { 0.50f,0.63f,0.78f,0.92f,1.07f,1.18f,1.31f,1.44f };

typedef struct {
    float buf[LQ_MAX_COMB]; int size, pos;
    float filterstore;
    float lfo, lfo_inc, depth;
} mcomb_t;

typedef struct {
    float buf[LQ_MAX_AP]; int size, pos;
    float lfo, lfo_inc, depth;
    float fb;
} mallp_t;

typedef struct {
    mcomb_t comb[8];
    mallp_t ap[4];
    mallp_t diff[2];
    float pre[LQ_PRE_DELAY]; int pre_pos;
} channel_t;

static channel_t L, R;

static void mallp_init(mallp_t *a, int size, float hz, float depth, float fb) {
    memset(a->buf, 0, sizeof a->buf);
    a->size = size; a->pos = 0;
    a->lfo  = 0.0f; a->lfo_inc = hz / (float)DSP_SAMPLE_RATE_HZ;
    a->depth = depth; a->fb = fb;
}

static void channel_init(channel_t *c, int spread) {
    memset(c->pre, 0, sizeof c->pre); c->pre_pos = 0;
    for (int i = 0; i < 8; ++i) {
        memset(c->comb[i].buf, 0, sizeof c->comb[i].buf);
        c->comb[i].size  = COMB_LEN[i] + spread;
        c->comb[i].pos   = 0;
        c->comb[i].filterstore = 0.0f;
        c->comb[i].lfo     = (float)i * 0.13f;                          /* phase spread */
        c->comb[i].lfo_inc = LQ_LFO_HZ[i] / (float)DSP_SAMPLE_RATE_HZ;
        c->comb[i].depth   = 6.0f + (float)i * 0.6f;                    /* 6..10 samples */
    }
    for (int i = 0; i < 4; ++i)
        mallp_init(&c->ap[i],   AP_LEN[i] + spread,
                   0.30f + 0.07f * (float)i, LQ_AP_MOD, AP_FEEDBACK);
    for (int i = 0; i < 2; ++i)
        mallp_init(&c->diff[i], LQ_DIFF_LEN[i] + spread,
                   0.20f + 0.05f * (float)i, LQ_DIFF_MOD, 0.62f);
}

/* Modulated comb: fractional read at (pos − lfo·depth) with linear interp. */
static inline float mcomb_process(mcomb_t *c, float in, float fb, float damp) {
    c->lfo += c->lfo_inc; if (c->lfo >= 1.0f) c->lfo -= 1.0f;
    float mod = (0.5f + 0.5f * sinf(c->lfo * 6.2831853f)) * c->depth;
    float rp  = (float)c->pos - mod;
    while (rp < 0.0f) rp += (float)c->size;
    int   i0 = (int)rp;
    int   i1 = (i0 + 1) % c->size;
    float fr = rp - (float)i0;
    float y  = c->buf[i0] * (1.0f - fr) + c->buf[i1] * fr;
    c->filterstore = y * (1.0f - damp) + c->filterstore * damp;
    c->buf[c->pos] = in + c->filterstore * fb;
    if (++c->pos >= c->size) c->pos = 0;
    return y;
}

/* Modulated Schroeder allpass: same y - in*fb output shape, fractional read. */
static inline float mallp_process(mallp_t *a, float in) {
    float rp;
    if (a->depth > 0.0f) {
        a->lfo += a->lfo_inc; if (a->lfo >= 1.0f) a->lfo -= 1.0f;
        float mod = (0.5f + 0.5f * sinf(a->lfo * 6.2831853f)) * a->depth;
        rp = (float)a->pos - mod;
        while (rp < 0.0f) rp += (float)a->size;
    } else {
        rp = (float)a->pos;
    }
    int   i0 = (int)rp;
    int   i1 = (i0 + 1) % a->size;
    float fr = rp - (float)i0;
    float y  = a->buf[i0] * (1.0f - fr) + a->buf[i1] * fr;
    a->buf[a->pos] = in + y * a->fb;
    if (++a->pos >= a->size) a->pos = 0;
    return y - in * a->fb;
}

static inline float channel_process(channel_t *c, float in, float fb, float damp) {
    /* pre-delay */
    float pd = c->pre[c->pre_pos];
    c->pre[c->pre_pos] = in;
    if (++c->pre_pos >= LQ_PRE_DELAY) c->pre_pos = 0;

    /* LIQUID stacks more gain than Freeverb (2 input diffusers + modulated
     * output AP), so the same FIXED_GAIN would push it past the engine test's
     * < 4.0 peak bound at size 0.9 / drive 0.5. Halve the inject level here;
     * outer wet_amp + reverb_set still feel the same to the user. */
    float x = pd * FIXED_GAIN * 0.45f;
    /* input diffusers (Greyhole-style) */
    x = mallp_process(&c->diff[0], x);
    x = mallp_process(&c->diff[1], x);
    /* parallel modulated combs */
    float y = 0.0f;
    for (int i = 0; i < 8; ++i) y += mcomb_process(&c->comb[i], x, fb, damp);
    /* series modulated output allpasses */
    for (int i = 0; i < 4; ++i) y = mallp_process(&c->ap[i], y);
    return y;
}

void reverb_init(void) {
    channel_init(&L, 0);
    channel_init(&R, STEREO_SPREAD);
    feedback_cur = feedback_tgt = OFFSET_ROOM + SCALE_ROOM * 0.7f;
    dampval_cur  = dampval_tgt  = SCALE_DAMP  * 0.3f;
    pre_cur = pre_tgt = 1.0f;  post_cur = post_tgt = 1.0f;
    ctl_coef = 0.05f;
}

void reverb_render(const float *inL, const float *inR,
                   float *outL,       float *outR, int frames) {
    feedback_cur += ctl_coef * (feedback_tgt - feedback_cur);
    dampval_cur  += ctl_coef * (dampval_tgt  - dampval_cur);
    pre_cur      += ctl_coef * (pre_tgt      - pre_cur);
    post_cur     += ctl_coef * (post_tgt     - post_cur);
    const float fb = feedback_cur, damp = dampval_cur, pre = pre_cur, post = post_cur;

    for (int n = 0; n < frames; ++n) {
        float xl = drive_shape(inL[n], pre, post);
        float xr = drive_shape(inR[n], pre, post);
        outL[n] = channel_process(&L, xl, fb, damp);
        outR[n] = channel_process(&R, xr, fb, damp);
    }
}

#endif  /* FAM_REVERB_MODE */

/* ===== Shared knobs ===================================================== */

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
