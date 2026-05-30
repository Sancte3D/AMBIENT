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
