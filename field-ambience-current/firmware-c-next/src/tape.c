/*
 * tape.c — hiss + warm-tanh saturation (ADR-0017 Phase 3).
 *
 * Lifted verbatim from tools/render_dreamy_warm.c (the "DAS IST ES"
 * reference render). The two stages together give the device a
 * consistent tape/vinyl character; see tape.h for the design notes.
 */

#include "tape.h"
#include "dsp.h"
#include <math.h>
#include <stdint.h>

/* --- hiss state — decorrelated L/R white-noise LCGs ---------------------- */

static uint32_t hr_L = 0xC0FFEE11u;
static uint32_t hr_R = 0xDEADBEEFu;
/* r18.97 (user: "hardcore am rauschen, alles stirbt"): the old default
 * 0.005 sat only ~23 dB under the music once the r18.92 ducking opened
 * under program — a constant white carpet. Spectral forensics: the
 * valley floor between the bed's partials was FLAT at the hiss level
 * from 500 Hz to 12 kHz. New default −63 dBFS full-scale; AGE raises it
 * with a square curve (character detail, never a carpet). */
static float    hiss_amp = 0.0012f;

/* r18.92 program-follower ducking (user: constant floor too present).
 * Real tape hiss sits UNDER program material; in silence a good machine
 * reads as quiet, not as a hiss demo. engine_render feeds the dry-bus
 * block peak in; hiss follows it fast up (~1 block) and slow down
 * (~600 ms), floored so the character never fully disappears. */
static float duck_env  = 0.0f;
#define DUCK_DOWN_COEF 0.010f            /* per block ≈ 600 ms release   */
#define DUCK_REF       0.02f             /* program peak for full level  */
#define HISS_FLOOR     0.30f             /* −10.5 dB in silence          */
#define CRACKLE_FLOOR  0.50f             /*  −6.0 dB in silence          */

void tape_set_program_level(float block_peak) {
    if (block_peak < 0.0f) block_peak = 0.0f;
    if (block_peak > duck_env) duck_env = block_peak;          /* fast up */
    else duck_env += DUCK_DOWN_COEF * (block_peak - duck_env); /* slow dn */
}

static inline float duck_gain(float floor_gain) {
    float d = duck_env * (1.0f / DUCK_REF);
    if (d > 1.0f) d = 1.0f;
    return floor_gain + (1.0f - floor_gain) * d;
}

static inline float hn(uint32_t *r) {
    *r = (*r) * 1664525u + 1013904223u;
    return (float)((int32_t)*r) * (1.0f / 2147483648.0f);
}

/* --- saturation state --------------------------------------------------- */

static float sat_drive = 1.10f;          /* subtle warmth */

/* --- r18.89: vinyl crackle — dust impulses ringing a resonant bandpass ----
 * Learned from the SC Dust→Ringz vinyl-noise idiom: sparse random impulses
 * excite a high-Q bandpass around 2.6 kHz → each hit becomes a tiny damped
 * "tick" instead of a click. Plus an ULTRA-quiet chaotic crackle base
 * (logistic map) underneath, so the ticks sit on a faint fry rather than
 * digital silence. Both scale with the AGE macro (engine_set_age). */
static dsp_dust_t    crackle_dust[2];
static dsp_svf_t     crackle_bp[2];
static dsp_crackle_t crackle_base;
static float         crackle_amp = 0.0f;   /* 0 = off (default) */

/* --- API ---------------------------------------------------------------- */

void tape_init(void) {
    hr_L      = 0xC0FFEE11u;
    hr_R      = 0xDEADBEEFu;
    hiss_amp  = 0.0012f;
    sat_drive = 1.10f;
    dsp_dust_seed(&crackle_dust[0], 0x7EA7A61Eu);
    dsp_dust_seed(&crackle_dust[1], 0x51DEB00Bu);      /* decorrelated L/R */
    for (int c = 0; c < 2; ++c) {
        dsp_svf_reset(&crackle_bp[c]);
        dsp_svf_set(&crackle_bp[c], 2600.0f, 2.2f);    /* ticky, not clicky */
    }
    dsp_crackle_init(&crackle_base, 1.85f);
    crackle_amp = 0.0f;
    duck_env    = 0.0f;
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

void tape_set_crackle(float v01) {
    if (v01 < 0.0f) v01 = 0.0f;
    if (v01 > 1.0f) v01 = 1.0f;
    /* density 0..~9 ticks/s per channel; level curve squared so the low end
     * of AGE stays nearly clean and the top is clearly "old record". */
    dsp_dust_set_density(&crackle_dust[0], v01 * 9.0f);
    dsp_dust_set_density(&crackle_dust[1], v01 * 7.0f);
    crackle_amp = v01 * v01 * 0.055f;
}

void tape_hiss_render_add(float *L, float *R, int frames) {
    if (hiss_amp <= 0.0f) return;
    const float a = hiss_amp * duck_gain(HISS_FLOOR);
    for (int i = 0; i < frames; ++i) {
        L[i] += hn(&hr_L) * a;
        R[i] += hn(&hr_R) * a;
    }
}

void tape_crackle_render_add(float *L, float *R, int frames) {
    if (crackle_amp <= 0.0f) return;
    const float g = duck_gain(CRACKLE_FLOOR);
    for (int i = 0; i < frames; ++i) {
        float base = dsp_crackle(&crackle_base) * 0.06f;   /* faint fry bed */
        L[i] += (dsp_svf_bp(&crackle_bp[0], dsp_dust(&crackle_dust[0])) + base)
                * crackle_amp * g;
        R[i] += (dsp_svf_bp(&crackle_bp[1], dsp_dust(&crackle_dust[1])) + base)
                * crackle_amp * g;
    }
}

void tape_saturation_process(float *L, float *R, int frames) {
    const float d = sat_drive;
    for (int i = 0; i < frames; ++i) {
        L[i] = tanhf(L[i] * d) * 0.78f;
        R[i] = tanhf(R[i] * d) * 0.78f;
    }
}
