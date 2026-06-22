/*
 * famTexture — Step 10. Always-on brown-noise ambient bed.
 *
 * Port of the webapp `ensureTexture`. Per channel: brown noise feeds a
 * lowpassed "rumble" and a band-passed, slowly-swept "breath"; their mix is
 * warmed by a 4.5 kHz lowpass and scaled by a slow amplitude. L and R use
 * independent noise streams for a wide stereo bed; the two slow LFOs (BP
 * centre sweep, breath amplitude pulse) and the filter coefficients are
 * shared and updated at control rate.
 */

#include "texture.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR            ((float)DSP_SAMPLE_RATE_HZ)
#define CTL_DECIMATE  16

#define AMOUNT_SCALE  0.12f       /* amount 0..1 → bed amp 0..0.12 (webapp) */
#define SILENCE_EPS   1.0e-5f

/* One brown-noise generator: leaky integrator of white noise (matches the
 * webapp's `last = (last + 0.02*white)/1.02; out = last*3.5`). */
typedef struct {
    uint32_t lcg;
    float    last;
} brown_t;

static brown_t   brown[2];           /* L, R independent streams */
static dsp_svf_t rumbleLP[2];        /* lowpass 220 Hz   */
static dsp_svf_t breathBP[2];        /* bandpass 600 Hz, swept */
static dsp_svf_t warmLP[2];          /* lowpass 4500 Hz  */
/* Tier A #5 — independent white noise + HP at ~3 kHz for the missing "air"
 * band in the dreamy/PS2 mix. Runs at 0.18× the bed level — present, not
 * loud (RMS roughly −44 dBFS at amount=1.0, well under the breath body). */
static dsp_svf_t airHP[2];           /* highpass 3000 Hz */
typedef struct { uint32_t lcg; } air_t;
static air_t airN[2];
static inline float air_white(air_t *a){
    a->lcg = a->lcg * 1664525u + 1013904223u;
    return (float)((int32_t)a->lcg) / 2147483648.0f;
}

static float sweep_phase;            /* 0.052 Hz — BP centre sweep */
static float breath_phase;           /* 0.04 Hz  — breath amp pulse */
static float breath_gain;            /* current breath gain (ctl-rate) */

static float amp_cur, amp_tgt;       /* bed amplitude (smoothed) */
static float amp_coef;               /* per-sample one-pole toward amp_tgt */
static int   ctl_phase;

static inline float brown_next(brown_t *b) {
    b->lcg = b->lcg * 1664525u + 1013904223u;
    float white = (float)((int32_t)b->lcg) / 2147483648.0f;   /* −1..1 */
    b->last = (b->last + 0.02f * white) / 1.02f;
    return b->last * 3.5f;
}

void texture_init(void) {
    brown[0].lcg = 0x1234567u; brown[0].last = 0.0f;
    brown[1].lcg = 0x89abcdefu; brown[1].last = 0.0f;     /* different seed → decorrelated */
    for (int c = 0; c < 2; ++c) {
        dsp_svf_reset(&rumbleLP[c]); dsp_svf_set(&rumbleLP[c], 220.0f, 0.707f);
        dsp_svf_reset(&breathBP[c]); dsp_svf_set(&breathBP[c], 600.0f, 1.6f);
        dsp_svf_reset(&warmLP[c]);   dsp_svf_set(&warmLP[c],   4500.0f, 0.707f);
        dsp_svf_reset(&airHP[c]);    dsp_svf_set(&airHP[c],    3000.0f, 0.707f);
    }
    airN[0].lcg = 0xBEEFFACEu; airN[1].lcg = 0xFACECAFEu;   /* decorrelated */
    sweep_phase = 0.0f;
    breath_phase = 0.0f;
    breath_gain = 0.5f;
    amp_cur = amp_tgt = 0.0f;          /* boots silent (SPEC §8) */
    /* ~2 s glide so the bed blooms in rather than snapping. */
    amp_coef = 1.0f - expf(-1.0f / (2.0f * SR));
    ctl_phase = 0;
}

void texture_set_amount(float amount_0_1) {
    amount_0_1 = dsp_clampf(amount_0_1, 0.0f, 1.0f);
    amp_tgt = amount_0_1 * AMOUNT_SCALE;
}

/* Control-rate: advance the two LFOs, re-aim the breath bandpass centre and
 * the breath amplitude. */
static void control_update(void) {
    sweep_phase  += (0.052f / SR) * (float)CTL_DECIMATE;
    if (sweep_phase  >= 1.0f) sweep_phase  -= 1.0f;
    breath_phase += (0.040f / SR) * (float)CTL_DECIMATE;
    if (breath_phase >= 1.0f) breath_phase -= 1.0f;

    float centre = 600.0f + dsp_sin(sweep_phase) * 260.0f;
    if (centre < 80.0f) centre = 80.0f;
    for (int c = 0; c < 2; ++c) dsp_svf_set(&breathBP[c], centre, 1.6f);

    breath_gain = 0.5f + dsp_sin(breath_phase) * 0.22f;
}

void texture_render_mix(float *dry_L, float *dry_R,
                        float *send_L, float *send_R,
                        int frames, float send_amount) {
    /* Skip entirely while the bed is effectively silent (amount 0). */
    if (amp_cur < SILENCE_EPS && amp_tgt < SILENCE_EPS) return;

    for (int n = 0; n < frames; ++n) {
        if (ctl_phase == 0) control_update();

        amp_cur += amp_coef * (amp_tgt - amp_cur);

        for (int c = 0; c < 2; ++c) {
            float nz = brown_next(&brown[c]);
            /* Tier A #4 — body weight dropped 0.35 → 0.10: removes the
             * "Brumm" the user heard at amount ≥ 0.12, keeps a hint of low
             * warmth so the bed isn't only breath. */
            float rumble = dsp_svf_lp(&rumbleLP[c], nz) * 0.10f;
            float breath = dsp_svf_bp(&breathBP[c], nz) * breath_gain;
            float warm   = dsp_svf_lp(&warmLP[c], rumble + breath);
            /* Tier A #5 — air band, parallel (NOT through warmLP). */
            float air    = dsp_svf_hp(&airHP[c], air_white(&airN[c])) * 0.18f;
            float out    = (warm + air) * amp_cur;

            if (c == 0) { dry_L[n] += out; send_L[n] += out * send_amount; }
            else        { dry_R[n] += out; send_R[n] += out * send_amount; }
        }

        if (++ctl_phase >= CTL_DECIMATE) ctl_phase = 0;
    }
}
