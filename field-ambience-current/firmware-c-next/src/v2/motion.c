/*
 * motion.c — slow modulation source for Engine V2.
 * No allocation, branch-free hot path, deterministic per seed.
 */

#include "v2/motion.h"
#include "dsp.h"
#include <math.h>

/* xorshift32 — deterministic, 1-cycle PRNG, fine for control rate. */
static uint32_t xs32(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *s = x ? x : 0x9E3779B1u;
    return *s;
}
static float urand(uint32_t *s) { return (float)xs32(s) * (1.0f / 4294967296.0f); }
static float srand(uint32_t *s) { return urand(s) * 2.0f - 1.0f; }

void motion_init(motion_t *m, uint32_t seed) {
    m->rng = seed ? seed : 0xA5A5A5A5u;
    m->slow1_phase  = urand(&m->rng);
    m->slow2_phase  = urand(&m->rng);
    m->breath_phase = urand(&m->rng);
    m->walk_target  = srand(&m->rng) * 0.5f;
    m->walk_value   = m->walk_target;
    m->entropy      = urand(&m->rng);
    m->speed        = 1.0f;
}

void motion_set_speed(motion_t *m, float speed) {
    if (speed < 0.05f) speed = 0.05f;
    if (speed > 8.0f)  speed = 8.0f;
    m->speed = speed;
}

void motion_advance(motion_t *m, float dt_s) {
    if (dt_s < 0.0f) dt_s = 0.0f;
    float k = m->speed * dt_s;

    /* slow1: 30 s period nominal */
    m->slow1_phase  += k * (1.0f / 30.0f);
    /* slow2: 47 s period — incommensurate with slow1 */
    m->slow2_phase  += k * (1.0f / 47.0f);
    /* breath: 13 s period */
    m->breath_phase += k * (1.0f / 13.0f);

    /* Wrap into [0,1). */
    m->slow1_phase  -= (float)(int)m->slow1_phase;
    m->slow2_phase  -= (float)(int)m->slow2_phase;
    m->breath_phase -= (float)(int)m->breath_phase;
    if (m->slow1_phase  < 0.0f) m->slow1_phase  += 1.0f;
    if (m->slow2_phase  < 0.0f) m->slow2_phase  += 1.0f;
    if (m->breath_phase < 0.0f) m->breath_phase += 1.0f;

    /* Random walk: pick fresh target every ~3-5 s of internal time,
     * always glide toward it with ~10 s time constant. */
    float retarget_p = k * (1.0f / 4.0f);
    if (urand(&m->rng) < retarget_p) {
        m->walk_target = srand(&m->rng);
    }
    float tau = 10.0f / m->speed;
    float coef = 1.0f - expf(-dt_s / tau);
    m->walk_value += coef * (m->walk_target - m->walk_value);
    if (m->walk_value >  1.0f) m->walk_value =  1.0f;
    if (m->walk_value < -1.0f) m->walk_value = -1.0f;

    /* Fresh entropy sample for dice rolls (consumers call motion_entropy()
     * once per block at most — re-rolling per sample is not the contract). */
    m->entropy = urand(&m->rng);
}

float motion_slow1(const motion_t *m)  { return dsp_sin(m->slow1_phase); }
float motion_slow2(const motion_t *m)  { return dsp_sin(m->slow2_phase); }
float motion_walk(const motion_t *m)   { return m->walk_value; }

/* Breath: half-cosine inhale-exhale, [0,1]. */
float motion_breath(const motion_t *m) {
    float s = dsp_sin(m->breath_phase);
    return 0.5f * (s + 1.0f);
}
float motion_entropy(const motion_t *m) { return m->entropy; }
