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
