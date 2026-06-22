/*
 * tape.c — hiss + warm-tanh saturation (ADR-0017 Phase 3).
 *
 * Lifted verbatim from tools/render_dreamy_warm.c (the "DAS IST ES"
 * reference render). The two stages together give the device a
 * consistent tape/vinyl character; see tape.h for the design notes.
 */

#include "tape.h"
#include <math.h>
#include <stdint.h>

/* --- hiss state — decorrelated L/R white-noise LCGs ---------------------- */

static uint32_t hr_L = 0xC0FFEE11u;
static uint32_t hr_R = 0xDEADBEEFu;
static float    hiss_amp = 0.005f;       /* ≈ −46 dBFS default */

static inline float hn(uint32_t *r) {
    *r = (*r) * 1664525u + 1013904223u;
    return (float)((int32_t)*r) * (1.0f / 2147483648.0f);
}

/* --- saturation state --------------------------------------------------- */

static float sat_drive = 1.10f;          /* subtle warmth */

/* --- API ---------------------------------------------------------------- */

void tape_init(void) {
    hr_L      = 0xC0FFEE11u;
    hr_R      = 0xDEADBEEFu;
    hiss_amp  = 0.005f;
    sat_drive = 1.10f;
}

void tape_set_hiss_amount(float amp) {
    if (amp < 0.0f) amp = 0.0f;
    if (amp > 1.0f) amp = 1.0f;
    hiss_amp = amp;
}

void tape_set_saturation_drive(float d) {
    if (d < 0.1f)  d = 0.1f;
    if (d > 4.0f)  d = 4.0f;
    sat_drive = d;
}

void tape_hiss_render_add(float *L, float *R, int frames) {
    if (hiss_amp <= 0.0f) return;
    const float a = hiss_amp;
    for (int i = 0; i < frames; ++i) {
        L[i] += hn(&hr_L) * a;
        R[i] += hn(&hr_R) * a;
    }
}

void tape_saturation_process(float *L, float *R, int frames) {
    const float d = sat_drive;
    for (int i = 0; i < frames; ++i) {
        L[i] = tanhf(L[i] * d) * 0.78f;
        R[i] = tanhf(R[i] * d) * 0.78f;
    }
}
