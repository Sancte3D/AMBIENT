#ifndef FAM_V2_MOTION_H
#define FAM_V2_MOTION_H

/*
 * motion.c — Engine V2 modulation source (ADR-0014 §Motion Engine).
 *
 * Produces slowly-varying control signals that the field voices, texture
 * and effects pull from for life-without-rhythm. NO fast LFOs: an
 * ambient field needs drift, breath, random walk — not vibrato.
 *
 * Outputs (call after motion_advance() per audio block):
 *   slow1     ∈ [-1, +1]   sine, period ~30 s
 *   slow2     ∈ [-1, +1]   sine, period ~47 s   (incommensurate with slow1)
 *   walk      ∈ [-1, +1]   smoothed random walk, ~10 s autocorrelation
 *   breath    ∈ [ 0,  1]   inhale-exhale half-cosine, period ~13 s
 *   entropy   ∈ [ 0,  1]   one fresh white sample per advance (for dice rolls)
 *
 * Speed scales them all (Motion macro 0..1 → 0.25× .. 4× nominal rate).
 */

#include <stdint.h>

typedef struct {
    uint32_t rng;
    float slow1_phase;     /* turns */
    float slow2_phase;
    float breath_phase;
    float walk_target;     /* random target */
    float walk_value;      /* smoothed */
    float entropy;
    float speed;           /* 0..∞, default 1.0 */
} motion_t;

void  motion_init(motion_t *m, uint32_t seed);
void  motion_set_speed(motion_t *m, float speed);   /* clamped [0.05, 8.0] */
void  motion_advance(motion_t *m, float dt_s);

float motion_slow1(const motion_t *m);
float motion_slow2(const motion_t *m);
float motion_walk(const motion_t *m);
float motion_breath(const motion_t *m);
float motion_entropy(const motion_t *m);

#endif
