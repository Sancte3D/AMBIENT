#ifndef FAM_V2_BEAT_H
#define FAM_V2_BEAT_H

/*
 * beat.c — Crystal-Castles-style drum machine (ADR-0014 r3).
 *
 * User: "nix kirche, sondern entfaltung wie crystal castles das macht mit
 * deren beats!!". So V2 grows a rhythm engine: synth kick + snare/clap +
 * hi-hats on a 16-step grid, driven by the engine's master 16th-note clock.
 * Punchy, slightly lo-fi, melancholic-energetic — the CC bedrock.
 *
 * Three voices, each monophonic (retrigger):
 *   KICK  — sine with a fast 120→48 Hz pitch drop + click transient.
 *   SNARE — bandpassed noise burst + a short 180 Hz body tone.
 *   HAT   — highpassed noise, closed (short) or open (long) per step.
 *
 * The engine calls beat_on_step() once per 16th note, then beat_render_add()
 * per audio block. Patterns are selectable presets.
 */

#include "dsp.h"
#include <stdint.h>

typedef enum {
    BEAT_PATTERN_FOUR = 0,   /* four-on-floor + offbeat hats */
    BEAT_PATTERN_CC,         /* broken Crystal-Castles-ish groove */
    BEAT_PATTERN_HALF,       /* half-time, sparse + heavy */
    BEAT_PATTERN_DRIVE,      /* busy 16th hats, driving kick */
    BEAT_PATTERN_COUNT,
} beat_pattern_t;

typedef struct {
    uint32_t rng;
    int   pattern;
    float amount;            /* 0..1 master level */

    /* kick */
    float k_phase, k_amp, k_pitch;
    /* snare */
    float s_amp, s_tone_phase;
    dsp_svf_t s_bp;
    /* hat */
    float h_amp, h_decay;    /* decay set per hit (closed vs open) */
    dsp_svf_t h_hp;
} beat_t;

void beat_init(beat_t *b, uint32_t seed);
void beat_set_pattern(beat_t *b, int pattern);
void beat_set_amount(beat_t *b, float amt_0_1);

/* Trigger drums scheduled on this 16th-note step (step counts up freely;
 * the pattern is read mod 16). */
void beat_on_step(beat_t *b, int step);

/* Render `frames` of drum audio MIXED-IN to L/R (drums are mostly centred
 * with the hats panned). Also writes a scaled copy into sL/sR for the
 * reverb send (snare/hat tails). */
void beat_render_add(beat_t *b, float *L, float *R, float *sL, float *sR,
                     int frames, float send_scale);

#endif
