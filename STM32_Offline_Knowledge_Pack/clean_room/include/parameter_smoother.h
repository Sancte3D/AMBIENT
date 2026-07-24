// SPDX-License-Identifier: MIT-0
#ifndef CR_PARAMETER_SMOOTHER_H
#define CR_PARAMETER_SMOOTHER_H

#include <math.h>

typedef struct
{
    float current;
    float target;
    float coefficient;
} cr_parameter_smoother_t;

static inline void cr_parameter_smoother_init(cr_parameter_smoother_t *smoother,
                                               float initial_value,
                                               float time_ms,
                                               float sample_rate)
{
    smoother->current = initial_value;
    smoother->target  = initial_value;

    if(time_ms <= 0.0f || sample_rate <= 0.0f)
    {
        smoother->coefficient = 0.0f;
        return;
    }

    const float samples = 0.001f * time_ms * sample_rate;
    smoother->coefficient = expf(-1.0f / samples);
}

static inline void cr_parameter_smoother_set_target(
    cr_parameter_smoother_t *smoother,
    float target)
{
    smoother->target = target;
}

static inline float cr_parameter_smoother_process(
    cr_parameter_smoother_t *smoother)
{
    const float a = smoother->coefficient;
    smoother->current
        = smoother->target + a * (smoother->current - smoother->target);
    return smoother->current;
}

#endif
