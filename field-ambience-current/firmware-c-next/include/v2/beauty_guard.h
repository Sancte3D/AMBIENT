#ifndef FAM_V2_BEAUTY_GUARD_H
#define FAM_V2_BEAUTY_GUARD_H

/*
 * beauty_guard.c — produces NO sound. Sits in the master path and
 * enforces gain safety so the device can never clip or self-overdrive,
 * regardless of how the user sets Density / Texture / Glow.
 *
 * Behaviour:
 *   - Tracks an envelope follower on the peak of |L|, |R|.
 *   - If peak rises above SAFE_PEAK (-3 dBFS) → smoothly drop a makeup
 *     gain stage to bring it back; smooth release.
 *   - Adds a tanh soft-clip stage at -1 dBFS as a last line of defence.
 *
 * The "level protected" message in ADR-0014 UX would fire when guard_active
 * flag turns true for > 200 ms.
 */

#include <stdbool.h>

typedef struct {
    float peak;            /* smoothed peak envelope */
    float gain;            /* current makeup gain, glides toward target */
    float target_gain;
    int   active_ms;       /* how long guard has been compressing */
} beauty_guard_t;

void bg_init(beauty_guard_t *b);
void bg_process(beauty_guard_t *b, float *L, float *R, int frames);

bool bg_is_active(const beauty_guard_t *b);

#endif
