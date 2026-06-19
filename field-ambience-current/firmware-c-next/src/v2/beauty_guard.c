/*
 * beauty_guard.c — peak follower + soft makeup compressor + tanh limiter.
 */

#include "v2/beauty_guard.h"
#include <math.h>

#define SR 44100.0f
#define SAFE_PEAK 0.708f      /* ~-3 dBFS */

void bg_init(beauty_guard_t *b) {
    b->peak = 0.0f;
    b->gain = 1.0f;
    b->target_gain = 1.0f;
    b->active_ms = 0;
}

static inline float soft_clip(float x) {
    if (x >  1.5f) return 0.95f;
    if (x < -1.5f) return -0.95f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

void bg_process(beauty_guard_t *b, float *L, float *R, int frames) {
    /* Attack: 5 ms, Release: 250 ms. */
    const float att = 1.0f - expf(-1.0f / (0.005f * SR));
    const float rel = 1.0f - expf(-1.0f / (0.250f * SR));
    /* Gain glide: ~30 ms. */
    const float gcoef = 1.0f - expf(-1.0f / (0.030f * SR));

    int loud_samples = 0;

    for (int i = 0; i < frames; ++i) {
        float aL = L[i] >= 0.0f ? L[i] : -L[i];
        float aR = R[i] >= 0.0f ? R[i] : -R[i];
        float am = aL > aR ? aL : aR;

        float coef = am > b->peak ? att : rel;
        b->peak += coef * (am - b->peak);

        /* If peak * current_gain exceeds safe, target a reduction. */
        float predicted = b->peak * b->gain;
        if (predicted > SAFE_PEAK) {
            b->target_gain = SAFE_PEAK / (b->peak + 1e-9f);
            ++loud_samples;
        } else {
            /* Recover slowly toward 1.0. */
            b->target_gain += 0.0001f * (1.0f - b->target_gain);
        }
        if (b->target_gain > 1.0f)  b->target_gain = 1.0f;
        if (b->target_gain < 0.1f)  b->target_gain = 0.1f;

        b->gain += gcoef * (b->target_gain - b->gain);

        float oL = L[i] * b->gain;
        float oR = R[i] * b->gain;
        L[i] = soft_clip(oL * 1.0f);
        R[i] = soft_clip(oR * 1.0f);
    }

    if (loud_samples > frames / 4) {
        b->active_ms += (int)((float)frames / SR * 1000.0f);
    } else if (b->active_ms > 0) {
        b->active_ms -= (int)((float)frames / SR * 1000.0f);
        if (b->active_ms < 0) b->active_ms = 0;
    }
}

bool bg_is_active(const beauty_guard_t *b) {
    return b->active_ms > 200;
}
