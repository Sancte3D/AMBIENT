/*
 * Shared DSP helpers — sine LUT + math utilities.
 */

#include "dsp.h"
#include <math.h>

/* Explicit Tau so we don't depend on M_PI being defined (it isn't in strict
 * C11; the Pico SDK happens to provide it, but this keeps dsp.c portable). */
#define DSP_TWO_PI 6.283185307179586f

#define LUT_BITS  10
#define LUT_SIZE  (1 << LUT_BITS)         /* 1024 */
#define LUT_MASK  (LUT_SIZE - 1)

static float sine_lut[LUT_SIZE + 1];      /* +1 guard point for interpolation */

void dsp_init(void) {
    for (int i = 0; i <= LUT_SIZE; ++i) {
        sine_lut[i] = sinf(DSP_TWO_PI * (float)i / (float)LUT_SIZE);
    }
}

float dsp_sin(float phase_turns) {
    /* Wrap to [0,1). */
    phase_turns -= (float)(int)phase_turns;     /* truncate toward zero */
    if (phase_turns < 0.0f) phase_turns += 1.0f;

    float fidx = phase_turns * (float)LUT_SIZE;
    int   i    = (int)fidx;
    float frac = fidx - (float)i;
    /* i is in [0, LUT_SIZE-1]; guard point at LUT_SIZE makes interp safe. */
    float a = sine_lut[i];
    float b = sine_lut[i + 1];
    return a + (b - a) * frac;
}

float dsp_midi_to_hz(float midi) {
    return 440.0f * powf(2.0f, (midi - 69.0f) / 12.0f);
}

/* ------------------------------------------------------------------ Step 9 */

/* polyBLEP residual: corrects the step discontinuity of a naive saw/square at
 * the point `t` (turns from the edge), scaled by the phase increment `dt`.
 * Adds a 2-sample-wide polynomial bump that cancels the alias energy. */
static float polyblep(float t, float dt) {
    if (t < dt) {                 /* just after a wrap */
        t /= dt;
        return t + t - t * t - 1.0f;
    } else if (t > 1.0f - dt) {   /* just before a wrap */
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

float dsp_poly_saw(float phase_turns, float dt) {
    float v = 2.0f * phase_turns - 1.0f;     /* naive ramp −1..+1 */
    v -= polyblep(phase_turns, dt);          /* anti-alias the falling wrap */
    return v;
}

float dsp_poly_square(float phase_turns, float dt) {
    float v = phase_turns < 0.5f ? 1.0f : -1.0f;
    v += polyblep(phase_turns, dt);          /* rising edge at phase 0 */
    float t2 = phase_turns + 0.5f;
    if (t2 >= 1.0f) t2 -= 1.0f;
    v -= polyblep(t2, dt);                   /* falling edge at phase 0.5 */
    return v;
}

float dsp_smooth_coef(float tau_s) {
    if (tau_s <= 0.0f) return 1.0f;          /* instant */
    return 1.0f - expf(-1.0f / (tau_s * (float)DSP_SAMPLE_RATE_HZ));
}

void dsp_svf_reset(dsp_svf_t *s) {
    s->ic1eq = s->ic2eq = 0.0f;
    s->a1 = s->a2 = s->a3 = 0.0f;
    s->k = 1.0f;
}

void dsp_svf_set(dsp_svf_t *s, float fc_hz, float q) {
    /* Keep cutoff inside (≈0, Nyquist); tanf blows up approaching Nyquist. */
    if (fc_hz < 10.0f)          fc_hz = 10.0f;
    if (fc_hz > 0.45f * (float)DSP_SAMPLE_RATE_HZ) fc_hz = 0.45f * (float)DSP_SAMPLE_RATE_HZ;
    if (q < 0.20f) q = 0.20f;   /* below this the response is uselessly soft */

    float g = tanf(DSP_TWO_PI * 0.5f * fc_hz / (float)DSP_SAMPLE_RATE_HZ);
    float k = 1.0f / q;
    s->k  = k;
    s->a1 = 1.0f / (1.0f + g * (g + k));
    s->a2 = g * s->a1;
    s->a3 = g * s->a2;
}

float dsp_svf_lp(dsp_svf_t *s, float x) {
    float v3 = x - s->ic2eq;
    float v1 = s->a1 * s->ic1eq + s->a2 * v3;
    float v2 = s->ic2eq + s->a2 * s->ic1eq + s->a3 * v3;
    s->ic1eq = 2.0f * v1 - s->ic1eq;
    s->ic2eq = 2.0f * v2 - s->ic2eq;
    return v2;                  /* lowpass output */
}

float dsp_svf_bp(dsp_svf_t *s, float x) {
    float v3 = x - s->ic2eq;
    float v1 = s->a1 * s->ic1eq + s->a2 * v3;
    float v2 = s->ic2eq + s->a2 * s->ic1eq + s->a3 * v3;
    s->ic1eq = 2.0f * v1 - s->ic1eq;
    s->ic2eq = 2.0f * v2 - s->ic2eq;
    /* v1 is the bandpass with peak gain Q; ·k (=1/Q) normalises to ≈unity. */
    return s->k * v1;
}

float dsp_svf_hp(dsp_svf_t *s, float x) {
    float v3 = x - s->ic2eq;
    float v1 = s->a1 * s->ic1eq + s->a2 * v3;
    float v2 = s->ic2eq + s->a2 * s->ic1eq + s->a3 * v3;
    s->ic1eq = 2.0f * v1 - s->ic1eq;
    s->ic2eq = 2.0f * v2 - s->ic2eq;
    return x - s->k * v1 - v2;   /* highpass = in − k·band − low */
}

/* --- r18.89 noise primitives (see dsp.h for design notes) ---------------- */

static inline float dsp_lcg_white(uint32_t *r) {
    *r = (*r) * 1664525u + 1013904223u;
    return (float)((int32_t)*r) * (1.0f / 2147483648.0f);
}

void dsp_pink_seed(dsp_pink_t *p, uint32_t seed) {
    p->lcg = seed ? seed : 1u;
    p->b0 = p->b1 = p->b2 = 0.0f;
}

float dsp_pink(dsp_pink_t *p) {
    /* Kellet economy coefficients — three one-poles at staggered corners
     * sum to ≈ -3 dB/oct. The 0.1848 output scale lands the peak near ±0.6
     * (comparable to the white input, headroom-safe). */
    float w = dsp_lcg_white(&p->lcg);
    p->b0 = 0.99765f * p->b0 + w * 0.0990460f;
    p->b1 = 0.96300f * p->b1 + w * 0.2965164f;
    p->b2 = 0.57000f * p->b2 + w * 1.0526913f;
    return (p->b0 + p->b1 + p->b2 + w * 0.1848f) * 0.2f;
}

void dsp_dust_seed(dsp_dust_t *d, uint32_t seed) {
    d->lcg = seed ? seed : 1u;
    d->thresh = 0.0f;
}

void dsp_dust_set_density(dsp_dust_t *d, float per_second) {
    if (per_second < 0.0f) per_second = 0.0f;
    d->thresh = per_second / (float)DSP_SAMPLE_RATE_HZ;
}

float dsp_dust(dsp_dust_t *d) {
    d->lcg = d->lcg * 1664525u + 1013904223u;
    float u = (float)(d->lcg >> 8) / 16777216.0f;        /* [0,1) */
    if (u >= d->thresh) return 0.0f;
    /* Impulse amplitude: reuse the fractional position inside the window so
     * hits vary in strength without a second RNG draw. */
    return d->thresh > 0.0f ? (u / d->thresh) * 0.7f + 0.3f : 0.0f;
}

void dsp_crackle_init(dsp_crackle_t *c, float param) {
    if (param < 1.0f)  param = 1.0f;
    if (param > 1.99f) param = 1.99f;
    c->param = param;
    c->y1 = 0.3f;
    c->y2 = 0.0f;
}

float dsp_crackle(dsp_crackle_t *c) {
    /* Chaotic recurrence y' = |y1·p − y2 − 0.05| — the map SC's Crackle
     * documents; chaotic across p ≈ 1..2 (the plain logistic map would need
     * r > 3.57 and CONVERGES in that range — first version of this function
     * did exactly that and fell silent after the transient). */
    float y = fabsf(c->y1 * c->param - c->y2 - 0.05f);
    if (y > 1.0f) y = 1.0f;            /* numeric safety fence */
    float out = c->y1 - c->y2;          /* first difference — removes the DC */
    c->y2 = c->y1;
    c->y1 = y;
    return out * 1.5f;
}
