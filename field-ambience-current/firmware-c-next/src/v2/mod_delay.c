/*
 * mod_delay.c — modulated stereo delay.
 */

#include "v2/mod_delay.h"
#include "dsp.h"
#include <string.h>

#define MASK (MD_BUF - 1)
#define SR 44100.0f

void mod_delay_init(mod_delay_t *md) {
    memset(md, 0, sizeof *md);
    md->base_L_samples = 0.350f * SR;
    md->base_R_samples = 0.490f * SR;
    md->depth_samples  = 18.0f;
    md->fb             = 0.22f;
    md->wet            = 0.25f;
}

void mod_delay_set_amount(mod_delay_t *md, float blur) {
    if (blur < 0.0f) blur = 0.0f;
    if (blur > 1.0f) blur = 1.0f;
    /* Blur scales delay time, feedback, wet. */
    md->base_L_samples = (0.280f + 0.140f * blur) * SR;
    md->base_R_samples = (0.370f + 0.240f * blur) * SR;
    md->depth_samples  = 10.0f + 20.0f * blur;
    md->fb             = 0.15f + 0.20f * blur;
    md->wet            = 0.20f + 0.20f * blur;
}

/* Interpolated read for fractional sample delay. */
static inline float interp_read(const float *buf, int write, float delay_samples) {
    if (delay_samples < 1.0f) delay_samples = 1.0f;
    if (delay_samples > MASK - 1) delay_samples = MASK - 1;
    int   d_int = (int)delay_samples;
    float d_frac = delay_samples - (float)d_int;
    int r0 = (write - d_int - 1) & MASK;
    int r1 = (write - d_int    ) & MASK;
    return buf[r0] + d_frac * (buf[r1] - buf[r0]);
}

void mod_delay_process(mod_delay_t *md, float *L, float *R, int frames) {
    /* LFOs at 0.07 Hz (L) and 0.11 Hz (R) — incommensurate. */
    const float lfo_L_hz = 0.07f;
    const float lfo_R_hz = 0.11f;
    const float pinc_L = lfo_L_hz / SR;
    const float pinc_R = lfo_R_hz / SR;

    float depth = md->depth_samples;
    float fb    = md->fb;
    float wet   = md->wet;

    for (int i = 0; i < frames; ++i) {
        md->lfo_L_phase += pinc_L; if (md->lfo_L_phase >= 1.0f) md->lfo_L_phase -= 1.0f;
        md->lfo_R_phase += pinc_R; if (md->lfo_R_phase >= 1.0f) md->lfo_R_phase -= 1.0f;
        float modL = dsp_sin(md->lfo_L_phase) * depth;
        float modR = dsp_sin(md->lfo_R_phase) * depth;

        float dL = md->base_L_samples + modL;
        float dR = md->base_R_samples + modR;

        float yL = interp_read(md->bufL, md->write, dL);
        float yR = interp_read(md->bufR, md->write, dR);

        /* Cross-feed for ping-pong-ish smear. */
        float inL = L[i] + fb * yR * 0.6f;
        float inR = R[i] + fb * yL * 0.6f;

        md->bufL[md->write] = inL;
        md->bufR[md->write] = inR;
        md->write = (md->write + 1) & MASK;

        L[i] = L[i] + yL * wet;
        R[i] = R[i] + yR * wet;
    }
}
