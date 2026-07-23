/*
 * horn.c — blown-brass / alphorn voice. See horn.h.
 *
 * Signal per voice:
 *   reed   : one band-limited saw (dsp_poly_saw) = the lip-reed buzz
 *   sub    : a quiet LUT sine one octave down = the horn's body weight
 *   blare  : a resonant SVF lowpass whose cutoff OVERSHOOTS on the attack and
 *            settles to a mellow sustain — the brass "blat" that opens bright
 *            and warms as the note holds (opposite envelope shape to a string)
 *   formant: one fixed SVF bandpass ≈ 950 Hz mixed in = the cupped-horn vowel
 *   chiff  : a short breath-noise burst through a high bandpass, ONLY at the
 *            onset (≈ 90 ms) = the air catching the reed, then gone
 *   drift  : very slow, shallow air-pressure tremor (alphorns barely vibrato)
 *
 * Control-rate work (coeff/LFO updates) every CTL samples; per-sample stays
 * one saw + one sine + one LP + one BP + adds. Alias-free, no per-sample
 * transcendental.
 */
#include "horn.h"
#include "dsp.h"
#include <string.h>

#define SR    ((float)DSP_SAMPLE_RATE_HZ)
#define CTL   32
#define VMAX  3

typedef enum { V_IDLE = 0, V_ATTACK, V_HOLD, V_RELEASE } vstage_t;

typedef struct {
    vstage_t stage;
    float    freq, amp;
    float    ph, inc;                 /* reed saw                          */
    float    subPh, subInc;           /* sub-octave body sine              */
    dsp_svf_t blare;                  /* envelope-tracking brass lowpass   */
    dsp_svf_t form;                   /* fixed horn formant bandpass       */
    dsp_svf_t chiffbp;                /* attack air-chiff bandpass         */
    uint32_t rng;

    float    env, envInc, relCoef;    /* amp envelope                      */
    int      hold_left;               /* samples of sustain left           */
    float    blareEnv;                /* brightness env: overshoot→settle  */
    int      chiff_left;              /* samples of air chiff remaining    */

    float    driftPh, driftInc;       /* slow air-pressure tremor (turns)  */
    float    panL, panR;
} hvoice_t;

static hvoice_t V[VMAX];
static int      ctl;

static inline float wnoise(uint32_t *r) {
    *r = (*r) * 1664525u + 1013904223u;
    return (float)((int32_t)*r) * (1.0f / 2147483648.0f);
}

void horn_init(void) {
    memset(V, 0, sizeof V);
    ctl = 0;
}

static int alloc_voice(void) {
    int best = -1; float lo = 1e9f;
    for (int i = 0; i < VMAX; ++i) {
        if (V[i].stage == V_IDLE) return i;
        if (V[i].env < lo) { lo = V[i].env; best = i; }
    }
    return best;
}

void horn_note(float freq_hz, float amp) {
    if (freq_hz < 20.0f) return;
    int i = alloc_voice();
    if (i < 0) return;
    hvoice_t *v = &V[i];
    v->freq = freq_hz;
    v->amp  = dsp_clampf(amp, 0.0f, 1.0f);
    v->inc  = freq_hz / SR;
    v->subInc = freq_hz * 0.5f / SR;
    if (V[i].stage == V_IDLE) { v->ph = 0.02f; v->subPh = 0.0f; }
    v->rng = 0x51ED270Bu ^ (uint32_t)(freq_hz * 97.0f);

    dsp_svf_reset(&v->blare);   dsp_svf_set(&v->blare, freq_hz * 3.0f, 1.6f);
    dsp_svf_reset(&v->form);    dsp_svf_set(&v->form, 950.0f, 2.2f);
    dsp_svf_reset(&v->chiffbp); dsp_svf_set(&v->chiffbp, 1700.0f, 1.1f);

    v->env = 0.0001f;
    v->envInc  = v->amp / (0.13f * SR);       /* ~130 ms blow-in (faster than bow) */
    v->relCoef = dsp_smooth_coef(0.7f);        /* ~1.4 s tail                       */
    v->hold_left = (int)(2.8f * SR);           /* call ~2.8 s                        */
    v->blareEnv  = 0.0f;
    v->chiff_left = (int)(0.09f * SR);         /* 90 ms of air at the onset          */
    v->driftPh = 0.0f; v->driftInc = 0.9f / SR;/* ~0.9 Hz air tremor                 */
    /* gentle stereo spread per voice */
    float pan = (i == 0) ? -0.2f : (i == 1) ? 0.2f : 0.0f;
    v->panL = 0.5f * (1.0f - pan);
    v->panR = 0.5f * (1.0f + pan);
    v->stage = V_ATTACK;
}

int horn_active_count(void) {
    int c = 0;
    for (int i = 0; i < VMAX; ++i) if (V[i].stage != V_IDLE) ++c;
    return c;
}

void horn_render_mix(float *dry_L, float *dry_R,
                     float *send_L, float *send_R,
                     int frames, float send_amount) {
    for (int n = 0; n < frames; ++n) {
        float L = 0.0f, R = 0.0f;
        int do_ctl = (ctl == 0);

        for (int i = 0; i < VMAX; ++i) {
            hvoice_t *v = &V[i];
            if (v->stage == V_IDLE) continue;

            if (do_ctl) {
                /* brass "blat": brightness overshoots during the attack
                 * (blareEnv → toward 1.0), then settles to a mellow sustain
                 * (~0.35). The cupped-horn cutoff rides that env HIGH and
                 * warms as the note holds — the signature brass shape. */
                float btgt = (v->stage == V_ATTACK) ? 1.0f : 0.35f;
                v->blareEnv += (btgt - v->blareEnv) * 0.035f;
                float drift = dsp_sin(v->driftPh);
                float cut = v->freq * (3.0f + 8.0f * v->blareEnv) * (1.0f + 0.03f * drift);
                dsp_svf_set(&v->blare, dsp_clampf(cut, 120.0f, SR * 0.45f), 1.6f);
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

            v->driftPh += v->driftInc; if (v->driftPh >= 1.0f) v->driftPh -= 1.0f;

            /* reed saw + sub-octave body sine */
            float reed = dsp_poly_saw(v->ph, v->inc);
            v->ph += v->inc; if (v->ph >= 1.0f) v->ph -= 1.0f;
            float sub = dsp_sin(v->subPh) * 0.30f;
            v->subPh += v->subInc; if (v->subPh >= 1.0f) v->subPh -= 1.0f;

            /* brass body: reed through the blaring lowpass, + a touch of the
             * fixed horn formant vowel for the cupped colour */
            float brass = dsp_svf_lp(&v->blare, reed + sub);
            brass += dsp_svf_bp(&v->form, reed) * 0.25f;

            /* attack air chiff (onset only) */
            if (v->chiff_left > 0) {
                --v->chiff_left;
                float a = (float)v->chiff_left / (0.09f * SR);   /* fade out */
                brass += dsp_svf_bp(&v->chiffbp, wnoise(&v->rng)) * a * a * 0.5f;
            }

            float out = brass * v->env * 0.5f;
            L += out * v->panL;
            R += out * v->panR;
        }

        if (++ctl >= CTL) ctl = 0;

        dry_L[n]  += L;  dry_R[n]  += R;
        send_L[n] += L * send_amount;
        send_R[n] += R * send_amount;
    }
}
