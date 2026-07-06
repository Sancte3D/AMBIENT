/*
 * shimmer.c — octave-up regeneration around the hall. See shimmer.h.
 */

#include "shimmer.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

/* Ring + grain window. W = 2048 (~46 ms grains) is the classic sweet spot:
 * long enough to carry pitch, short enough to smear attacks into wash. */
#define SH_N     4096
#define SH_MASK  (SH_N - 1)
#define SH_W     2048.0f

static float rbL[SH_N], rbR[SH_N];
static int   wr;
static float ph;                    /* grain phase 0..SH_W (both taps)   */
static float g_cur, g_tgt;          /* smoothed loop gain                */
static float lpL, lpR;              /* darkening LP inside the loop      */
static float hpL_x1, hpL_y1, hpR_x1, hpR_y1;   /* mud guard (~180 Hz HP) */

void shimmer_init(void) {
    memset(rbL, 0, sizeof rbL);
    memset(rbR, 0, sizeof rbR);
    wr = 0;
    ph = 0.0f;
    g_cur = g_tgt = 0.0f;
    lpL = lpR = 0.0f;
    hpL_x1 = hpL_y1 = hpR_x1 = hpR_y1 = 0.0f;
}

void shimmer_set_amount(float v01) {
    if (v01 < 0.0f) v01 = 0.0f;
    if (v01 > 1.0f) v01 = 1.0f;
    /* Loop gain caps at 0.55: the reverb tail already decays < 1, so the
     * octave cascade always dies out — bloom, never runaway. Square curve:
     * the lower half of the slot is a hint, the top is the full halo. */
    g_tgt = v01 * v01 * 0.55f;
}

/* one grain tap: linear-interp read at (base - delay). `base` is the
 * PER-SAMPLE virtual write position (wr + n): the capture for this block
 * runs after the feed, so the write head must be advanced virtually or
 * the read rate collapses to 1× inside a block (= no pitch shift — the
 * first version had exactly that bug, caught by the octave Goertzel).
 * SH_GUARD keeps the read behind the truly-written region. */
#define SH_GUARD 260.0f
static inline float tap(const float *rb, float base, float delay) {
    float pos = base - delay - SH_GUARD;
    int   i0  = (int)floorf(pos);
    float fr  = pos - (float)i0;
    int   a   = i0 & SH_MASK;
    int   b   = (i0 + 1) & SH_MASK;
    return rb[a] + fr * (rb[b] - rb[a]);
}

void shimmer_feed_add(float *send_L, float *send_R, int frames) {
    g_cur += 0.05f * (g_tgt - g_cur);
    if (g_cur < 1.0e-4f && g_tgt < 1.0e-4f) return;   /* bit-exact off */
    const float g = g_cur;

    for (int n = 0; n < frames; ++n) {
        /* Dual-tap Doppler: delay shrinks SH_W → 0 at 1 sample/sample, so
         * the read head moves 2× through the material = +12 st. Second tap
         * offset half a window; triangular crossfade hides the resets. */
        ph += 1.0f;
        if (ph >= SH_W) ph -= SH_W;
        float p0 = ph;
        float p1 = ph + SH_W * 0.5f;
        if (p1 >= SH_W) p1 -= SH_W;
        /* power-complementary sine windows (sin² + cos² = 1): constant
         * output POWER across the crossfade — the triangular version
         * amplitude-modulated at the grain rate and smeared the octave
         * into sidebands (caught by the Goertzel test) */
        float w0 = dsp_sin(0.5f * p0 / SH_W);
        float w1 = dsp_sin(0.5f * p1 / SH_W);

        float base = (float)(wr + n);
        float sL = tap(rbL, base, SH_W - p0) * w0 + tap(rbL, base, SH_W - p1) * w1;
        float sR = tap(rbR, base, SH_W - p0) * w0 + tap(rbR, base, SH_W - p1) * w1;

        /* darken each generation (~4.3 kHz one-pole): higher octaves get
         * softer — silk, not glass dust */
        lpL += 0.48f * (sL - lpL);
        lpR += 0.48f * (sR - lpR);
        /* and keep mud out (~180 Hz one-pole HP) */
        float hL = lpL - hpL_x1 + 0.975f * hpL_y1; hpL_x1 = lpL; hpL_y1 = hL;
        float hR = lpR - hpR_x1 + 0.975f * hpR_y1; hpR_x1 = lpR; hpR_y1 = hR;

        send_L[n] += hL * g;
        send_R[n] += hR * g;
    }
}

void shimmer_capture(const float *wet_L, const float *wet_R, int frames) {
    if (g_cur < 1.0e-4f && g_tgt < 1.0e-4f) return;
    for (int n = 0; n < frames; ++n) {
        rbL[wr] = wet_L[n];
        rbR[wr] = wet_R[n];
        wr = (wr + 1) & SH_MASK;
    }
}
