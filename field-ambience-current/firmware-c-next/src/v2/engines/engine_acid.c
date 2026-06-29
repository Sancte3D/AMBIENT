/*
 * engine_acid.c — "ACID RAIN": a TB-303-style resonant acid bass.
 *
 * Now built on a real 4-pole Huovilainen ladder filter (dsp_ladder, MIT port of
 * DaisySP's LadderFilter) instead of a generic SVF — THAT is what gives the
 * squelchy, singing, slightly-overdriven acid character a plain SVF can't. The
 * per-note filter envelope sweeps the ladder cutoff (the squelch); the ladder's
 * own nonlinear drive + high resonance + accent give the bite.
 *
 * Signal: saw(+a little square) → ladder LP (cutoff swept by the note env,
 *   resonance high, input drive = grit) → amp ADSR. dsp.h + dsp_ladder only.
 */
#include "v2/synth_engine.h"
#include "dsp.h"
#include "dsp_ladder.h"
#include <math.h>
#include <string.h>

#define SR          ((float)DSP_SAMPLE_RATE_HZ)
#define FILT_UPDATE 8           /* control-rate cutoff refresh (samples) */

enum { A_IDLE = 0, A_ATTACK, A_DECAY, A_SUSTAIN, A_RELEASE };

static struct {
    /* oscillators */
    float phase, sq_phase;
    float freq_cur, freq_tgt;
    float glide_coef;
    /* amp envelope: attack → decay → sustain (while gated) → release */
    int   astate;
    float amp, atk_inc, dec_coef, sustain, rel_coef;
    /* per-note filter envelope (exp decay → the squelch) */
    float fenv, fenv_coef;
    /* the real ladder filter */
    dsp_ladder_t lad;
    int   fctr;
    float accent;                /* 0..1 for the current note */
    /* parameters */
    float base_cut;              /* Hz — filter floor */
    float env_amt;               /* Hz — sweep height */
    float res;                   /* resonance 0..1.7 */
    float decay_s;               /* filter-env decay time */
    float drive;                 /* ladder input drive (grit) */
    float level;
    float send;
} a;

static void recalc_decay(void) { a.fenv_coef = expf(-1.0f / (a.decay_s * SR)); }

static void acid_init(void) {
    memset(&a, 0, sizeof a);
    a.freq_cur = a.freq_tgt = 110.0f;        /* A2 */
    a.glide_coef = dsp_smooth_coef(0.030f);
    a.atk_inc    = 1.0f / (0.006f * SR);
    a.dec_coef   = dsp_smooth_coef(0.080f);
    a.sustain    = 0.80f;
    a.rel_coef   = dsp_smooth_coef(0.060f);
    a.base_cut   = 320.0f;
    a.env_amt    = 5500.0f;
    a.res        = 1.05f;        /* strong resonance = the squelch */
    a.decay_s    = 0.18f;
    a.drive      = 1.8f;         /* ladder input drive = the acid grit */
    a.level      = 0.85f;
    a.send       = 0.06f;
    recalc_decay();
    dsp_ladder_init(&a.lad, SR);
    dsp_ladder_set_res(&a.lad, a.res);
    dsp_ladder_set_drive(&a.lad, a.drive);
    dsp_ladder_set_freq(&a.lad, a.base_cut);
    a.astate = A_IDLE;
}

static void acid_activate(void)   { acid_init(); }
static void acid_deactivate(void) { if (a.astate != A_IDLE) a.astate = A_RELEASE; }
static void acid_panic(void)      { a.amp = 0.0f; a.fenv = 0.0f; a.astate = A_IDLE; }

static void acid_note_on(int midi, float vel) {
    float f = dsp_midi_to_hz((float)midi);
    if (a.astate == A_IDLE || a.amp < 1.0e-3f) a.freq_cur = f;   /* snap from silence */
    a.freq_tgt = f;
    a.accent   = dsp_clampf(vel, 0.0f, 1.0f);
    a.fenv     = 1.0f;
    a.astate   = A_ATTACK;
}

static void acid_note_off(void) { if (a.astate != A_IDLE) a.astate = A_RELEASE; }

static void acid_set_param(synth_param_t p, float v) {
    v = dsp_clampf(v, 0.0f, 1.0f);
    switch (p) {
        case SP_A: a.base_cut = 100.0f + v * v * 2200.0f;                  break; /* Cutoff   */
        case SP_B: a.res = 0.30f + v * 1.40f;                             break; /* Resonance*/
        case SP_C: a.decay_s = 0.04f + v * 0.50f; recalc_decay();          break; /* Decay    */
        case SP_D: a.drive = 0.6f + v * 3.0f; dsp_ladder_set_drive(&a.lad, a.drive); break; /* Drive */
        case SP_E: a.glide_coef = dsp_smooth_coef(0.008f + v * 0.12f);     break; /* Glide    */
        case SP_F: a.env_amt = 1500.0f + v * 7000.0f;                     break; /* Env amt  */
        default: break;
    }
}

static void acid_render_mix(float *dL, float *dR, float *sL, float *sR, int frames) {
    if (a.astate == A_IDLE && a.amp < 1.0e-4f) return;

    for (int n = 0; n < frames; ++n) {
        a.freq_cur += a.glide_coef * (a.freq_tgt - a.freq_cur);

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

        a.fenv *= a.fenv_coef;

        if ((a.fctr++ % FILT_UPDATE) == 0) {
            float cut = a.base_cut + (a.env_amt * (1.0f + a.accent * 0.8f)) * a.fenv;
            dsp_ladder_set_freq(&a.lad, cut);
            dsp_ladder_set_res(&a.lad, a.res + a.accent * 0.25f);   /* accent = more squelch */
        }

        const float dt = a.freq_cur / SR;
        float saw = dsp_poly_saw(a.phase, dt);
        float sq  = dsp_poly_square(a.sq_phase, dt);
        float osc = 0.80f * saw + 0.20f * sq;
        a.phase    += dt; if (a.phase    >= 1.0f) a.phase    -= 1.0f;
        a.sq_phase += dt; if (a.sq_phase >= 1.0f) a.sq_phase -= 1.0f;

        float f   = dsp_ladder_process(&a.lad, osc);
        float out = f * a.amp * a.level * (0.8f + a.accent * 0.3f);

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
