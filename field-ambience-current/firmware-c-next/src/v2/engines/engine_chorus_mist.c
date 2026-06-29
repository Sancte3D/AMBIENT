/*
 * engine_chorus_mist.c — "CHORUS MIST": Juno-style detuned saw pad + chorus.
 *
 * Reference (ambient_world_03_juno_mist_chorus_pad): a sustained pad on a
 * single low note (C2), warm, slowly moving — the classic Juno sound is a
 * detuned-saw stack run through a stereo BBD chorus. So: NVOX band-limited
 * saws spread by a small detune → SVF lowpass → a 2-tap modulated chorus for
 * the stereo shimmer → slow pad amp envelope.
 *
 * This engine writes a genuine stereo image (L/R differ via the two chorus
 * taps); the host's reverb widens it further. dsp.h only.
 */
#include "v2/synth_engine.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR      ((float)DSP_SAMPLE_RATE_HZ)
#define NVOX    5
#define CH_LEN  2048            /* ~46 ms chorus delay line */
#define F_UPD   16

enum { P_IDLE = 0, P_ATTACK, P_SUSTAIN, P_RELEASE };

static const float SPREAD[NVOX] = { -1.0f, -0.5f, 0.0f, 0.5f, 1.0f };

static struct {
    float ph[NVOX], ratio[NVOX];
    float freq_cur, freq_tgt, glide_coef;
    int   astate;
    float amp, atk_coef, rel_coef;
    dsp_svf_t lp; int fctr; float cutoff;
    float buf[CH_LEN]; int wr;
    float lfo1, lfo2, lfo_inc, depth;
    float detune, level, send;
} c;

static void set_detune(float cents) {
    for (int i = 0; i < NVOX; ++i)
        c.ratio[i] = powf(2.0f, (SPREAD[i] * cents) / 1200.0f);
}

static void cm_init(void) {
    memset(&c, 0, sizeof c);
    c.freq_cur = c.freq_tgt = 65.41f;     /* C2 */
    c.glide_coef = dsp_smooth_coef(0.040f);
    c.atk_coef = dsp_smooth_coef(0.30f);  /* slow pad attack */
    c.rel_coef = dsp_smooth_coef(0.50f);
    c.cutoff   = 1600.0f;
    c.detune   = 14.0f;                   /* cents */
    c.depth    = 0.004f;                  /* chorus depth (s) */
    c.lfo_inc  = 0.6f / SR;               /* ~0.6 Hz chorus LFO */
    c.lfo2     = 0.25f;                   /* quarter-cycle offset for stereo */
    c.level    = 0.5f;                    /* a stacked pad is loud — headroom */
    c.send     = 0.18f;
    set_detune(c.detune);
    dsp_svf_reset(&c.lp); dsp_svf_set(&c.lp, c.cutoff, 0.707f);
    c.astate = P_IDLE;
}

static void cm_activate(void)   { cm_init(); }
static void cm_deactivate(void) { if (c.astate != P_IDLE) c.astate = P_RELEASE; }
static void cm_panic(void)      { c.amp = 0.0f; c.astate = P_IDLE; memset(c.buf,0,sizeof c.buf); }

static void cm_note_on(int midi, float vel) {
    (void)vel;
    float f = dsp_midi_to_hz((float)midi);
    if (c.astate == P_IDLE || c.amp < 1.0e-3f) c.freq_cur = f;
    c.freq_tgt = f;
    c.astate   = P_ATTACK;
}
static void cm_note_off(void) { if (c.astate != P_IDLE) c.astate = P_RELEASE; }

static void cm_set_param(synth_param_t p, float v) {
    v = dsp_clampf(v, 0.0f, 1.0f);
    switch (p) {
        case SP_A: c.cutoff  = 500.0f + v * 5000.0f;                    break; /* Filter   */
        case SP_B: c.detune  = 2.0f + v * 30.0f; set_detune(c.detune);  break; /* Detune   */
        case SP_C: c.depth   = 0.001f + v * 0.008f;                     break; /* Chorus   */
        case SP_D: c.lfo_inc = (0.15f + v * 1.2f) / SR;                 break; /* Motion   */
        case SP_E: c.glide_coef = dsp_smooth_coef(0.008f + v * 0.20f);  break; /* Glide    */
        case SP_F: c.atk_coef = dsp_smooth_coef(0.04f + v * 0.8f);      break; /* Attack   */
        default: break;
    }
}

static float chorus_tap(float lfo) {
    float d = (0.012f + c.depth * (0.5f + 0.5f * dsp_sin(lfo))) * SR;  /* samples */
    float rp = (float)c.wr - d;
    while (rp < 0) rp += CH_LEN;
    int i0 = (int)rp; float fr = rp - i0;
    int i1 = (i0 + 1) % CH_LEN;
    return c.buf[i0] + fr * (c.buf[i1] - c.buf[i0]);
}

static void cm_render_mix(float *dL, float *dR, float *sL, float *sR, int frames) {
    if (c.astate == P_IDLE && c.amp < 1.0e-4f) return;

    for (int n = 0; n < frames; ++n) {
        c.freq_cur += c.glide_coef * (c.freq_tgt - c.freq_cur);
        if (c.astate == P_ATTACK) {
            c.amp += c.atk_coef * (1.0f - c.amp);
            if (c.amp > 0.999f) { c.amp = 1.0f; c.astate = P_SUSTAIN; }
        } else if (c.astate == P_RELEASE) {
            c.amp += c.rel_coef * (0.0f - c.amp);
            if (c.amp < 1.0e-4f) { c.amp = 0.0f; c.astate = P_IDLE; }
        }

        float saws = 0.0f;
        for (int i = 0; i < NVOX; ++i) {
            float dt = (c.freq_cur * c.ratio[i]) / SR;
            saws += dsp_poly_saw(c.ph[i], dt);
            c.ph[i] += dt; if (c.ph[i] >= 1.0f) c.ph[i] -= 1.0f;
        }
        saws *= (1.0f / NVOX);

        if ((c.fctr++ % F_UPD) == 0) dsp_svf_set(&c.lp, c.cutoff, 0.707f);
        float v = dsp_svf_lp(&c.lp, saws);

        c.buf[c.wr] = v;
        float wetL = chorus_tap(c.lfo1);
        float wetR = chorus_tap(c.lfo1 + c.lfo2);
        c.wr = (c.wr + 1) % CH_LEN;
        c.lfo1 += c.lfo_inc; if (c.lfo1 >= 1.0f) c.lfo1 -= 1.0f;

        float a = c.amp * c.level;
        float outL = (0.6f * v + 0.7f * wetL) * a;
        float outR = (0.6f * v + 0.7f * wetR) * a;
        dL[n] += outL;            dR[n] += outR;
        sL[n] += outL * c.send;   sR[n] += outR * c.send;
    }
}

const synth_engine_t engine_chorus_mist = {
    .name        = "CHORUS MIST",
    .init        = cm_init,
    .activate    = cm_activate,
    .deactivate  = cm_deactivate,
    .note_on     = cm_note_on,
    .note_off    = cm_note_off,
    .set_param   = cm_set_param,
    .render_mix  = cm_render_mix,
    .panic       = cm_panic,
};
