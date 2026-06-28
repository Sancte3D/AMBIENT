/*
 * engine_acid.c — "ACID RAIN": a TB-303-style resonant acid bass.
 *
 * Calibrated against the golden reference (ambient_world_01_acid_rain...wav):
 *   bass register A2..D3, ~117 BPM straight 8ths, and a resonant lowpass
 *   whose cutoff sweeps ~6.5 kHz → ~1.6 kHz per note (the squelch). That
 *   per-note filter envelope + high resonance + a little drive IS the sound.
 *
 * Signal: saw(+a little square) → resonant SVF lowpass (cutoff = base +
 *   env_amt*filter_env, env retriggered each note, exp decay) → VCA(amp env)
 *   → tanh drive. Accent (high velocity) opens the filter further, adds
 *   resonance, and pushes drive/level. Portamento glides between legato notes.
 *
 * dsp.h only — no malloc, no samples, no per-sample powf/sinf. The host owns
 * reverb + the master limiter, so a high-Q overshoot here is caught downstream.
 */
#include "v2/synth_engine.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR          ((float)DSP_SAMPLE_RATE_HZ)
#define FILT_UPDATE 16          /* control-rate cutoff refresh (samples) */

enum { A_IDLE = 0, A_ATTACK, A_DECAY, A_SUSTAIN, A_RELEASE };

static struct {
    /* oscillators */
    float phase, sq_phase;
    float freq_cur, freq_tgt;
    float glide_coef;
    /* amp envelope (attack → decay → sustain while gated → release) */
    int   astate;
    float amp, atk_inc, dec_coef, sustain, rel_coef;
    /* per-note filter envelope (exp decay → the squelch) */
    float fenv, fenv_coef;
    /* resonant lowpass */
    dsp_svf_t lp;
    int   fctr;
    float accent;                /* 0..1 for the current note */
    /* parameters (mapped to internal ranges) */
    float base_cut;              /* Hz — filter floor */
    float env_amt;               /* Hz — sweep height */
    float q;                     /* resonance */
    float decay_s;               /* filter-env decay time */
    float drive;                 /* tanh pre-gain */
    float level;
    float send;                  /* reverb send (acid is fairly dry) */
} a;

static void recalc_decay(void) {
    /* per-sample multiplier for an exp decay of time-constant decay_s */
    a.fenv_coef = expf(-1.0f / (a.decay_s * SR));
}

static void acid_init(void) {
    memset(&a, 0, sizeof a);
    a.freq_cur = a.freq_tgt = 110.0f;        /* A2 */
    a.glide_coef = dsp_smooth_coef(0.030f);
    a.atk_inc    = 1.0f / (0.006f * SR);     /* 6 ms attack  */
    a.dec_coef   = dsp_smooth_coef(0.080f);  /* 80 ms decay → sustain (the pluck) */
    a.sustain    = 0.78f;
    a.rel_coef   = dsp_smooth_coef(0.060f);  /* 60 ms release */
    a.base_cut   = 350.0f;
    a.env_amt    = 6000.0f;
    a.q          = 4.5f;
    a.decay_s    = 0.18f;
    a.drive      = 1.5f;
    a.level      = 0.9f;
    a.send       = 0.06f;
    recalc_decay();
    dsp_svf_reset(&a.lp);
    dsp_svf_set(&a.lp, a.base_cut, a.q);
    a.astate = A_IDLE;
}

static void acid_activate(void)   { acid_init(); }
static void acid_deactivate(void) { if (a.astate != A_IDLE) a.astate = A_RELEASE; }
static void acid_panic(void)      { a.amp = 0.0f; a.fenv = 0.0f; a.astate = A_IDLE; }

static void acid_note_on(int midi, float vel) {
    float f = dsp_midi_to_hz((float)midi);
    if (a.astate == A_IDLE || a.amp < 1.0e-3f) a.freq_cur = f;   /* snap from silence */
    a.freq_tgt = f;                                             /* else glide (slide) */
    a.accent   = dsp_clampf(vel, 0.0f, 1.0f);
    a.fenv     = 1.0f;                                          /* retrigger the squelch */
    a.astate   = A_ATTACK;
}

static void acid_note_off(void) { if (a.astate != A_IDLE) a.astate = A_RELEASE; }

static void acid_set_param(synth_param_t p, float v) {
    v = dsp_clampf(v, 0.0f, 1.0f);
    switch (p) {
        case SP_A: a.base_cut = 120.0f + v * v * 2000.0f;                 break; /* Cutoff   */
        case SP_B: a.q        = 0.8f + v * 7.0f;                          break; /* Resonance*/
        case SP_C: a.decay_s  = 0.04f + v * 0.50f; recalc_decay();        break; /* Decay    */
        case SP_D: a.drive    = 1.0f + v * 3.0f;                          break; /* Drive    */
        case SP_E: a.glide_coef = dsp_smooth_coef(0.008f + v * 0.12f);    break; /* Glide    */
        case SP_F: a.env_amt  = 1500.0f + v * 7000.0f;                    break; /* Env amt  */
        default: break;
    }
}

static void acid_render_mix(float *dL, float *dR, float *sL, float *sR, int frames) {
    if (a.astate == A_IDLE && a.amp < 1.0e-4f) return;

    for (int n = 0; n < frames; ++n) {
        /* portamento */
        a.freq_cur += a.glide_coef * (a.freq_tgt - a.freq_cur);

        /* amp envelope: attack → decay → sustain (while gated) → release */
        if (a.astate == A_ATTACK) {
            a.amp += a.atk_inc;
            if (a.amp >= 1.0f) { a.amp = 1.0f; a.astate = A_DECAY; }
        } else if (a.astate == A_DECAY) {
            a.amp += a.dec_coef * (a.sustain - a.amp);
            if (a.amp <= a.sustain + 1.0e-3f) { a.amp = a.sustain; a.astate = A_SUSTAIN; }
        } else if (a.astate == A_RELEASE) {
            a.amp += a.rel_coef * (0.0f - a.amp);
            if (a.amp < 1.0e-4f) { a.amp = 0.0f; a.astate = A_IDLE; }
        }

        /* filter envelope decays toward 0 → cutoff falls → squelch */
        a.fenv *= a.fenv_coef;

        /* control-rate cutoff: accent opens it more + adds resonance */
        if ((a.fctr++ % FILT_UPDATE) == 0) {
            float cut = a.base_cut + (a.env_amt * (1.0f + a.accent * 0.8f)) * a.fenv;
            if (cut > 16000.0f) cut = 16000.0f;
            dsp_svf_set(&a.lp, cut, a.q + a.accent * 2.0f);
        }

        /* oscillator: saw with a touch of square for the 303 buzz */
        const float dt  = a.freq_cur / SR;
        float saw = dsp_poly_saw(a.phase, dt);
        float sq  = dsp_poly_square(a.sq_phase, dt);
        float osc = 0.80f * saw + 0.20f * sq;
        a.phase    += dt; if (a.phase    >= 1.0f) a.phase    -= 1.0f;
        a.sq_phase += dt; if (a.sq_phase >= 1.0f) a.sq_phase -= 1.0f;

        /* VCF → VCA → drive */
        float f      = dsp_svf_lp(&a.lp, osc);
        float driven = tanhf(f * a.drive * (1.0f + a.accent * 0.6f));
        float out    = driven * a.amp * a.level * (0.7f + a.accent * 0.3f);

        dL[n] += out;          dR[n] += out;
        sL[n] += out * a.send; sR[n] += out * a.send;
    }
}

const synth_engine_t engine_acid = {
    .name        = "ACID RAIN",
    .init        = acid_init,
    .activate    = acid_activate,
    .deactivate  = acid_deactivate,
    .note_on     = acid_note_on,
    .note_off    = acid_note_off,
    .set_param   = acid_set_param,
    .render_mix  = acid_render_mix,
    .panic       = acid_panic,
};
