#ifndef FAM_AMBIENT_MOTION_H
#define FAM_AMBIENT_MOTION_H
/* Ambient voicing V3: slow, incommensurate motion inside every held note.
 * Two sub-audio drifts per voice, level and brightness, with periods of
 * roughly 7-33 s. Rates and phases are drawn per note from a deterministic
 * hash, so no two voices move together and the texture never repeats
 * exactly (the Music for Airports principle at the scale of one note). All
 * rates stay far below the ~4 Hz peak of fluctuation strength; the drifts
 * are continuous, so they add no discontinuity. Bounded, heap-free and
 * main/IRQ safe: one LUT sine per drift per sample. */
#include "dsp.h"
#include <math.h>
#include <stdint.h>

typedef struct { float level_ph, level_inc, tone_ph, tone_inc; } ambient_motion_t;

static inline float ambient_motion_unit(uint32_t *h) {
    uint32_t x=*h+0x9E3779B9u;
    x^=x>>16; x*=0x7feb352du; x^=x>>15; x*=0x846ca68bu; x^=x>>16;
    *h=x; return (float)(x>>8)*(1.0f/16777216.0f);
}
static inline void ambient_motion_prepare(ambient_motion_t *m,uint32_t seed) {
    uint32_t h=seed;
    m->level_inc=(.045f+.100f*ambient_motion_unit(&h))/(float)DSP_SAMPLE_RATE_HZ; /* .045-.145 Hz */
    m->tone_inc =(.030f+.080f*ambient_motion_unit(&h))/(float)DSP_SAMPLE_RATE_HZ; /* .030-.110 Hz */
    m->level_ph=ambient_motion_unit(&h); m->tone_ph=ambient_motion_unit(&h);
}
/* Advance one sample. Returns the level gain (about +-1.4 dB) and writes a
 * brightness offset in Color-shade units, +-depth. */
static inline float ambient_motion_step(ambient_motion_t *m,float depth,float *tone) {
    float level=1.0f+.17f*dsp_sin(m->level_ph);
    *tone=depth*dsp_sin(m->tone_ph);
    m->level_ph+=m->level_inc; if(m->level_ph>=1.0f) m->level_ph-=1.0f;
    m->tone_ph +=m->tone_inc;  if(m->tone_ph >=1.0f) m->tone_ph -=1.0f;
    return level;
}
/* The same drifts at block rate: the value at the current phase, and the
 * value after n samples. A caller interpolates linearly in between; at these
 * rates (<= .145 Hz) that is exact to far below audibility and continuous. */
static inline float ambient_motion_peek(const ambient_motion_t *m,float depth,float *tone) {
    *tone=depth*dsp_sin(m->tone_ph);
    return 1.0f+.17f*dsp_sin(m->level_ph);
}
static inline float ambient_motion_skip(ambient_motion_t *m,float depth,int n,float *tone) {
    m->level_ph+=m->level_inc*(float)n; m->level_ph-=(float)(int)m->level_ph;
    m->tone_ph +=m->tone_inc*(float)n;  m->tone_ph -=(float)(int)m->tone_ph;
    return ambient_motion_peek(m,depth,tone);
}
/* Natural register balance, set once per note: above ~520 Hz a note sits
 * progressively softer (-3 dB per octave), as on acoustic instruments; the
 * ear's 2-5 kHz sensitivity otherwise pushes high tones forward. */
static inline float ambient_register_gain(float hz) {
    return hz>520.0f ? sqrtf(520.0f/hz) : 1.0f;
}
#endif
