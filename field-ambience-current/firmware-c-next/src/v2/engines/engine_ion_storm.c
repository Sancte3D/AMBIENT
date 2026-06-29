/*
 * engine_ion_storm.c — "ION STORM": Alpha-Juno / hoover-style stacked stab.
 *
 * Reference (ambient_world_04_ion_storm_hoover): aggressive, wide, fast — the
 * hoover is a detuned stack of saws + pulse-width-modulated pulses, often with
 * a short downward pitch bend on the attack. Build: 2 detuned saws + 2 PWM
 * pulses (PWM via the saw-minus-shifted-saw trick, LFO-moved width) → SVF
 * lowpass → tanh drive. A per-note pitch blip down gives the hoover "whoom".
 *
 * dsp.h only. Writes mono (host reverb adds the width).
 */
#include "v2/synth_engine.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR    ((float)DSP_SAMPLE_RATE_HZ)
#define F_UPD 16

enum { I_IDLE = 0, I_ATTACK, I_DECAY, I_SUSTAIN, I_RELEASE };

static struct {
    float saw1, saw2;            /* detuned saw phases */
    float pwa1, pwb1, pwa2, pwb2;/* pulse phases (a/b for the PWM difference) */
    float r_dn, r_up;            /* detune ratios */
    float freq_cur, freq_tgt, glide_coef;
    float bend, bend_coef;       /* attack pitch blip (1→0) */
    int   astate;
    float amp, atk_inc, dec_coef, sustain, rel_coef;
    dsp_svf_t lp; int fctr; float cutoff;
    float pwm_lfo, pwm_inc, pwm_depth;
    float detune, drive, level, send;
} s;

static void is_init(void) {
    memset(&s, 0, sizeof s);
    s.freq_cur = s.freq_tgt = 55.0f;       /* A1 */
    s.glide_coef = dsp_smooth_coef(0.020f);
    s.bend_coef  = dsp_smooth_coef(0.020f);
    s.detune     = 16.0f;                  /* cents */
    s.r_dn = powf(2.0f, -s.detune/1200.0f);
    s.r_up = powf(2.0f,  s.detune/1200.0f);
    s.atk_inc  = 1.0f / (0.004f * SR);
    s.dec_coef = dsp_smooth_coef(0.12f);
    s.sustain  = 0.80f;
    s.rel_coef = dsp_smooth_coef(0.12f);
    s.cutoff   = 2200.0f;
    s.pwm_inc  = 0.8f / SR;
    s.pwm_depth= 0.35f;
    s.drive    = 1.6f;
    s.level    = 0.5f;
    s.send     = 0.14f;
    dsp_svf_reset(&s.lp); dsp_svf_set(&s.lp, s.cutoff, 0.9f);
    s.astate = I_IDLE;
}

static void is_activate(void)   { is_init(); }
static void is_deactivate(void) { if (s.astate != I_IDLE) s.astate = I_RELEASE; }
static void is_panic(void)      { s.amp = 0.0f; s.astate = I_IDLE; }

static void is_note_on(int midi, float vel) {
    float f = dsp_midi_to_hz((float)midi);
    if (s.astate == I_IDLE || s.amp < 1.0e-3f) s.freq_cur = f;
    s.freq_tgt = f;
    s.bend     = 0.06f * (0.5f + 0.5f * dsp_clampf(vel,0,1)); /* small downward blip */
    s.astate   = I_ATTACK;
}
static void is_note_off(void) { if (s.astate != I_IDLE) s.astate = I_RELEASE; }

static void is_set_param(synth_param_t p, float v) {
    v = dsp_clampf(v, 0.0f, 1.0f);
    switch (p) {
        case SP_A: s.cutoff = 600.0f + v * 6000.0f;                          break; /* Cutoff */
        case SP_B: s.detune = 4.0f + v * 30.0f;
                   s.r_dn = powf(2.0f,-s.detune/1200.0f);
                   s.r_up = powf(2.0f, s.detune/1200.0f);                    break; /* Detune */
        case SP_C: s.pwm_depth = v * 0.45f;                                  break; /* PWM    */
        case SP_D: s.drive = 1.0f + v * 3.0f;                                break; /* Drive  */
        case SP_E: s.glide_coef = dsp_smooth_coef(0.008f + v * 0.15f);       break; /* Glide  */
        case SP_F: s.pwm_inc = (0.1f + v * 2.0f) / SR;                       break; /* Motion */
        default: break;
    }
}

static void is_render_mix(float *dL, float *dR, float *sL, float *sR, int frames) {
    if (s.astate == I_IDLE && s.amp < 1.0e-4f) return;

    for (int n = 0; n < frames; ++n) {
        s.freq_cur += s.glide_coef * (s.freq_tgt - s.freq_cur);
        s.bend     += s.bend_coef * (0.0f - s.bend);          /* blip decays to 0 */
        float f = s.freq_cur * (1.0f - s.bend);

        if (s.astate == I_ATTACK) {
            s.amp += s.atk_inc;
            if (s.amp >= 1.0f) { s.amp = 1.0f; s.astate = I_DECAY; }
        } else if (s.astate == I_DECAY) {
            s.amp += s.dec_coef * (s.sustain - s.amp);
            if (s.amp <= s.sustain + 1.0e-3f) { s.amp = s.sustain; s.astate = I_SUSTAIN; }
        } else if (s.astate == I_RELEASE) {
            s.amp += s.rel_coef * (0.0f - s.amp);
            if (s.amp < 1.0e-4f) { s.amp = 0.0f; s.astate = I_IDLE; }
        }

        s.pwm_lfo += s.pwm_inc; if (s.pwm_lfo >= 1.0f) s.pwm_lfo -= 1.0f;
        float width = 0.5f + s.pwm_depth * dsp_sin(s.pwm_lfo);

        /* 2 detuned saws */
        float dt1 = (f * s.r_dn) / SR, dt2 = (f * s.r_up) / SR;
        float saw1 = dsp_poly_saw(s.saw1, dt1);
        float saw2 = dsp_poly_saw(s.saw2, dt2);
        s.saw1 += dt1; if (s.saw1 >= 1.0f) s.saw1 -= 1.0f;
        s.saw2 += dt2; if (s.saw2 >= 1.0f) s.saw2 -= 1.0f;

        /* 2 PWM pulses = saw(phase) - saw(phase+width) */
        float dtp = f / SR;
        float p1 = dsp_poly_saw(s.pwa1, dtp) - dsp_poly_saw(s.pwb1, dtp);
        float w2 = 0.5f + s.pwm_depth * dsp_sin(s.pwm_lfo + 0.33f);
        float p2 = dsp_poly_saw(s.pwa2, dtp) - dsp_poly_saw(s.pwb2, dtp);
        s.pwa1 += dtp; if (s.pwa1 >= 1.0f) s.pwa1 -= 1.0f;
        s.pwb1 = s.pwa1 + width; if (s.pwb1 >= 1.0f) s.pwb1 -= 1.0f;
        s.pwa2 += dtp; if (s.pwa2 >= 1.0f) s.pwa2 -= 1.0f;
        s.pwb2 = s.pwa2 + w2;    if (s.pwb2 >= 1.0f) s.pwb2 -= 1.0f;

        float mix = 0.32f * (saw1 + saw2) + 0.22f * (p1 + p2);

        if ((s.fctr++ % F_UPD) == 0) dsp_svf_set(&s.lp, s.cutoff, 0.9f);
        float lp = dsp_svf_lp(&s.lp, mix);
        float driven = tanhf(lp * s.drive);
        float out = driven * s.amp * s.level;
        dL[n] += out;          dR[n] += out;
        sL[n] += out * s.send; sR[n] += out * s.send;
    }
}

const synth_engine_t engine_ion_storm = {
    .name        = "ION STORM",
    .init        = is_init,
    .activate    = is_activate,
    .deactivate  = is_deactivate,
    .note_on     = is_note_on,
    .note_off    = is_note_off,
    .set_param   = is_set_param,
    .render_mix  = is_render_mix,
    .panic       = is_panic,
};
