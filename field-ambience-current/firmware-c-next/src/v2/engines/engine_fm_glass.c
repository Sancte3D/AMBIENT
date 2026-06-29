/*
 * engine_fm_glass.c — "FM GLASS": a DX7-style 2-operator FM key/glass tone.
 *
 * Calibrated against ambient_world_02_fm_glass_station_dx7_inspired.wav, which
 * measures as a soft, *harmonic* (not bell-inharmonic) hollow tone in the low
 * register (MIDI 40-45), RMS ~0.11, upper harmonics emphasised. The reference's
 * "unmelodic" feel comes from non-integer FM ratios producing inharmonic
 * sidebands — so the fix here is to **lock the carrier:modulator ratio to an
 * integer**, which lands every sideband exactly on the harmonic series.
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
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR          ((float)DSP_SAMPLE_RATE_HZ)
#define TONE_UPDATE 16

enum { G_IDLE = 0, G_ATTACK, G_DECAY, G_SUSTAIN, G_RELEASE };

static struct {
    float car_ph, mod_ph;
    float freq_cur, freq_tgt;
    float glide_coef;
    int   ratio;                 /* integer carrier:modulator ratio */
    /* index (FM brightness) envelope */
    float idx_cur, idx_peak, idx_sustain, idx_coef;
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
    g.freq_cur = g.freq_tgt = 110.0f;
    g.glide_coef = dsp_smooth_coef(0.030f);
    g.ratio      = 2;
    g.idx_peak   = 0.55f;         /* in turns: 0.55 turn ≈ FM index ~3.5 rad (bright) */
    g.idx_sustain= 0.28f;         /* keep upper harmonics alive (glassy, not a pure sine) */
    g.idx_coef   = dsp_smooth_coef(0.30f);
    g.atk_inc    = 1.0f / (0.006f * SR);
    g.dec_coef   = dsp_smooth_coef(0.25f);
    g.sustain    = 0.55f;
    g.rel_coef   = dsp_smooth_coef(0.30f);
    g.cutoff_hz  = 4500.0f;
    g.level      = 0.9f;
    g.send       = 0.16f;        /* glassy → a bit more space than the bass */
    dsp_svf_reset(&g.lp); dsp_svf_set(&g.lp, g.cutoff_hz, 0.707f);
    g.astate = G_IDLE;
}

static void fm_activate(void)   { fm_init(); }
static void fm_deactivate(void) { if (g.astate != G_IDLE) g.astate = G_RELEASE; }
static void fm_panic(void)      { g.amp = 0.0f; g.idx_cur = 0.0f; g.astate = G_IDLE; }

static void fm_note_on(int midi, float vel) {
    float f = dsp_midi_to_hz((float)midi);
    if (g.astate == G_IDLE || g.amp < 1.0e-3f) g.freq_cur = f;   /* snap from silence */
    g.freq_tgt = f;
    /* velocity scales the brightness (harder = brighter), like a DX7 */
    g.idx_cur  = g.idx_peak * (0.5f + 0.5f * dsp_clampf(vel, 0.0f, 1.0f));
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
        case SP_F: g.idx_sustain = v * 0.50f;                              break; /* Body (sustained brightness) */
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

        /* index (brightness) settles from peak toward sustain */
        g.idx_cur += g.idx_coef * (g.idx_sustain - g.idx_cur);

        /* 2-op FM */
        const float dt_c = g.freq_cur / SR;
        const float dt_m = (g.freq_cur * (float)g.ratio) / SR;
        float m  = dsp_sin(g.mod_ph);
        float ph = g.car_ph + g.idx_cur * m;
        ph -= floorf(ph);                      /* wrap to [0,1) for dsp_sin */
        float c  = dsp_sin(ph);

        g.car_ph += dt_c; if (g.car_ph >= 1.0f) g.car_ph -= 1.0f;
        g.mod_ph += dt_m; if (g.mod_ph >= 1.0f) g.mod_ph -= 1.0f;

        if ((g.tctr++ % TONE_UPDATE) == 0) dsp_svf_set(&g.lp, g.cutoff_hz, 0.707f);
        float tone = dsp_svf_lp(&g.lp, c);

        float out = tone * g.amp * g.level;
        dL[n] += out;          dR[n] += out;
        sL[n] += out * g.send; sR[n] += out * g.send;
    }
}

const synth_engine_t engine_fm_glass = {
    .name        = "FM GLASS",
    .init        = fm_init,
    .activate    = fm_activate,
    .deactivate  = fm_deactivate,
    .note_on     = fm_note_on,
    .note_off    = fm_note_off,
    .set_param   = fm_set_param,
    .render_mix  = fm_render_mix,
    .panic       = fm_panic,
};
