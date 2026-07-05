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
#define FAM_REVERB_MODE 2   /* 0 = Freeverb (legacy), 1 = LIQUID, 2 = HALL */
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
#if FAM_REVERB_MODE != 2
static const int COMB_LEN[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
static const int AP_LEN  [4] = {  556,  441,  341,  225 };
#endif

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

#elif FAM_REVERB_MODE == 1
/* ===== LIQUID modulated FDN + diffusion (mode 1) =================
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

/* Modulated comb: fractional read `size − mod` samples back (linear interp).
 * r18.91 BUG FIX: this used to read at pos − mod, which is a sample only
 * `mod` (≈6–10) samples OLD — the feedback loop was 6–10 samples long
 * instead of ~1200, so the LIQUID tail collapsed instantly (measured:
 * tail RMS 0.000 at 1 s even at size 0.9). The mode-2 HALL replaces this
 * topology as default; the fix keeps the legacy mode honest for A/B. */
static inline float mcomb_process(mcomb_t *c, float in, float fb, float damp) {
    c->lfo += c->lfo_inc; if (c->lfo >= 1.0f) c->lfo -= 1.0f;
    float mod = (0.5f + 0.5f * sinf(c->lfo * 6.2831853f)) * c->depth;
    float rp  = (float)c->pos + 1.0f + mod;            /* age = size − mod */
    while (rp >= (float)c->size) rp -= (float)c->size;
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

#else
/* ===== HALL — cross-coupled figure-8 tank (default, mode 2, r18.91) =======
 *
 * Topology learned from Jon Dattorro's 1997 "Effect Design Part 1" plate
 * (the published Lexicon-school reference; topology + published paper, no
 * code copied). Why this beats the LIQUID mode for OUR identity:
 *
 *   1. ONE ROOM, not two. LIQUID ran two fully independent channels with a
 *      sample spread — decorrelated, but never a shared space. The Dattorro
 *      tank is a single figure-8: branch A feeds branch B and vice versa,
 *      and BOTH ears tap BOTH branches → the tail images as one deep room
 *      behind the instrument (the "cinematic" depth the SOUND_WORLD asks
 *      for), not as width glued onto a mono wash.
 *   2. Diffusion FIRST (4 series input allpasses after a bandwidth lowpass)
 *      → a note dissolves into the room without discrete early echoes.
 *   3. Two slowly MODULATED tank allpasses keep the modes moving so the
 *      tail never rings metallic — same job as LIQUID's 16 comb LFOs at a
 *      fraction of the cost. Rates are OUR drift language (0.101/0.127 Hz,
 *      incommensurate), not the paper's ~1 Hz wobble.
 *
 * All lengths are this project's own: the paper's 29.761 kHz relations
 * scaled to 44.1 kHz and nudged to nearby coprime values, tap positions
 * re-picked by the same rule. Memory ≈ 136 KB (< LIQUID's 166 KB); CPU
 * ≈ half of LIQUID (2 fractional reads/sample instead of 28). */

#define HALL_PRE_DELAY  1060            /* ≈ 24 ms                        */
#define HALL_MOD_DEPTH  12.0f           /* tank-AP excursion (samples)    */

/* series input diffusers: {length, gain} */
static const int   HDIF_LEN[4] = { 211, 157, 563, 409 };
static const float HDIF_G  [4] = { 0.75f, 0.75f, 0.625f, 0.625f };

/* tank line lengths (samples) — branch A / branch B */
#define HA_MAP  997                     /* modulated AP, g = -0.70        */
#define HA_D1   6599
#define HA_AP   2663
#define HA_D2   5507
#define HB_MAP  1343
#define HB_D1   6247
#define HB_AP   3931
#define HB_D2   4683

typedef struct { float buf[600]; int size, pos; float g; } hap_t;   /* diffusers */

typedef struct {
    /* branch memory */
    float map[1400]; int map_size, map_pos;        /* modulated allpass  */
    float d1[6700];  int d1_size,  d1_pos;
    float ap[4000];  int ap_size,  ap_pos;
    float d2[5600];  int d2_size,  d2_pos;
    float lp;                                       /* damping state      */
    float lfo, lfo_inc;
} hbranch_t;

static struct {
    float pre[HALL_PRE_DELAY]; int pre_pos;
    float bw_lp;                                    /* input bandwidth    */
    hap_t dif[4];
    hbranch_t A, B;
    float fbA, fbB;                                 /* cross-couple state */
} H;

/* fixed integer output taps (samples into each line, < line length) */
static const int TAP_B_D1a = 397,  TAP_B_D1b = 5320, TAP_B_AP = 1770,
                 TAP_B_D2  = 2946, TAP_A_D1  = 2831, TAP_A_AP = 496,
                 TAP_A_D2  = 1213;
static const int TAP_A_D1a = 523,  TAP_A_D1b = 5479, TAP_A_APx = 1926,
                 TAP_A_D2x = 3547, TAP_B_D1x = 2643, TAP_B_APx = 350,
                 TAP_B_D2x = 1081;

static inline float hap_process(hap_t *a, float x) {
    float y = a->buf[a->pos];
    float v = x - a->g * y;                 /* lattice allpass            */
    a->buf[a->pos] = v;
    if (++a->pos >= a->size) a->pos = 0;
    return y + a->g * v;
}

static inline float hdelay_tap(const float *buf, int size, int pos, int tap) {
    int i = pos - tap; if (i < 0) i += size;
    return buf[i];
}

static void hbranch_init(hbranch_t *b, int map_size, int d1, int ap, int d2,
                         float lfo_hz, float lfo_phase) {
    memset(b, 0, sizeof *b);
    b->map_size = map_size; b->d1_size = d1; b->ap_size = ap; b->d2_size = d2;
    b->lfo = lfo_phase;
    b->lfo_inc = lfo_hz / (float)DSP_SAMPLE_RATE_HZ;
}

/* one branch step: in → modulated AP → d1 →(tap for fb: LP → decay)→ AP → d2.
 * Returns the OTHER branch's next feedback (this branch's d2 tail × decay). */
static inline float hbranch_process(hbranch_t *b, float x,
                                    float decay, float damp) {
    /* modulated allpass (g = -0.70): fractional read, our drift rates */
    b->lfo += b->lfo_inc; if (b->lfo >= 1.0f) b->lfo -= 1.0f;
    float mod = (0.5f + 0.5f * dsp_sin(b->lfo)) * HALL_MOD_DEPTH;
    float rp  = (float)b->map_pos - 1.0f - mod;
    while (rp < 0.0f) rp += (float)b->map_size;
    int   i0 = (int)rp, i1 = i0 + 1; if (i1 >= b->map_size) i1 = 0;
    float fr = rp - (float)i0;
    float my = b->map[i0] + fr * (b->map[i1] - b->map[i0]);
    const float mg = -0.70f;
    float mv = x - mg * my;
    b->map[b->map_pos] = mv;
    if (++b->map_pos >= b->map_size) b->map_pos = 0;
    float v = my + mg * mv;

    /* long delay 1 */
    float d1y = b->d1[b->d1_pos];
    b->d1[b->d1_pos] = v;
    if (++b->d1_pos >= b->d1_size) b->d1_pos = 0;

    /* damping lowpass + decay into the second half */
    b->lp += (1.0f - damp) * (d1y - b->lp);
    float w = b->lp * decay;

    /* plain allpass (g = 0.50) */
    float ay = b->ap[b->ap_pos];
    const float ag = 0.50f;
    float av = w - ag * ay;
    b->ap[b->ap_pos] = av;
    if (++b->ap_pos >= b->ap_size) b->ap_pos = 0;
    float u = ay + ag * av;

    /* long delay 2 → feedback for the other branch */
    float d2y = b->d2[b->d2_pos];
    b->d2[b->d2_pos] = u;
    if (++b->d2_pos >= b->d2_size) b->d2_pos = 0;
    return d2y * decay;
}

void reverb_init(void) {
    memset(&H, 0, sizeof H);
    for (int i = 0; i < 4; ++i) {
        H.dif[i].size = HDIF_LEN[i]; H.dif[i].pos = 0; H.dif[i].g = HDIF_G[i];
        memset(H.dif[i].buf, 0, sizeof H.dif[i].buf);
    }
    hbranch_init(&H.A, HA_MAP, HA_D1, HA_AP, HA_D2, 0.101f, 0.00f);
    hbranch_init(&H.B, HB_MAP, HB_D1, HB_AP, HB_D2, 0.127f, 0.41f);
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
    /* map the legacy feedback/damp ranges onto tank decay / damping:
     * feedback 0.70..0.98 → decay 0.58..0.97; dampval 0..0.4 → damp 0..0.62 */
    const float decay = dsp_clampf(0.58f + (feedback_cur - OFFSET_ROOM) *
                                   (0.39f / SCALE_ROOM), 0.0f, 0.97f);
    const float damp  = dsp_clampf(dampval_cur * (0.62f / SCALE_DAMP), 0.0f, 0.95f);
    const float bw    = 1.0f - 0.05f - damp * 0.20f;   /* input bandwidth  */
    const float pre = pre_cur, post = post_cur;

    for (int n = 0; n < frames; ++n) {
        /* mono drive-shaped sum into the tank (stereo comes from the taps) */
        float x = drive_shape(0.5f * (inL[n] + inR[n]), pre, post) * 0.30f;

        /* pre-delay */
        float pd = H.pre[H.pre_pos];
        H.pre[H.pre_pos] = x;
        if (++H.pre_pos >= HALL_PRE_DELAY) H.pre_pos = 0;

        /* input bandwidth + series diffusion */
        H.bw_lp += bw * (pd - H.bw_lp);
        float d = H.bw_lp;
        for (int i = 0; i < 4; ++i) d = hap_process(&H.dif[i], d);

        /* figure-8 tank: each branch is fed by the other's tail */
        float nfbA = hbranch_process(&H.A, d + H.fbB, decay, damp);
        float nfbB = hbranch_process(&H.B, d + H.fbA, decay, damp);
        H.fbA = nfbA; H.fbB = nfbB;

        /* distributed stereo taps — both ears hear both branches */
        float yl = hdelay_tap(H.B.d1, H.B.d1_size, H.B.d1_pos, TAP_B_D1a)
                 + hdelay_tap(H.B.d1, H.B.d1_size, H.B.d1_pos, TAP_B_D1b)
                 - hdelay_tap(H.B.ap, H.B.ap_size, H.B.ap_pos, TAP_B_AP)
                 + hdelay_tap(H.B.d2, H.B.d2_size, H.B.d2_pos, TAP_B_D2)
                 - hdelay_tap(H.A.d1, H.A.d1_size, H.A.d1_pos, TAP_A_D1)
                 - hdelay_tap(H.A.ap, H.A.ap_size, H.A.ap_pos, TAP_A_AP)
                 - hdelay_tap(H.A.d2, H.A.d2_size, H.A.d2_pos, TAP_A_D2);
        float yr = hdelay_tap(H.A.d1, H.A.d1_size, H.A.d1_pos, TAP_A_D1a)
                 + hdelay_tap(H.A.d1, H.A.d1_size, H.A.d1_pos, TAP_A_D1b)
                 - hdelay_tap(H.A.ap, H.A.ap_size, H.A.ap_pos, TAP_A_APx)
                 + hdelay_tap(H.A.d2, H.A.d2_size, H.A.d2_pos, TAP_A_D2x)
                 - hdelay_tap(H.B.d1, H.B.d1_size, H.B.d1_pos, TAP_B_D1x)
                 - hdelay_tap(H.B.ap, H.B.ap_size, H.B.ap_pos, TAP_B_APx)
                 - hdelay_tap(H.B.d2, H.B.d2_size, H.B.d2_pos, TAP_B_D2x);
        outL[n] = yl * 0.9f;
        outR[n] = yr * 0.9f;
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
