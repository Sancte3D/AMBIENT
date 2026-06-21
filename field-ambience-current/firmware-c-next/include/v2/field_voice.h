#ifndef FAM_V2_FIELD_VOICE_H
#define FAM_V2_FIELD_VOICE_H

/*
 * field_voice.c — Engine V2 single-voice synthesis layer (ADR-0014 §7).
 *
 * Three voice characters — only the rendering method changes between them.
 * Driven from harmony_field's freq/amp targets one sample at a time.
 *
 *   GLASS    : 2 sines + 1 triangle + leaky FM + faint octave. Digital,
 *              obertonreich, "Licht statt Holz".
 *   TAPE     : bandlimited saw + 1-pole LP + wow + flutter + saturation.
 *              Warm, instabil, organisch.
 *   PARTICLE : sine + bandpass + envelope-modulated micro-grains. Sparkly,
 *              not a sustained tone — driven by an internal grain trigger.
 *
 * All voices share the same dsp_svf_t shaping (Color macro feeds cutoff
 * + saturation).
 */

#include "dsp.h"
#include <stdbool.h>

typedef enum {
    FV_GLASS = 0,
    FV_TAPE,
    FV_PARTICLE,
} fv_type_t;

typedef struct {
    fv_type_t type;
    float phase1, phase2, phase3;
    float wow_phase;       /* tape wow */
    float drift_phase;     /* slow detune */
    dsp_svf_t lp;          /* color filter */
    dsp_svf_t bp;          /* particle bandpass */
    float lp_state;        /* 1-pole tape LP */
    float grain_phase;     /* particle envelope phase */
    float grain_amp;       /* particle envelope amplitude */
    uint32_t rng;          /* per-voice */
} fv_state_t;

void fv_reset(fv_state_t *s, fv_type_t type, uint32_t seed);

/* Render one sample. The voice receives:
 *   freq_hz      — current desired frequency (already detune-applied)
 *   amp          — current amplitude (already glided)
 *   color_0_1    — global Color macro (0=dark, 1=bright)
 *   motion_walk  — current ±1 random-walk modulation (for tape wow / pitch)
 *   motion_slow  — slower modulation source (for filter drift)
 */
float fv_render(fv_state_t *s,
                float freq_hz, float amp,
                float color_0_1,
                float motion_walk, float motion_slow);

#endif
