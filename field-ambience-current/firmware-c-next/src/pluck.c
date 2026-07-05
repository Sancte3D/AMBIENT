/*
 * pluck.c — Karplus-Strong plucked-string voices. See pluck.h.
 */

#include "pluck.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR        ((float)DSP_SAMPLE_RATE_HZ)
#define BUF_LEN   1024                 /* > SR/PLUCK_MIN_HZ = 735 @44.1k */
#define T60_S     3.2f                 /* ring time, pitch-independent   */
#define VERB_SEND 0.50f                /* plucks bloom into the hall     */
#define ENV_EPS   2.5e-4f              /* ≈ −72 dBFS → voice retires     */

typedef struct {
    float buf[BUF_LEN];
    float N;              /* loop length in samples (fractional)  */
    int   widx;           /* write index                          */
    float rho;            /* per-sample loop gain for the T60     */
    float y_prev;         /* averaging-lowpass memory             */
    float env;            /* tracked peak envelope (for retiring) */
    float panL, panR;
    int   active;
    uint32_t age;         /* steal-the-oldest bookkeeping         */
} pluck_voice_t;

static pluck_voice_t v[PLUCK_VOICES];
static float    s_damp = 0.42f;   /* averaging-LP blend: 0=bright (macro) */
static int      next_voice;
static uint32_t age_counter;
static uint32_t burst_rng = 0x9E3779B9u;

static inline float burst_white(void) {
    burst_rng = burst_rng * 1664525u + 1013904223u;
    return (float)((int32_t)burst_rng) * (1.0f / 2147483648.0f);
}

void pluck_init(void) {
    memset(v, 0, sizeof v);
    /* Slight opposing pans — equal-power at ±0.35. */
    for (int i = 0; i < PLUCK_VOICES; ++i) {
        float p = (i == 0) ? -0.35f : 0.35f;
        float a = (p + 1.0f) * 0.25f * 3.14159265f;
        v[i].panL = cosf(a);
        v[i].panR = sinf(a);
    }
    next_voice  = 0;
    age_counter = 0;
    burst_rng   = 0x9E3779B9u;
    s_damp      = 0.42f;
}

void pluck_set_damp(float damp) {
    if (damp < 0.0f) damp = 0.0f;
    if (damp > 0.9f) damp = 0.9f;
    s_damp = damp;
}

void pluck_note(float freq_hz, float amp) {
    if (freq_hz < PLUCK_MIN_HZ) freq_hz = PLUCK_MIN_HZ;
    if (amp < 0.0f) amp = 0.0f;
    if (amp > 1.0f) amp = 1.0f;

    /* Round-robin, but steal the OLDEST if the preferred slot still rings
     * loudly (keeps overlapping sparkles from cutting each other hard). */
    int i = next_voice;
    if (v[i].active && v[i].env > 0.05f) {
        int oldest = 0;
        for (int k = 1; k < PLUCK_VOICES; ++k)
            if (v[k].age < v[oldest].age) oldest = k;
        i = oldest;
    }
    next_voice = (i + 1) % PLUCK_VOICES;

    pluck_voice_t *p = &v[i];
    p->N   = SR / freq_hz;
    if (p->N > (float)(BUF_LEN - 4)) p->N = (float)(BUF_LEN - 4);
    p->rho = powf(0.001f, 1.0f / (freq_hz * T60_S));   /* −60 dB in T60 */
    p->widx   = 0;
    p->y_prev = 0.0f;
    p->env    = amp;
    p->age    = ++age_counter;
    p->active = 1;

    /* Excitation: one loop-length of lowpassed noise (the classic KS burst;
     * the one-pole softens the attack from "snap" to "bell"). The write head
     * starts just PAST the burst so the fractional read (N behind the write
     * head) lands inside the fresh burst — starting it at 0 would overwrite
     * the excitation with silence before the read head ever got there. */
    int n = (int)p->N + 1;
    float lp = 0.0f;
    for (int k = 0; k < BUF_LEN; ++k) {
        if (k < n) {
            lp += 0.45f * (burst_white() - lp);
            p->buf[k] = lp * amp * 2.2f;   /* ≈ peak `amp` after the LP loss */
        } else {
            p->buf[k] = 0.0f;
        }
    }
    p->widx = n;
}

int pluck_active_count(void) {
    int c = 0;
    for (int i = 0; i < PLUCK_VOICES; ++i) c += v[i].active ? 1 : 0;
    return c;
}

void pluck_render_mix(float *dry_L, float *dry_R,
                      float *send_L, float *send_R, int frames) {
    for (int i = 0; i < PLUCK_VOICES; ++i) {
        pluck_voice_t *p = &v[i];
        if (!p->active) continue;

        float env_track = p->env;
        for (int n = 0; n < frames; ++n) {
            /* Fractional read `N` samples behind the write head. */
            float rpos = (float)p->widx - p->N;
            if (rpos < 0.0f) rpos += (float)BUF_LEN;
            int   r0 = (int)rpos;
            float fr = rpos - (float)r0;
            int   r1 = r0 + 1; if (r1 >= BUF_LEN) r1 = 0;
            float y  = p->buf[r0] + fr * (p->buf[r1] - p->buf[r0]);

            /* Damped feedback: averaging lowpass + T60 loop gain. */
            float fb = p->rho * ((1.0f - s_damp) * y + s_damp * p->y_prev);
            p->y_prev = y;
            p->buf[p->widx] = fb;
            if (++p->widx >= BUF_LEN) p->widx = 0;

            dry_L[n]  += y * p->panL;
            dry_R[n]  += y * p->panR;
            send_L[n] += y * p->panL * VERB_SEND;
            send_R[n] += y * p->panR * VERB_SEND;

            /* cheap peak tracker: instant up, slow down */
            float a = y < 0.0f ? -y : y;
            env_track = a > env_track ? a : env_track * 0.99995f;
        }
        p->env = env_track;
        if (p->env < ENV_EPS) p->active = 0;
    }
}
