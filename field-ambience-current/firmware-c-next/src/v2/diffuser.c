/*
 * diffuser.c — 4-stage allpass cascade per channel.
 *
 * Schroeder allpass:  y[n] = -fb*x[n] + x[n-d] + fb*y[n-d]
 * The all-pass property keeps the frequency response flat; only the phase
 * is smeared — exactly what "Blur" should do to attacks without dulling
 * the tone.
 */

#include "v2/diffuser.h"
#include <string.h>

#define MASK 1023

/* Sample delays @ 44.1 kHz: 7, 11, 17, 23 ms. */
static const int DELAY_BASE[DF_TAPS] = { 308, 485, 750, 1014 };
/* R offsets for Schroeder stereo spread. */
static const int DELAY_R_OFFSET[DF_TAPS] = { +3, +5, +7, +11 };

void diffuser_init(diffuser_t *d) {
    memset(d, 0, sizeof *d);
    for (int t = 0; t < DF_TAPS; ++t) {
        int dL = DELAY_BASE[t];
        int dR = DELAY_BASE[t] + DELAY_R_OFFSET[t];
        if (dL > MASK) dL = MASK;
        if (dR > MASK) dR = MASK;
        d->delayL[t] = dL;
        d->delayR[t] = dR;
    }
    d->fb = 0.45f;
}

void diffuser_set_amount(diffuser_t *d, float blur) {
    if (blur < 0.0f) blur = 0.0f;
    if (blur > 1.0f) blur = 1.0f;
    /* Map Blur 0..1 → fb 0.35..0.55. */
    d->fb = 0.35f + 0.20f * blur;
}

static inline float allpass_step(float *buf, int *w, int delay, float in, float fb) {
    int read = (*w - delay) & MASK;
    float delayed = buf[read];
    float y = -fb * in + delayed;
    buf[*w] = in + fb * y;
    *w = (*w + 1) & MASK;
    return y;
}

void diffuser_process(diffuser_t *d, float *L, float *R, int frames) {
    float fb = d->fb;
    for (int i = 0; i < frames; ++i) {
        float lx = L[i], rx = R[i];
        for (int t = 0; t < DF_TAPS; ++t) {
            lx = allpass_step(d->bufL[t], &d->writeL[t], d->delayL[t], lx, fb);
            rx = allpass_step(d->bufR[t], &d->writeR[t], d->delayR[t], rx, fb);
        }
        L[i] = lx;
        R[i] = rx;
    }
}
