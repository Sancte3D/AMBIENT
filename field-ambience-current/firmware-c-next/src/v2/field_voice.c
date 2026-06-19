/*
 * field_voice.c — three voice characters that share the harmony_field API.
 */

#include "v2/field_voice.h"
#include "dsp.h"
#include <math.h>

#define SR 44100.0f

static uint32_t xs32(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *s = x ? x : 0xBADC0DE0u;
    return *s;
}
static float urand_v(uint32_t *s) { return (float)xs32(s) * (1.0f / 4294967296.0f); }

static inline float soft_clip(float x) {
    /* Cheap tanh-alike — preserves slope near 0, gently rolls past ±1. */
    if (x >  3.0f) return 1.0f;
    if (x < -3.0f) return -1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

void fv_reset(fv_state_t *s, fv_type_t type, uint32_t seed) {
    s->type = type;
    s->rng = seed ? seed : 0xF00Du;
    s->phase1 = urand_v(&s->rng);
    s->phase2 = urand_v(&s->rng);
    s->phase3 = urand_v(&s->rng);
    s->wow_phase = urand_v(&s->rng);
    s->drift_phase = urand_v(&s->rng);
    dsp_svf_reset(&s->lp);
    dsp_svf_reset(&s->bp);
    dsp_svf_set(&s->lp, 8000.0f, 0.7f);
    dsp_svf_set(&s->bp, 2000.0f, 4.0f);
    s->lp_state = 0.0f;
    s->grain_phase = urand_v(&s->rng);
    s->grain_amp = 0.0f;
}

/* GLASS: sine fundamental + sine octave + triangle 12th (fifth-above-octave).
 * Tiny FM on the octave via slow drift. */
static float render_glass(fv_state_t *s, float f, float amp,
                          float color, float walk, float slow) {
    float dt = f / SR;
    s->phase1 += dt;                 if (s->phase1 >= 1.0f) s->phase1 -= 1.0f;
    s->phase2 += dt * 2.0f;          if (s->phase2 >= 1.0f) s->phase2 -= 1.0f;
    s->phase3 += dt * 3.0f;          if (s->phase3 >= 1.0f) s->phase3 -= 1.0f;
    s->drift_phase += 1.0f / SR / 7.5f;
    if (s->drift_phase >= 1.0f) s->drift_phase -= 1.0f;

    float fm = dsp_sin(s->drift_phase) * 0.003f;
    float o1 = dsp_sin(s->phase1);
    float o2 = dsp_sin(s->phase2 + fm);
    float o3 = dsp_tri(s->phase3 + slow * 0.01f);

    /* Mix: fundamental dominant, leaky octave, small triangle harmonic. */
    float mix = 0.65f * o1 + 0.22f * o2 + 0.13f * o3;

    /* Color → LP cutoff 2..14 kHz. */
    float fc = 2000.0f + 12000.0f * color * (0.85f + 0.15f * walk);
    dsp_svf_set(&s->lp, fc, 0.65f);
    float y = dsp_svf_lp(&s->lp, mix);
    return y * amp;
}

/* TAPE: bandlimited saw + 1-pole LP + wow (slow pitch warble) + soft sat. */
static float render_tape(fv_state_t *s, float f, float amp,
                         float color, float walk, float slow) {
    /* Wow: 4-6 Hz LFO modulating pitch ±0.5%, plus motion walk adds ±0.3%. */
    s->wow_phase += 4.5f / SR;
    if (s->wow_phase >= 1.0f) s->wow_phase -= 1.0f;
    float wow = dsp_sin(s->wow_phase) * 0.005f + walk * 0.003f;
    float fw = f * (1.0f + wow);
    float dt = fw / SR;

    s->phase1 += dt;
    if (s->phase1 >= 1.0f) s->phase1 -= 1.0f;

    float saw = dsp_poly_saw(s->phase1, dt);
    /* Round the saw with a touch of its own fundamental sine so it reads warm
     * (felt/tape) rather than buzzy. */
    float fund = dsp_sin(s->phase1);
    float src = 0.62f * saw + 0.38f * fund;

    /* Color → 1-pole LP cutoff 300..4200 Hz (kept dark — tape, not synth-lead). */
    float fc = 300.0f + 3900.0f * color * (0.85f + 0.15f * slow);
    if (fc > 9000.0f) fc = 9000.0f;
    float a = 1.0f - expf(-2.0f * 3.14159265f * fc / SR);
    s->lp_state += a * (src - s->lp_state);

    /* Gentle saturation for analog warmth, scaled modestly with color. */
    float drive = 1.0f + color * 0.8f;
    float sat = soft_clip(s->lp_state * drive) / drive;
    return sat * amp;
}

/* PARTICLE: short grains modulated by an internal envelope; pitch quantised
 * to the requested freq. Sparkly dust, not a sustain tone. */
static float render_particle(fv_state_t *s, float f, float amp,
                             float color, float walk, float slow) {
    /* Grain rate scales with motion: ~3-15 Hz triggers. */
    float grain_hz = 3.0f + 12.0f * (0.5f + 0.5f * slow);
    s->grain_phase += grain_hz / SR;
    if (s->grain_phase >= 1.0f) {
        s->grain_phase -= 1.0f;
        s->grain_amp = 1.0f;
        /* Random freq mult ±octave for some grains (color modulates spread). */
        float jitter = (urand_v(&s->rng) - 0.5f) * color * 0.2f;
        s->phase1 = urand_v(&s->rng);
        (void)jitter;
    }
    /* Exponential decay envelope on grain_amp. */
    s->grain_amp *= 0.9986f;

    float dt = f / SR;
    s->phase1 += dt;
    if (s->phase1 >= 1.0f) s->phase1 -= 1.0f;
    float sine = dsp_sin(s->phase1);

    /* Bandpass tuned around f * 3 with Q scaled by color → glittery upper. */
    float bp_fc = f * (2.5f + 1.5f * color) * (1.0f + walk * 0.05f);
    if (bp_fc > 17000.0f) bp_fc = 17000.0f;
    dsp_svf_set(&s->bp, bp_fc, 6.0f);
    float bp = dsp_svf_bp(&s->bp, sine);

    return bp * s->grain_amp * amp * 0.7f;
}

float fv_render(fv_state_t *s,
                float freq_hz, float amp,
                float color, float walk, float slow)
{
    if (freq_hz < 20.0f)  freq_hz = 20.0f;
    if (freq_hz > 18000.0f) freq_hz = 18000.0f;
    if (amp < 0.0f) amp = 0.0f;
    if (color < 0.0f) color = 0.0f;
    if (color > 1.0f) color = 1.0f;

    switch (s->type) {
        case FV_GLASS:    return render_glass(s, freq_hz, amp, color, walk, slow);
        case FV_TAPE:     return render_tape (s, freq_hz, amp, color, walk, slow);
        case FV_PARTICLE: return render_particle(s, freq_hz, amp, color, walk, slow);
        default:          return 0.0f;
    }
}
