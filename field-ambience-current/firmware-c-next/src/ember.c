/*
 * ember.c — warm subtractive analog-style melody voice. See ember.h.
 */

#include "ember.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR ((float)DSP_SAMPLE_RATE_HZ)

/* --- Voice character constants (the "patch"). --- */
#define DETUNE_CENTS  6.5f      /* saw1/saw2 spread → analog chorus beat   */
#define SUB_LEVEL     0.34f     /* sub-octave triangle body                */
#define SAW_LEVEL     0.5f      /* each saw                                */
#define ATK_S         0.055f    /* soft attack (s)                         */
#define AMP_T60_S     2.6f      /* amp ring after the attack (s)           */
#define FLT_T60_S     0.55f     /* filter envelope decay — closes fast (s) */
#define FC_BASE_HZ    280.0f    /* cutoff floor                            */
#define FC_RANGE_HZ   3300.0f   /* env sweep on top of the floor           */
#define FLT_Q         2.6f      /* resonance — vintage vowel               */
#define LFO_HZ        5.2f      /* vibrato rate                            */
#define VIB_CENTS     6.0f      /* vibrato depth                           */
#define VIB_DELAY_S   0.40f     /* vibrato fades in after the attack       */
#define EMBER_SEND    0.46f     /* hall send — blooms into the room        */
#define FC_UPDATE     32        /* re-set the SVF cutoff every N samples   */
#define RETIRE_EPS    2.5e-4f

typedef struct {
    float ph1, ph2, phs;        /* saw1, saw2, sub-triangle phase (turns) */
    float inc1, inc2, incs;     /* base phase increments per sample       */
    float amp;
    float env;                  /* amp envelope                            */
    int   attacking;            /* 1 = in the attack ramp                  */
    float atk_inc, amp_coef;
    float fenv;                 /* filter envelope 1 → 0                    */
    float fenv_coef;
    dsp_svf_t svf;
    float lfo_ph, lfo_inc;
    float vib_ramp, vib_ramp_inc;
    int   right;                /* stereo seat                             */
    int   active;
    uint32_t age;
} ember_voice_t;

static ember_voice_t ev[EMBER_VOICES];
static uint32_t e_stamp;
static int      e_seat;

void ember_init(void) {
    memset(ev, 0, sizeof ev);
    e_stamp = 0;
    e_seat  = 0;
}

void ember_note(float freq_hz, float amp) {
    if (freq_hz < 40.0f)  freq_hz = 40.0f;
    if (amp < 0.0f) amp = 0.0f;
    if (amp > 1.0f) amp = 1.0f;

    int pick = -1;
    for (int i = 0; i < EMBER_VOICES; ++i)
        if (!ev[i].active) { pick = i; break; }
    if (pick < 0) {
        uint32_t oldest = 0xFFFFFFFFu;
        for (int i = 0; i < EMBER_VOICES; ++i)
            if (ev[i].age < oldest) { oldest = ev[i].age; pick = i; }
    }

    ember_voice_t *v = &ev[pick];
    float up = powf(2.0f,  DETUNE_CENTS / 1200.0f);
    float dn = powf(2.0f, -DETUNE_CENTS / 1200.0f);
    v->ph1 = 0.0f; v->ph2 = 0.37f; v->phs = 0.11f;   /* offset phases: no click-stack */
    v->inc1 = freq_hz * dn / SR;
    v->inc2 = freq_hz * up / SR;
    v->incs = freq_hz * 0.5f  / SR;                  /* sub octave           */
    v->amp  = amp;
    v->env  = 0.0f;
    v->attacking = 1;
    v->atk_inc   = 1.0f / (ATK_S * SR);
    v->amp_coef  = powf(0.001f, 1.0f / (AMP_T60_S * SR));
    v->fenv      = 1.0f;
    v->fenv_coef = powf(0.001f, 1.0f / (FLT_T60_S * SR));
    dsp_svf_reset(&v->svf);
    dsp_svf_set(&v->svf, FC_BASE_HZ + FC_RANGE_HZ, FLT_Q);
    v->lfo_ph = 0.0f;
    v->lfo_inc = LFO_HZ / SR;
    v->vib_ramp = 0.0f;
    v->vib_ramp_inc = 1.0f / (VIB_DELAY_S * SR);
    v->right = e_seat; e_seat ^= 1;
    v->active = 1;
    v->age = ++e_stamp;
}

int ember_active_count(void) {
    int n = 0;
    for (int i = 0; i < EMBER_VOICES; ++i) if (ev[i].active) ++n;
    return n;
}

void ember_render_mix(float *dry_L, float *dry_R,
                      float *send_L, float *send_R, int frames) {
    const float vib_depth = powf(2.0f, VIB_CENTS / 1200.0f) - 1.0f;  /* ≈ fractional */

    for (int i = 0; i < EMBER_VOICES; ++i) {
        ember_voice_t *v = &ev[i];
        if (!v->active) continue;

        float gL = v->right ? 0.38f : 0.62f;
        float gR = v->right ? 0.62f : 0.38f;

        for (int n = 0; n < frames; ++n) {
            /* delayed vibrato → gently modulate all oscillator pitches */
            float lfo = dsp_sin(v->lfo_ph);
            v->lfo_ph += v->lfo_inc; if (v->lfo_ph >= 1.0f) v->lfo_ph -= 1.0f;
            float vib = 1.0f + lfo * vib_depth * v->vib_ramp;

            float d1 = v->inc1 * vib, d2 = v->inc2 * vib, ds = v->incs * vib;
            float s1 = dsp_poly_saw(v->ph1, d1);
            float s2 = dsp_poly_saw(v->ph2, d2);
            float sb = dsp_tri(v->phs);
            v->ph1 += d1; if (v->ph1 >= 1.0f) v->ph1 -= 1.0f;
            v->ph2 += d2; if (v->ph2 >= 1.0f) v->ph2 -= 1.0f;
            v->phs += ds; if (v->phs >= 1.0f) v->phs -= 1.0f;

            float osc = (s1 + s2) * SAW_LEVEL * 0.5f + sb * SUB_LEVEL;

            /* filter cutoff re-set at control-rate (tanf is not per-sample) */
            if ((n & (FC_UPDATE - 1)) == 0)
                dsp_svf_set(&v->svf, FC_BASE_HZ + FC_RANGE_HZ * v->fenv, FLT_Q);
            float y = dsp_svf_lp(&v->svf, osc);

            /* envelopes */
            if (v->attacking) { v->env += v->atk_inc;
                                if (v->env >= 1.0f) { v->env = 1.0f; v->attacking = 0; } }
            else               v->env *= v->amp_coef;
            v->fenv *= v->fenv_coef;
            if (v->vib_ramp < 1.0f) { v->vib_ramp += v->vib_ramp_inc;
                                      if (v->vib_ramp > 1.0f) v->vib_ramp = 1.0f; }

            float o = y * v->amp * v->env;
            float l = o * gL, r = o * gR;
            dry_L[n]  += l;             dry_R[n]  += r;
            send_L[n] += l * EMBER_SEND; send_R[n] += r * EMBER_SEND;
        }

        if (!v->attacking && v->amp * v->env < RETIRE_EPS) v->active = 0;
    }
}
