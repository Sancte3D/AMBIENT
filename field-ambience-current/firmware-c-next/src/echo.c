/*
 * echo.c — tape-style stereo delay (Reddit Echo macro).
 *
 * Circular-buffer delay-line per channel + 1-pole lowpass in the feedback
 * loop (the lowpass is what makes each repeat darker → tape character).
 * Design referenced from:
 *   - Signalsmith Audio "reverb-example-code" (MIT-ish, delay-line +
 *     mix-matrix patterns)
 *   - SuperCollider CombC + LPF algorithm (GPL3 — algorithm only, not
 *     linked)
 *   - DaisySP Tone (1-pole LPF) and DelayLine (MIT — concept, not linked)
 * Implementation is from-scratch C, no external dep, libm-free.
 *
 * The user-facing macro 0..1 maps to a meaningful slice of the
 * (time, feedback, wet, tone) 4-D space — at echo=0 the module short-
 * circuits to silent; echo=0.5 lands on a clean musical echo; echo=1.0
 * pushes into ambient-wash territory.
 */

#include "echo.h"
#include "dsp.h"
#include <stdint.h>
#include <string.h>

#define SR              ((float)DSP_SAMPLE_RATE_HZ)
#define ECHO_MAX_TIME_S 0.6f
/* Integer-only so the buffer size is a real compile-time constant: 0.6 s
 * × 44100 = 26460 + 1 guard sample. */
#define ECHO_MAX_SAMPS  ((6 * DSP_SAMPLE_RATE_HZ) / 10 + 1)
#define SILENCE_EPS     1.0e-5f
#define SMOOTH_COEF     0.04f      /* per-block one-pole on macro target */

static float bufL[ECHO_MAX_SAMPS];
static float bufR[ECHO_MAX_SAMPS];
static int   widx = 0;

/* Live (smoothed) internal params */
static float time_samps;   /* delay in samples */
static float fbk;          /* 0..0.85 */
static float wet;          /* 0..0.75 */
static float lp_coef;      /* tone → one-pole coef in feedback */
static float lp_zL = 0.0f, lp_zR = 0.0f;

/* Macro target & current */
static float amount_cur = 0.0f, amount_tgt = 0.0f;

/* Map the macro to the four internal params. Hand-tuned to feel musical:
 *   echo=0      → fb=0, wet=0
 *   echo=0.3    → 250 ms, fb=0.30, wet=0.20, tone=0.55
 *   echo=0.7    → 450 ms, fb=0.60, wet=0.45, tone=0.35
 *   echo=1.0    → 580 ms, fb=0.85, wet=0.65, tone=0.25 */
static void recompute_internals(float a) {
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    float t_s = 0.10f + a * 0.48f;                /* 100..580 ms */
    time_samps = t_s * SR;
    fbk        = a * 0.85f;
    wet        = a * 0.65f;
    /* tone: bright at small echo, dark at big echo (more "tape" feel) */
    float tone = 0.60f - a * 0.35f;               /* 0.60..0.25 */
    /* one-pole LP coef from a normalised "tone" 0..1.
     * coef = exp(-2π·fc/SR); we approximate libm-free via a tuned curve:
     * tone=0   → fc≈500 Hz   → coef≈0.93
     * tone=1   → fc≈8 kHz   → coef≈0.32
     * We just lerp the coef directly — sounds the same to a user. */
    lp_coef = 0.93f - tone * (0.93f - 0.32f);
}

void echo_init(void) {
    memset(bufL, 0, sizeof bufL);
    memset(bufR, 0, sizeof bufR);
    widx = 0;
    amount_cur = amount_tgt = 0.0f;
    lp_zL = lp_zR = 0.0f;
    recompute_internals(0.0f);
}

void echo_set_amount(float a) {
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    amount_tgt = a;
}

void echo_render_mix(float *dry_L, float *dry_R,
                     float *send_L, float *send_R,
                     int frames, float send_amount) {
    /* Smooth the macro at block rate, then recompute internals from it.
     * Single source of truth (amount_cur → all four params) means time +
     * fbk + wet + tone glide together, no zipper, no fight. */
    amount_cur += SMOOTH_COEF * (amount_tgt - amount_cur);
    if (amount_cur < SILENCE_EPS && amount_tgt < SILENCE_EPS) return;
    recompute_internals(amount_cur);

    /* Integer + fractional split of the delay (for cheap linear interp). */
    int t_int = (int)time_samps;
    float t_frac = time_samps - (float)t_int;
    if (t_int < 1) t_int = 1;
    if (t_int >= ECHO_MAX_SAMPS - 1) t_int = ECHO_MAX_SAMPS - 2;

    for (int n = 0; n < frames; ++n) {
        /* Read delayed sample (lin-interp between buf[ri] and buf[ri-1]). */
        int riA = widx - t_int;     if (riA < 0) riA += ECHO_MAX_SAMPS;
        int riB = riA - 1;          if (riB < 0) riB += ECHO_MAX_SAMPS;
        float dL = bufL[riA] + (bufL[riB] - bufL[riA]) * t_frac;
        float dR = bufR[riA] + (bufR[riB] - bufR[riA]) * t_frac;

        /* Lowpass the feedback (tape darkness). */
        lp_zL += (1.0f - lp_coef) * (dL - lp_zL);
        lp_zR += (1.0f - lp_coef) * (dR - lp_zR);

        /* Write input + feedback*delayed-filtered back to buffer. */
        float inL = dry_L[n], inR = dry_R[n];
        bufL[widx] = inL + lp_zL * fbk;
        bufR[widx] = inR + lp_zR * fbk;

        /* Mix the wet (un-filtered, the filter is only in the feedback path,
         * so the FIRST echo is bright and the TAIL gets dark). */
        float wL = dL * wet, wR = dR * wet;
        dry_L[n]  += wL;
        dry_R[n]  += wR;
        send_L[n] += wL * send_amount;
        send_R[n] += wR * send_amount;

        if (++widx >= ECHO_MAX_SAMPS) widx = 0;
    }
}
