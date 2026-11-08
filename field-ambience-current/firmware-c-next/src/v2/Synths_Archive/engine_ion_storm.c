/*
 * engine_ion_storm.c — sustained pulse/reed candidate for the ambient palette.
 * Two gently detuned saws plus two slowly width-modulated pulses feed the
 * existing ladder. No attack pitch bend or chorus: a centred, steady voice
 * rather than Mist's wide ensemble. Product listening acceptance is pending.
 * Fixed mono state; host provides velocity level, shared room and master.
 */
#include "v2/synth_engine.h"
#include "synth_names.h"
#include "dsp.h"
#include "shape.h"
#include "dsp_ladder.h"
#include <math.h>
#include <string.h>

#define SR    ((float)DSP_SAMPLE_RATE_HZ)
#define F_UPD 16

enum { I_IDLE = 0, I_ATTACK, I_DECAY, I_SUSTAIN, I_RELEASE };

static struct {
    float colour_scale, colour_res;
    float saw1, saw2;            /* detuned saw phases */
    float pwa1, pwb1, pwa2, pwb2;/* pulse phases (a/b for the PWM difference) */
    float r_dn, r_up;            /* detune ratios */
    float freq_cur, freq_tgt, glide_coef;
    int   astate;
    float amp, atk_inc, dec_coef, sustain, rel_coef;
    dsp_ladder_t lad; int fctr; float cutoff; float res;
    float pwm_lfo, pwm_inc, pwm_depth;
    float detune, drive, level, send;
} s;

static void is_init(void) {
    memset(&s, 0, sizeof s);
    s.colour_scale = 1.0f;
    s.freq_cur = s.freq_tgt = 55.0f;       /* A1 */
    s.glide_coef = dsp_smooth_coef(0.020f);
    s.detune     = 6.0f;                  /* cents */
    s.r_dn = powf(2.0f, -s.detune/1200.0f);
    s.r_up = powf(2.0f,  s.detune/1200.0f);
    s.atk_inc  = 1.0f / (0.080f * SR);
    s.dec_coef = dsp_smooth_coef(0.12f);
    s.sustain  = 0.80f;
    s.rel_coef = dsp_smooth_coef(0.60f);
    s.cutoff   = 1800.0f;
    s.pwm_inc  = 0.035f / SR;
    s.pwm_depth= 0.081f;
    s.drive    = 1.15f;
    s.res      = 0.25f;        /* retain a little body, avoid a sharp vowel peak */
    s.level    = 0.6f;
    s.send     = 0.14f;
    dsp_ladder_init(&s.lad, SR);
    dsp_ladder_set_res(&s.lad, s.res);
    dsp_ladder_set_drive(&s.lad, s.drive);
    dsp_ladder_set_freq(&s.lad, s.cutoff);
    s.astate = I_IDLE;
}

static void is_activate(void)   { is_init(); }
static void is_deactivate(void) { if (s.astate != I_IDLE) s.astate = I_RELEASE; }
static void is_panic(void)      { s.amp = 0.0f; s.astate = I_IDLE; }

/* Existing glide smooths this target; never re-trigger an envelope. */
static void is_retune_hz(float hz) {
    if (isfinite(hz) && hz >= 20.0f && hz <= 16000.0f)
        s.freq_tgt = hz;
}

static void is_note_on(float midi, float vel) {
    s.atk_inc = 1.0f / (0.080f * shape_attack_scale() * SR);
    s.rel_coef = dsp_smooth_coef(0.60f * shape_release_scale());
    float f = dsp_midi_to_hz((float)midi);
    if (s.astate == I_IDLE || s.amp < 1.0e-3f) s.freq_cur = f;
    s.freq_tgt = f;
    (void)vel; /* The host scales note level; velocity never pulls pitch. */
    s.astate   = I_ATTACK;
}
static void is_note_off(void) { if (s.astate != I_IDLE) s.astate = I_RELEASE; }

static void is_set_param(synth_param_t p, float v) {
    v = dsp_clampf(v, 0.0f, 1.0f);
    switch (p) {
        case SP_A: s.cutoff = 600.0f + v * 6000.0f;                          break; /* Cutoff */
        case SP_B: s.detune = v * 12.0f;
                   s.r_dn = powf(2.0f,-s.detune/1200.0f);
                   s.r_up = powf(2.0f, s.detune/1200.0f);                    break; /* Detune */
        case SP_C: s.pwm_depth = v * 0.18f;                                  break; /* PWM    */
        case SP_D: s.drive = 1.0f + v; dsp_ladder_set_drive(&s.lad, s.drive); break; /* Drive */
        case SP_E: s.glide_coef = dsp_smooth_coef(0.008f + v * 0.15f);       break; /* Glide  */
        case SP_F: s.pwm_inc = (0.015f + v * 0.065f) / SR;                       break; /* Motion */
        default: break;
    }
}

static void is_render_mix(float *dL, float *dR, float *sL, float *sR, int frames) {
    if (s.astate == I_IDLE && s.amp < 1.0e-4f) return;

    for (int n = 0; n < frames; ++n) {
        s.freq_cur += s.glide_coef * (s.freq_tgt - s.freq_cur);
        float f = s.freq_cur; /* no transient offset from the requested tuning */

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

        if ((s.fctr++ % F_UPD) == 0) {
            dsp_ladder_set_freq(&s.lad, dsp_clampf(s.cutoff * s.colour_scale,80.0f,8000.0f));
            dsp_ladder_set_res(&s.lad, s.res + s.colour_res * 1.1f);
        }
        float lp  = dsp_ladder_process(&s.lad, mix);   /* ladder drive = the grit */
        float out = lp * s.amp * s.level;
        dL[n] += out;          dR[n] += out;
        sL[n] += out * s.send; sR[n] += out * s.send;
    }
}

static void is_set_colour(float scale, float res) {
    s.colour_scale = scale; s.colour_res = res;
}

const synth_engine_t engine_ion_storm = {
    .name        = SYNTH_NAME_ION_STORM,
    .init        = is_init,
    .activate    = is_activate,
    .deactivate  = is_deactivate,
    .note_on     = is_note_on,
    .note_off    = is_note_off,
    .set_param   = is_set_param,
    .render_mix  = is_render_mix,
    .panic       = is_panic,
    .set_colour  = is_set_colour,
    .retune_hz = is_retune_hz,
};
