/*
 * engine_bamboo_circuit.c — "BAMBOO CIRCUIT": Buchla / west-coast LPG pluck.
 *
 * Reference (ambient_world_06_bamboo_circuit_lpg): short, organic, woody
 * plucks (measured sustain at 150 ms was only ~0.30 → fast decay). The signature
 * is the Low-Pass Gate: one fast-decaying envelope drives BOTH the amplitude
 * AND the filter cutoff together (vactrol behaviour), so brightness and level
 * fall as one — that is what makes it sound struck and physical, not synthy.
 *
 * Osc = sine + a little FM (wood↔metal) → SVF lowpass (cutoff tracks the LPG
 * envelope) → VCA (same envelope). Plucky, no sustain. dsp.h only.
 */
#include "v2/synth_engine.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR    ((float)DSP_SAMPLE_RATE_HZ)
#define F_UPD 8                 /* the gate moves fast → update the filter often */

static struct {
    float car_ph, mod_ph;
    float freq_cur, freq_tgt, glide_coef;
    float env, env_coef;        /* the single LPG envelope (exp decay) */
    int   active;
    float fm_amt;               /* wood↔metal */
    float decay_s;
    dsp_svf_t lp; int fctr;
    float cut_base, cut_range;
    float level, send;
} b;

static void recalc(void) { b.env_coef = expf(-1.0f / (b.decay_s * SR)); }

static void bc_init(void) {
    memset(&b, 0, sizeof b);
    b.freq_cur = b.freq_tgt = 110.0f;
    b.glide_coef = dsp_smooth_coef(0.020f);
    b.fm_amt   = 0.12f;
    b.decay_s  = 0.18f;          /* short pluck */
    b.cut_base = 200.0f;
    b.cut_range= 5000.0f;
    b.level    = 0.9f;
    b.send     = 0.22f;          /* plucks love a little space */
    recalc();
    dsp_svf_reset(&b.lp); dsp_svf_set(&b.lp, b.cut_base, 0.8f);
}

static void bc_activate(void)   { bc_init(); }
static void bc_deactivate(void) { b.env = 0.0f; b.active = 0; }
static void bc_panic(void)      { b.env = 0.0f; b.active = 0; }

static void bc_note_on(int midi, float vel) {
    float f = dsp_midi_to_hz((float)midi);
    if (!b.active || b.env < 1.0e-3f) b.freq_cur = f;
    b.freq_tgt = f;
    b.env      = 0.4f + 0.6f * dsp_clampf(vel,0,1);    /* strike strength */
    b.active   = 1;
}
static void bc_note_off(void) { /* LPG plucks ring out on their own; gate is a no-op */ }

static void bc_set_param(synth_param_t p, float v) {
    v = dsp_clampf(v, 0.0f, 1.0f);
    switch (p) {
        case SP_A: b.cut_range = 1000.0f + v * 7000.0f;                     break; /* Pluck (brightness) */
        case SP_B: b.decay_s   = 0.05f + v * 0.6f; recalc();                break; /* LPG decay          */
        case SP_C: b.fm_amt    = v * 0.5f;                                  break; /* Wood↔Metal         */
        case SP_D: b.cut_base  = 100.0f + v * 1200.0f;                      break; /* Tone floor         */
        case SP_E: b.glide_coef= dsp_smooth_coef(0.008f + v * 0.12f);       break; /* Glide              */
        case SP_F: b.send      = v * 0.5f;                                  break; /* Space              */
        default: break;
    }
}

static void bc_render_mix(float *dL, float *dR, float *sL, float *sR, int frames) {
    if (!b.active && b.env < 1.0e-4f) return;

    for (int n = 0; n < frames; ++n) {
        b.freq_cur += b.glide_coef * (b.freq_tgt - b.freq_cur);

        b.env *= b.env_coef;                       /* the LPG envelope */
        if (b.env < 1.0e-4f) { b.env = 0.0f; b.active = 0; }

        /* osc: sine + a little FM for wood/metal character */
        float dt_c = b.freq_cur / SR;
        float dt_m = (b.freq_cur * 2.0f) / SR;     /* modulator at 2× = wooden overtone */
        float m  = dsp_sin(b.mod_ph);
        float ph = b.car_ph + b.fm_amt * m;
        ph -= floorf(ph);
        float osc = dsp_sin(ph);
        b.car_ph += dt_c; if (b.car_ph >= 1.0f) b.car_ph -= 1.0f;
        b.mod_ph += dt_m; if (b.mod_ph >= 1.0f) b.mod_ph -= 1.0f;

        /* LPG: cutoff AND amp both follow b.env (filter+amp close together) */
        if ((b.fctr++ % F_UPD) == 0)
            dsp_svf_set(&b.lp, b.cut_base + b.cut_range * b.env, 0.8f);
        float lp  = dsp_svf_lp(&b.lp, osc);
        float out = lp * b.env * b.level;

        dL[n] += out;          dR[n] += out;
        sL[n] += out * b.send; sR[n] += out * b.send;
    }
}

const synth_engine_t engine_bamboo_circuit = {
    .name        = "BAMBOO CIRCUIT",
    .init        = bc_init,
    .activate    = bc_activate,
    .deactivate  = bc_deactivate,
    .note_on     = bc_note_on,
    .note_off    = bc_note_off,
    .set_param   = bc_set_param,
    .render_mix  = bc_render_mix,
    .panic       = bc_panic,
};
