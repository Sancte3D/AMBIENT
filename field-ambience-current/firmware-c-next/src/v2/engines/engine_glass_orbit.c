/*
 * engine_glass_orbit.c — "GLASS ORBIT": wavetable-style morphing tone.
 *
 * Reference (ambient_world_05_glass_orbit_wavetable): a digital harmonic tone
 * whose spectrum *moves* over time (it had the highest spectral-centroid drift
 * of the pack). We approximate the wavetable morph by crossfading a small set
 * of waveforms derived from one phase — sine → triangle → saw → bright pulse —
 * with the morph position swept by a slow LFO (plus note velocity). A gentle
 * lowpass keeps it glassy, not buzzy.
 *
 * dsp.h only; no actual wavetable RAM needed — the "table" is computed.
 */
#include "v2/synth_engine.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR    ((float)DSP_SAMPLE_RATE_HZ)
#define F_UPD 16

enum { O_IDLE = 0, O_ATTACK, O_DECAY, O_SUSTAIN, O_RELEASE };

static struct {
    float ph;
    float freq_cur, freq_tgt, glide_coef;
    int   astate;
    float amp, atk_inc, dec_coef, sustain, rel_coef;
    dsp_svf_t lp; int fctr; float cutoff;
    float morph_lfo, morph_inc, morph_depth, morph_base;
    float level, send;
} o;

static void go_init(void) {
    memset(&o, 0, sizeof o);
    o.freq_cur = o.freq_tgt = 110.0f;
    o.glide_coef = dsp_smooth_coef(0.030f);
    o.atk_inc  = 1.0f / (0.010f * SR);
    o.dec_coef = dsp_smooth_coef(0.30f);
    o.sustain  = 0.7f;
    o.rel_coef = dsp_smooth_coef(0.35f);
    o.cutoff   = 4000.0f;
    o.morph_inc   = 0.35f / SR;       /* slow orbit */
    o.morph_depth = 0.5f;
    o.morph_base  = 0.5f;
    o.level    = 0.7f;
    o.send     = 0.20f;
    dsp_svf_reset(&o.lp); dsp_svf_set(&o.lp, o.cutoff, 0.707f);
    o.astate = O_IDLE;
}

static void go_activate(void)   { go_init(); }
static void go_deactivate(void) { if (o.astate != O_IDLE) o.astate = O_RELEASE; }
static void go_panic(void)      { o.amp = 0.0f; o.astate = O_IDLE; }

static void go_note_on(int midi, float vel) {
    float f = dsp_midi_to_hz((float)midi);
    if (o.astate == O_IDLE || o.amp < 1.0e-3f) o.freq_cur = f;
    o.freq_tgt = f;
    o.morph_base = 0.3f + 0.5f * dsp_clampf(vel,0,1);   /* harder = brighter table pos */
    o.astate   = O_ATTACK;
}
static void go_note_off(void) { if (o.astate != O_IDLE) o.astate = O_RELEASE; }

static void go_set_param(synth_param_t p, float v) {
    v = dsp_clampf(v, 0.0f, 1.0f);
    switch (p) {
        case SP_A: o.morph_base = v;                                       break; /* Wavetable pos */
        case SP_B: o.morph_inc  = (0.05f + v * 1.5f) / SR;                 break; /* Motion        */
        case SP_C: o.cutoff     = 800.0f + v * 8000.0f;                    break; /* Brightness    */
        case SP_D: o.morph_depth= v * 0.5f;                                break; /* Spread        */
        case SP_E: o.glide_coef = dsp_smooth_coef(0.008f + v * 0.15f);     break; /* Glide         */
        case SP_F: o.sustain    = 0.2f + v * 0.75f;                        break; /* Body          */
        default: break;
    }
}

/* crossfade sine → triangle → saw → pulse across morph∈[0,1] */
static float morph_osc(float ph, float dt, float morph) {
    float m = morph * 3.0f;                 /* 0..3 spans 4 tables, 3 segments */
    int seg = (int)m; if (seg > 2) seg = 2;
    float fr = m - seg;
    float w0, w1;
    float sine = dsp_sin(ph);
    float tri  = dsp_tri(ph);
    float saw  = dsp_poly_saw(ph, dt);
    float pul  = dsp_poly_square(ph, dt);
    switch (seg) {
        case 0:  w0 = sine; w1 = tri; break;
        case 1:  w0 = tri;  w1 = saw; break;
        default: w0 = saw;  w1 = pul; break;
    }
    return w0 + fr * (w1 - w0);
}

static void go_render_mix(float *dL, float *dR, float *sL, float *sR, int frames) {
    if (o.astate == O_IDLE && o.amp < 1.0e-4f) return;

    for (int n = 0; n < frames; ++n) {
        o.freq_cur += o.glide_coef * (o.freq_tgt - o.freq_cur);
        if (o.astate == O_ATTACK) {
            o.amp += o.atk_inc;
            if (o.amp >= 1.0f) { o.amp = 1.0f; o.astate = O_DECAY; }
        } else if (o.astate == O_DECAY) {
            o.amp += o.dec_coef * (o.sustain - o.amp);
            if (o.amp <= o.sustain + 1.0e-3f) { o.amp = o.sustain; o.astate = O_SUSTAIN; }
        } else if (o.astate == O_RELEASE) {
            o.amp += o.rel_coef * (0.0f - o.amp);
            if (o.amp < 1.0e-4f) { o.amp = 0.0f; o.astate = O_IDLE; }
        }

        o.morph_lfo += o.morph_inc; if (o.morph_lfo >= 1.0f) o.morph_lfo -= 1.0f;
        float morph = dsp_clampf(o.morph_base + o.morph_depth * dsp_sin(o.morph_lfo), 0.0f, 1.0f);

        float dt = o.freq_cur / SR;
        float osc = morph_osc(o.ph, dt, morph);
        o.ph += dt; if (o.ph >= 1.0f) o.ph -= 1.0f;

        if ((o.fctr++ % F_UPD) == 0) dsp_svf_set(&o.lp, o.cutoff, 0.707f);
        float lp = dsp_svf_lp(&o.lp, osc);
        float out = lp * o.amp * o.level;
        dL[n] += out;          dR[n] += out;
        sL[n] += out * o.send; sR[n] += out * o.send;
    }
}

const synth_engine_t engine_glass_orbit = {
    .name        = "GLASS ORBIT",
    .init        = go_init,
    .activate    = go_activate,
    .deactivate  = go_deactivate,
    .note_on     = go_note_on,
    .note_off    = go_note_off,
    .set_param   = go_set_param,
    .render_mix  = go_render_mix,
    .panic       = go_panic,
};
