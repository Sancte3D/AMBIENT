#ifndef FAM_V2_MATERIAL_TEXTURE_H
#define FAM_V2_MATERIAL_TEXTURE_H

/*
 * material_texture.c — three-layer texture engine (ADR-0014 §8).
 *
 *   AIR   high-band noise 3-9 kHz → space without rhythm.
 *   DUST  short bandpass clicks  → micro-motion, weich nicht perkussiv.
 *   BODY  low rumble 80-250 Hz   → gewicht ohne tonale information.
 *
 * The three layers stack independently. Texture macro (0..1) raises the
 * MASTER amount; per-layer character (which one dominates) is set by the
 * active World preset (texture_air_base / dust_base / body_base).
 */

#include "dsp.h"
#include <stdint.h>

typedef struct {
    uint32_t lcg;
    dsp_svf_t air_bp;
    dsp_svf_t body_lp;
    dsp_svf_t dust_bp;
    float dust_env;            /* envelope for transient dust hits */
    float dust_rate_phase;     /* trigger phase */
} mt_state_t;

void mt_init(mt_state_t *m, uint32_t seed);

/* Layer base balance — set by world. Each 0..1. */
void mt_set_layer_balance(mt_state_t *m,
                          float air_base, float dust_base, float body_base);

/* Render `frames` of stereo noise into L/R, MIXED-IN (accumulates with the
 * existing contents). texture_macro 0..1 = master amount. motion_walk feeds
 * tiny pan modulation. */
void mt_render_add(mt_state_t *m, float *L, float *R, int frames,
                   float texture_macro, float motion_walk);

#endif
