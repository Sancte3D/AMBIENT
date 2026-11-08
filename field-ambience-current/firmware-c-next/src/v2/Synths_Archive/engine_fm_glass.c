/*
 * engine_fm_glass.c — "FM GLASS": a DX7-style 2-operator FM key/glass tone.
 *
 * Integer modulation ratios keep the unaliased partials on the harmonic
 * series. The index envelope falls from a velocity-scaled peak to a Body
 * fraction of that peak; Body must never override a low Index setting.
 * High-register/high-ratio aliasing still needs separate qualification.
 *
 * 2-op phase-modulation FM:
 *   mod   = sin(mod_phase)                       (modulator freq = carrier*ratio)
 *   out   = sin(carrier_phase + index * mod)     (carrier = the pitch)
 * A per-note index envelope (peak → sustain) gives the glassy attack; an amp
 * ADSR gives the ringing key tone; a gentle SVF lowpass tames harshness.
 *
 * dsp.h only — no malloc, no samples, no per-sample powf. The host owns the FX.
 */
#include "v2/synth_engine.h"
#include "synth_names.h"
#include "dsp.h"
#include "shape.h"
#include <math.h>
#include <string.h>

#define SR          ((float)DSP_SAMPLE_RATE_HZ)
#define TONE_UPDATE 16
#define INDEX_SMOOTH (1.0f / (0.080f * SR))

enum { G_IDLE = 0, G_ATTACK, G_DECAY, G_SUSTAIN, G_RELEASE };

static struct {
    float colour_scale, colour_res;
    float car_ph, mod_ph;
    float freq_cur, freq_tgt;
    float glide_coef;
    int   ratio;                 /* integer carrier:modulator ratio */
    /* index (FM brightness) envelope */
    float idx_cur, idx_peak, idx_sustain, idx_coef;
    float idx_env, idx_velocity; /* normalized decay, note velocity scale */
    /* amp envelope */
    int   astate;
    float amp, atk_inc, dec_coef, sustain, rel_coef;
    /* tone */
    dsp_svf_t lp;
    int   tctr;
    float cutoff_hz;
    float level;
    float send;
} g;

static void fm_init(void) {
    memset(&g, 0, sizeof g);
    g.colour_scale = 1.0f;
    g.freq_cur = g.freq_tgt = 110.0f;
    g.glide_coef = dsp_smooth_coef(0.030f);
    g.ratio      = 2;
    g.idx_peak   = 0.28f;         /* turns; restrained glass attack */
    g.idx_sustain= 0.36f;         /* fraction of velocity-scaled peak */
    g.idx_coef   = dsp_smooth_coef(0.30f);
    g.atk_inc    = 1.0f / (0.018f * SR);
    g.dec_coef   = dsp_smooth_coef(0.25f);
    g.sustain    = 0.55f;
    g.rel_coef   = dsp_smooth_coef(0.30f);
    g.cutoff_hz  = 4500.0f;
    g.level      = 0.9f;
    g.send       = 0.16f;        /* glassy → a bit more space than the bass */
    dsp_svf_reset(&g.lp); dsp_svf_set(&g.lp, dsp_clampf(g.cutoff_hz * g.colour_scale, 80.0f, 8000.0f), 0.707f + g.colour_res * 1.8f);
    g.astate = G_IDLE;
}

static void fm_activate(void)   { fm_init(); }
static void fm_deactivate(void) { if (g.astate != G_IDLE) g.astate = G_RELEASE; }
static void fm_panic(void)      { g.amp = 0.0f; g.idx_cur = 0.0f; g.astate = G_IDLE; }

/* Existing glide smooths this target; never re-trigger an envelope. */
static void fm_retune_hz(float hz) {
    if (isfinite(hz) && hz >= 20.0f && hz <= 16000.0f)
        g.freq_tgt = hz;
}

static void fm_note_on(float midi, float vel) {
    g.atk_inc = 1.0f / (0.018f * shape_attack_scale() * SR);
    g.rel_coef = dsp_smooth_coef(0.3f * shape_release_scale());
    float f = dsp_midi_to_hz((float)midi);
    g.idx_velocity = 0.5f + 0.5f * dsp_clampf(vel, 0.0f, 1.0f);
    if (g.astate == G_IDLE || g.amp < 1.0e-3f) {
        g.freq_cur = f;   /* snap only while inaudible */
        g.idx_cur = g.idx_peak * g.idx_velocity;
    }
    g.freq_tgt = f;
    /* A reattack preserves the running index and oscillator phases. */
    g.idx_env = 1.0f;
    g.astate   = G_ATTACK;
}

static void fm_note_off(void) { if (g.astate != G_IDLE) g.astate = G_RELEASE; }

static void fm_set_param(synth_param_t p, float v) {
    v = dsp_clampf(v, 0.0f, 1.0f);
    switch (p) {
        case SP_A: g.idx_peak    = 0.05f + v * 0.85f;                       break; /* Brightness */
        case SP_B: g.ratio       = 1 + (int)(v * 5.0f + 0.5f);              break; /* Ratio 1..6 */
        case SP_C: g.idx_coef    = dsp_smooth_coef(0.05f + v * 0.75f);
                   g.dec_coef    = dsp_smooth_coef(0.05f + v * 0.60f);      break; /* Decay */
        case SP_D: g.cutoff_hz   = 800.0f + v * 8200.0f;                    break; /* Tone */
        case SP_E: g.glide_coef  = dsp_smooth_coef(0.008f + v * 0.12f);     break; /* Glide */
        case SP_F: g.idx_sustain = v * 0.80f;                              break; /* Body: 0..80% of peak */
        default: break;
    }
}

static void fm_render_mix(float *dL, float *dR, float *sL, float *sR, int frames) {
    if (g.astate == G_IDLE && g.amp < 1.0e-4f) return;

    for (int n = 0; n < frames; ++n) {
        g.freq_cur += g.glide_coef * (g.freq_tgt - g.freq_cur);

        /* amp envelope */
        if (g.astate == G_ATTACK) {
            g.amp += g.atk_inc;
            if (g.amp >= 1.0f) { g.amp = 1.0f; g.astate = G_DECAY; }
        } else if (g.astate == G_DECAY) {
            g.amp += g.dec_coef * (g.sustain - g.amp);
            if (g.amp <= g.sustain + 1.0e-3f) { g.amp = g.sustain; g.astate = G_SUSTAIN; }
        } else if (g.astate == G_RELEASE) {
            g.amp += g.rel_coef * (0.0f - g.amp);
            if (g.amp < 1.0e-4f) { g.amp = 0.0f; g.astate = G_IDLE; }
        }

        /* Body is a fraction, so low Index cannot brighten itself during
         * decay. Index remains live on held notes; 80 ms smoothing also
         * avoids a phase-modulation jump when a sounding note reattacks. */
        g.idx_env += g.idx_coef * (g.idx_sustain - g.idx_env);
        float idx_target = g.idx_peak * g.idx_velocity * g.idx_env;
        g.idx_cur += INDEX_SMOOTH * (idx_target - g.idx_cur);

        /* 2-op FM */
        const float dt_c = g.freq_cur / SR;
        const float dt_m = (g.freq_cur * (float)g.ratio) / SR;
        float m  = dsp_sin(g.mod_ph);
        float ph = g.car_ph + g.idx_cur * m;
        ph -= floorf(ph);                      /* wrap to [0,1) for dsp_sin */
        float c  = dsp_sin(ph);

        g.car_ph += dt_c; if (g.car_ph >= 1.0f) g.car_ph -= 1.0f;
        g.mod_ph += dt_m; if (g.mod_ph >= 1.0f) g.mod_ph -= 1.0f;

        if ((g.tctr++ % TONE_UPDATE) == 0) dsp_svf_set(&g.lp, dsp_clampf(g.cutoff_hz * g.colour_scale, 80.0f, 8000.0f), 0.707f + g.colour_res * 1.8f);
        float tone = dsp_svf_lp(&g.lp, c);

        float out = tone * g.amp * g.level;
        dL[n] += out;          dR[n] += out;
        sL[n] += out * g.send; sR[n] += out * g.send;
    }
}

static void fm_set_colour(float scale, float res) {
    g.colour_scale = scale; g.colour_res = res;
}

const synth_engine_t engine_fm_glass = {
    .name        = SYNTH_NAME_FM_GLASS,
    .init        = fm_init,
    .activate    = fm_activate,
    .deactivate  = fm_deactivate,
    .note_on     = fm_note_on,
    .note_off    = fm_note_off,
    .set_param   = fm_set_param,
    .render_mix  = fm_render_mix,
    .panic       = fm_panic,
    .set_colour  = fm_set_colour,
    .retune_hz = fm_retune_hz,
};
