/*
 * bowed.c — bowed-string voice. See bowed.h.
 *
 * Signal per voice:
 *   string : two detuned band-limited saws (dsp_poly_saw) = harmonic body
 *   bow    : white noise → bandpass, level follows a bow-pressure envelope
 *            (grain swells on the attack, settles to a whisper on the sustain)
 *   body   : one resonant SVF lowpass = the wooden instrument body; its cutoff
 *            opens with bow pressure and breathes with a slow LFO
 *   symp   : two high-Q SVF bandpass resonators at the 5th and octave, lightly
 *            fed back = sympathetic strings (the lyra/Hardanger shimmer)
 *   vibrato: slow LUT-sine on the pitch, delayed onset (bowing settles first)
 *
 * Control-rate work (coeff/LFO updates) every CTL samples; per-sample stays
 * two saws + one LP + two BP + adds. Alias-free, no per-sample transcendental.
 */
#include "bowed.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR    ((float)DSP_SAMPLE_RATE_HZ)
#define CTL   32
#define VMAX  3

typedef enum { V_IDLE = 0, V_ATTACK, V_HOLD, V_RELEASE } vstage_t;

typedef struct {
    vstage_t stage;
    float    freq, amp;
    float    ph, ph2, inc, inc2, dt;      /* two detuned saws            */
    dsp_svf_t body, symp1, symp2;
    uint32_t rng;
    dsp_svf_t bowbp;                       /* bow-noise bandpass          */

    float    env, envInc, relCoef;         /* amp envelope                */
    int      hold_left;                    /* samples of sustain left     */
    float    bow;                          /* bow-pressure env 0..1       */

    float    vibPh, vibInc;                /* vibrato LFO (turns)         */
    float    bodyPh, bodyInc;              /* body-breath LFO             */
    int      vibDelay;                     /* samples before vibrato fades in */

    float    panL, panR;
    float    body_base, symp_gain;         /* colour-dependent            */
} bvoice_t;

static bvoice_t V[VMAX];
static int      ctl;
static int      s_colour = 0;

static inline float wnoise(uint32_t *r) {
    *r = (*r) * 1664525u + 1013904223u;
    return (float)((int32_t)*r) * (1.0f / 2147483648.0f);
}

void bowed_init(void) {
    memset(V, 0, sizeof V);
    ctl = 0;
    s_colour = 0;
}

void bowed_set_colour(int colour) { s_colour = colour ? 1 : 0; }

static int alloc_voice(void) {
    int best = -1; float lo = 1e9f;
    for (int i = 0; i < VMAX; ++i) {
        if (V[i].stage == V_IDLE) return i;
        if (V[i].env < lo) { lo = V[i].env; best = i; }
    }
    return best;
}

void bowed_note(float freq_hz, float amp) {
    if (freq_hz < 20.0f) return;
    int i = alloc_voice();
    if (i < 0) return;
    bvoice_t *v = &V[i];
    v->freq = freq_hz;
    v->amp  = dsp_clampf(amp, 0.0f, 1.0f);
    v->inc  = freq_hz / SR;
    v->inc2 = freq_hz * 1.0041f / SR;         /* +7 cents ensemble detune  */
    v->dt   = v->inc;
    if (V[i].stage == V_IDLE) { v->ph = 0.03f; v->ph2 = 0.51f; }  /* fresh phase */
    v->rng  = 0x9E3779B9u ^ (uint32_t)(freq_hz * 131.0f);

    /* colour: Open Sea = warmer/brighter body, moderate symp; Fjords = darker,
     * more sympathetic ring. */
    v->body_base = (s_colour == 0) ? freq_hz * 6.5f : freq_hz * 4.2f;
    v->symp_gain = (s_colour == 0) ? 0.10f : 0.17f;

    dsp_svf_reset(&v->body);  dsp_svf_set(&v->body, v->body_base, 0.9f);
    dsp_svf_reset(&v->symp1); dsp_svf_set(&v->symp1, freq_hz * 1.5f, 9.0f);
    dsp_svf_reset(&v->symp2); dsp_svf_set(&v->symp2, freq_hz * 2.0f, 8.0f);
    dsp_svf_reset(&v->bowbp); dsp_svf_set(&v->bowbp, freq_hz * 2.6f, 1.4f);

    v->env = 0.0001f;
    v->envInc = v->amp / (0.30f * SR);        /* ~300 ms bow swell         */
    v->relCoef = dsp_smooth_coef(0.9f);        /* ~2 s tail                 */
    v->hold_left = (int)(3.6f * SR);           /* sing ~3.6 s               */
    v->bow = 0.0f;
    v->vibPh = 0.0f; v->vibInc = 5.1f / SR;    /* ~5.1 Hz vibrato           */
    v->bodyPh = 0.0f; v->bodyInc = 0.13f / SR; /* slow body breath          */
    v->vibDelay = (int)(0.6f * SR);            /* vibrato fades in after 0.6 s */
    /* gentle stereo spread per voice */
    float pan = (i == 0) ? -0.25f : (i == 1) ? 0.25f : 0.0f;
    v->panL = 0.5f * (1.0f - pan);
    v->panR = 0.5f * (1.0f + pan);
    v->stage = V_ATTACK;
}

int bowed_active_count(void) {
    int c = 0;
    for (int i = 0; i < VMAX; ++i) if (V[i].stage != V_IDLE) ++c;
    return c;
}

void bowed_render_mix(float *dry_L, float *dry_R,
                      float *send_L, float *send_R,
                      int frames, float send_amount) {
    for (int n = 0; n < frames; ++n) {
        float L = 0.0f, R = 0.0f;
        int do_ctl = (ctl == 0);

        for (int i = 0; i < VMAX; ++i) {
            bvoice_t *v = &V[i];
            if (v->stage == V_IDLE) continue;

            if (do_ctl) {
                /* bow pressure: rises through the attack, settles to a low
                 * sustained value — the grain follows it. */
                float target = (v->stage == V_ATTACK) ? 1.0f : 0.35f;
                v->bow += (target - v->bow) * 0.02f;
                /* vibrato depth fades in; body cutoff opens with bow + breath */
                float breath = dsp_sin(v->bodyPh);
                float cut = v->body_base * (1.0f + 0.35f * v->bow + 0.06f * breath);
                dsp_svf_set(&v->body, dsp_clampf(cut, 120.0f, SR * 0.45f), 0.9f);
            }

            /* amp envelope */
            switch (v->stage) {
                case V_ATTACK:
                    v->env += v->envInc;
                    if (v->env >= v->amp) { v->env = v->amp; v->stage = V_HOLD; }
                    break;
                case V_HOLD:
                    if (--v->hold_left <= 0) v->stage = V_RELEASE;
                    break;
                case V_RELEASE:
                    v->env -= v->relCoef * v->env;
                    if (v->env <= 1.0e-5f) { v->env = 0.0f; v->stage = V_IDLE; }
                    break;
                default: break;
            }
            if (v->stage == V_IDLE) continue;

            /* vibrato (LUT sine, per sample is cheap — table lookup) */
            float vibAmt = 1.0f;
            if (v->vibDelay > 0) { --v->vibDelay; vibAmt = 0.0f; }
            float vib = dsp_sin(v->vibPh) * 0.0035f * vibAmt;   /* ±~6 cents */
            v->vibPh += v->vibInc; if (v->vibPh >= 1.0f) v->vibPh -= 1.0f;
            v->bodyPh += v->bodyInc; if (v->bodyPh >= 1.0f) v->bodyPh -= 1.0f;

            /* string: two detuned band-limited saws */
            float inc  = v->inc  * (1.0f + vib);
            float inc2 = v->inc2 * (1.0f + vib);
            float s = dsp_poly_saw(v->ph,  inc)  * 0.6f
                    + dsp_poly_saw(v->ph2, inc2) * 0.4f;
            v->ph  += inc;  if (v->ph  >= 1.0f) v->ph  -= 1.0f;
            v->ph2 += inc2; if (v->ph2 >= 1.0f) v->ph2 -= 1.0f;

            /* bow-noise grain */
            float bn = dsp_svf_bp(&v->bowbp, wnoise(&v->rng)) * (0.06f + 0.20f * v->bow);

            /* wooden body */
            float body = dsp_svf_lp(&v->body, s + bn);

            /* sympathetic resonators (fed lightly, ring back in) */
            float sy = dsp_svf_bp(&v->symp1, body) + dsp_svf_bp(&v->symp2, body);
            float out = (body + sy * v->symp_gain) * v->env * 0.5f;

            L += out * v->panL;
            R += out * v->panR;
        }

        if (++ctl >= CTL) ctl = 0;

        dry_L[n]  += L;  dry_R[n]  += R;
        send_L[n] += L * send_amount;
        send_R[n] += R * send_amount;
    }
}
