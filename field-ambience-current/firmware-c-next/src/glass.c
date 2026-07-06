/*
 * glass.c — 2-op FM glass voice. See glass.h for the model notes.
 */

#include "glass.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR ((float)DSP_SAMPLE_RATE_HZ)

/* Inharmonic modulator ratio. 3.5307 puts the strongest sidebands between
 * the harmonic series — the classic FM bell/glass region (an integer ratio
 * here would sound like an organ, not glass). */
#define GLASS_RATIO   3.5307f
#define GLASS_T60_S   2.5f      /* amplitude ring time                    */
#define GLASS_IDX_TAU 0.13f     /* mod-index decay — the "strike"         */
#define GLASS_SEND    0.5f      /* same hall send as the string plucks    */

typedef struct {
    float ph_c, ph_m;           /* carrier / modulator phase, turns 0..1  */
    float inc_c, inc_m;
    float amp;                  /* strike level                           */
    float env;                  /* amplitude envelope 1 → 0               */
    float env_coef;             /* per-sample decay for GLASS_T60_S       */
    float idx;                  /* index envelope (in TURNS of phase)     */
    float idx_coef;
    int   active;
    int   right;                /* stereo seat: 0 = L-ish, 1 = R-ish      */
    uint32_t age;               /* for oldest-voice stealing              */
} glass_voice_t;

static glass_voice_t gv[GLASS_VOICES];
static uint32_t g_stamp;
static int      g_next_seat;

void glass_init(void) {
    memset(gv, 0, sizeof gv);
    g_stamp = 0;
    g_next_seat = 0;
}

void glass_note(float freq_hz, float amp) {
    if (freq_hz < 40.0f) freq_hz = 40.0f;
    if (amp < 0.0f) amp = 0.0f;
    if (amp > 1.0f) amp = 1.0f;

    /* pick a free voice, else the oldest */
    int pick = -1;
    for (int i = 0; i < GLASS_VOICES; ++i)
        if (!gv[i].active) { pick = i; break; }
    if (pick < 0) {
        uint32_t oldest = 0xFFFFFFFFu;
        for (int i = 0; i < GLASS_VOICES; ++i)
            if (gv[i].age < oldest) { oldest = gv[i].age; pick = i; }
    }

    glass_voice_t *v = &gv[pick];
    v->ph_c  = 0.0f;
    v->ph_m  = 0.0f;
    v->inc_c = freq_hz / SR;
    v->inc_m = freq_hz * GLASS_RATIO / SR;
    v->amp   = amp;
    v->env   = 1.0f;
    /* T60: env·10^(-3) after T60 seconds */
    v->env_coef = powf(0.001f, 1.0f / (GLASS_T60_S * SR));
    /* index in TURNS: dsp_sin takes phase in turns, so index I radians is
     * I/2π turns. Velocity drives the strike brightness: 0.9..2.6 rad. */
    v->idx      = (0.9f + amp * 12.0f) * (1.0f / 6.2831853f);
    if (v->idx > 2.6f * (1.0f / 6.2831853f)) v->idx = 2.6f * (1.0f / 6.2831853f);
    v->idx_coef = expf(-1.0f / (GLASS_IDX_TAU * SR));
    v->active   = 1;
    v->right    = g_next_seat;
    g_next_seat ^= 1;
    v->age      = ++g_stamp;
}

int glass_active_count(void) {
    int n = 0;
    for (int i = 0; i < GLASS_VOICES; ++i)
        if (gv[i].active) ++n;
    return n;
}

void glass_render_mix(float *dry_L, float *dry_R,
                      float *send_L, float *send_R, int frames) {
    for (int i = 0; i < GLASS_VOICES; ++i) {
        glass_voice_t *v = &gv[i];
        if (!v->active) continue;

        float gL = v->right ? 0.35f : 0.65f;
        float gR = v->right ? 0.65f : 0.35f;

        for (int n = 0; n < frames; ++n) {
            float m = dsp_sin(v->ph_m);
            float y = dsp_sin(v->ph_c + v->idx * m) * v->amp * v->env;

            v->ph_c += v->inc_c;
            if (v->ph_c >= 1.0f) v->ph_c -= 1.0f;
            v->ph_m += v->inc_m;
            if (v->ph_m >= 1.0f) v->ph_m -= 1.0f;

            v->idx *= v->idx_coef;      /* the strike fades fast          */
            v->env *= v->env_coef;      /* the ring fades slow            */

            float l = y * gL, r = y * gR;
            dry_L[n]  += l;
            dry_R[n]  += r;
            send_L[n] += l * GLASS_SEND;
            send_R[n] += r * GLASS_SEND;
        }

        /* below audibility (~ -72 dBFS on a full strike) → free the voice */
        if (v->amp * v->env < 2.5e-4f) v->active = 0;
    }
}
