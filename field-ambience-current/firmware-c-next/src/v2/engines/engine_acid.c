/* Dusk: a warm, compact subtractive voice with a rounded filter bloom.
 * Legacy engine_acid symbol and scene slot are stable. Filter frequency is
 * referenced to C4 and tracks the smoothed pitch, preserving harmonic balance
 * across registers. One shared ladder; no new audio buffers or layers. */
#include "v2/synth_engine.h"
#include "synth_names.h"
#include "dsp.h"
#include "shape.h"
#include "dsp_ladder.h"
#include <math.h>
#include <string.h>

#define SR          ((float)DSP_SAMPLE_RATE_HZ)
#define C4_HZ       261.625565f
#define FENV_SMOOTH (1.0f / (0.060f * SR))
#define FILT_UPDATE 8           /* control-rate cutoff refresh (samples) */

enum { A_IDLE = 0, A_ATTACK, A_DECAY, A_SUSTAIN, A_RELEASE };

static struct {
    float colour_scale, colour_res;
    /* oscillators */
    float phase, sq_phase;
    float freq_cur, freq_tgt;
    float glide_coef;
    /* amp envelope: attack → decay → sustain (while gated) → release */
    int   astate;
    float amp, atk_inc, dec_coef, sustain, rel_coef;
    /* Decaying excitation, smoothed before it reaches the filter. */
    float fenv, fenv_coef, fenv_trigger;
    /* the real ladder filter */
    dsp_ladder_t lad;
    int   fctr;
    float accent;                /* 0..1 for the current note */
    /* parameters */
    float base_cut;              /* Hz at C4 — tracks current pitch */
    float env_amt;               /* Hz at C4 — bloom height */
    float res;                   /* native resonance 0.15..1.10 */
    float decay_s;               /* filter-env decay time */
    float drive;                 /* ladder input drive (grit) */
    float level;
    float send;
} a;

static void recalc_decay(void) { a.fenv_coef = expf(-1.0f / (a.decay_s * SR)); }

static void acid_init(void) {
    memset(&a, 0, sizeof a);
    a.colour_scale = 1.0f;
    a.freq_cur = a.freq_tgt = 110.0f;        /* A2 */
    a.glide_coef = dsp_smooth_coef(0.008f);
    a.atk_inc    = 1.0f / (0.120f * SR);
    a.dec_coef   = dsp_smooth_coef(0.280f);
    a.sustain    = 0.80f;
    a.rel_coef   = dsp_smooth_coef(0.400f);
    a.base_cut   = 650.0f;
    a.env_amt    = 312.0f;
    a.res        = 0.1975f;      /* restrained native resonance */
    a.decay_s    = 0.62f;
    a.drive      = 1.00f;        /* gentle native saturation */
    a.level      = 0.50f;
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
static void acid_panic(void)      { a.amp = 0.0f; a.fenv = a.fenv_trigger = 0.0f; a.astate = A_IDLE; }

/* Existing glide smooths this target; never re-trigger an envelope. */
static void acid_retune_hz(float hz) {
    if (isfinite(hz) && hz >= 20.0f && hz <= 16000.0f)
        a.freq_tgt = hz;
}

static void acid_note_on(float midi, float vel) {
    a.atk_inc = 1.0f / (0.120f * shape_attack_scale() * SR);
    a.rel_coef = dsp_smooth_coef(0.40f * shape_release_scale());
    float f = dsp_midi_to_hz((float)midi);
    if (a.astate == A_IDLE || a.amp < 1.0e-3f) a.freq_cur = f;   /* snap from silence */
    a.freq_tgt = f;
    a.accent   = dsp_clampf(vel, 0.0f, 1.0f);
    a.fenv_trigger = 1.0f;       /* preserve current filter state on reattack */
    a.astate   = A_ATTACK;
}

static void acid_note_off(void) { if (a.astate != A_IDLE) a.astate = A_RELEASE; }

static void acid_set_param(synth_param_t p, float v) {
    v = dsp_clampf(v, 0.0f, 1.0f);
    switch (p) {
        case SP_A: a.base_cut = 100.0f + v * v * 2200.0f;                  break; /* Cutoff   */
        case SP_B: a.res = 0.15f + v * 0.95f;                             break; /* Resonance*/
        case SP_C: a.decay_s = 0.20f + v * 1.20f; recalc_decay();          break; /* Decay    */
        case SP_D: a.drive = 1.0f + v * 1.20f; dsp_ladder_set_drive(&a.lad, a.drive); break; /* Drive */
        case SP_E: a.glide_coef = dsp_smooth_coef(0.008f + v * 0.12f);     break; /* Glide    */
        case SP_F: a.env_amt = v * 2600.0f;                     break; /* Env amt  */
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

        a.fenv_trigger *= a.fenv_coef;
        a.fenv += FENV_SMOOTH * (a.fenv_trigger - a.fenv);

        if ((a.fctr++ % FILT_UPDATE) == 0) {
            /* Track smoothed pitch: a held retune must not jump the cutoff. */
            float cut = (a.base_cut + a.env_amt * (1.0f + a.accent * 0.3f) * a.fenv)
                        * (a.freq_cur / C4_HZ);
            dsp_ladder_set_freq(&a.lad, dsp_clampf(cut * a.colour_scale,80.0f,8000.0f));
            dsp_ladder_set_res(&a.lad, dsp_clampf(a.res + a.accent * 0.10f + a.colour_res * 0.5f,0.0f,1.8f));   /* gentle accent */
        }

        const float dt = a.freq_cur / SR;
        float saw = dsp_poly_saw(a.phase, dt);
        float sq  = dsp_poly_square(a.sq_phase, dt);
        /* These DSP primitives have opposite fundamental polarity at the
         * same phase. Subtract square so fundamentals reinforce; adding it
         * suppressed the root and let the octave dominate the default tone. */
        float osc = 0.80f * saw - 0.20f * sq;
        a.phase    += dt; if (a.phase    >= 1.0f) a.phase    -= 1.0f;
        a.sq_phase += dt; if (a.sq_phase >= 1.0f) a.sq_phase -= 1.0f;

        float f   = dsp_ladder_process(&a.lad, osc);
        float out = f * a.amp * a.level * (0.8f + a.accent * 0.3f);

        dL[n] += out;          dR[n] += out;
        sL[n] += out * a.send; sR[n] += out * a.send;
    }
}

static void acid_set_colour(float scale, float res) {
    a.colour_scale = scale; a.colour_res = res;
}

const synth_engine_t engine_acid = {
    .name        = SYNTH_NAME_ACID,
    .init        = acid_init,
    .activate    = acid_activate,
    .deactivate  = acid_deactivate,
    .note_on     = acid_note_on,
    .note_off    = acid_note_off,
    .set_param   = acid_set_param,
    .render_mix  = acid_render_mix,
    .panic       = acid_panic,
    .set_colour  = acid_set_colour,
    .retune_hz = acid_retune_hz,
};
