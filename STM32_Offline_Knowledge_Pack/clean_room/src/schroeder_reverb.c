// SPDX-License-Identifier: MIT-0
#include "schroeder_reverb.h"

#include <math.h>
#include <string.h>

static float cr_clamp(float value, float minimum, float maximum)
{
    if(value < minimum)
        return minimum;
    if(value > maximum)
        return maximum;
    return value;
}

static size_t cr_scaled_length(size_t base_length,
                               float sample_rate,
                               unsigned int spread,
                               size_t capacity)
{
    float scaled = (float)base_length * (sample_rate / 48000.0f);
    scaled += (float)spread;

    if(scaled < 1.0f)
        scaled = 1.0f;
    if(scaled > (float)capacity)
        scaled = (float)capacity;

    return (size_t)scaled;
}

void cr_schroeder_reverb_reset(cr_schroeder_reverb_t *reverb)
{
    memset(reverb->comb_buffer, 0, sizeof(reverb->comb_buffer));
    memset(reverb->allpass_buffer, 0, sizeof(reverb->allpass_buffer));
    memset(reverb->comb_index, 0, sizeof(reverb->comb_index));
    memset(reverb->allpass_index, 0, sizeof(reverb->allpass_index));
    memset(reverb->comb_damped, 0, sizeof(reverb->comb_damped));
}

void cr_schroeder_reverb_init(cr_schroeder_reverb_t *reverb,
                              float sample_rate,
                              unsigned int spread)
{
    static const size_t comb_base[CR_REVERB_COMB_COUNT]
        = {1447u, 1559u, 1663u, 1787u};
    static const float comb_feedback[CR_REVERB_COMB_COUNT]
        = {0.742f, 0.754f, 0.766f, 0.778f};
    static const size_t allpass_base[CR_REVERB_ALLPASS_COUNT]
        = {421u, 631u};

    memset(reverb, 0, sizeof(*reverb));

    for(size_t i = 0; i < CR_REVERB_COMB_COUNT; ++i)
    {
        reverb->comb_length[i]
            = cr_scaled_length(comb_base[i],
                               sample_rate,
                               spread * (unsigned int)(i + 1u),
                               CR_REVERB_COMB_CAPACITY);
        reverb->comb_feedback[i] = comb_feedback[i];
    }

    for(size_t i = 0; i < CR_REVERB_ALLPASS_COUNT; ++i)
    {
        reverb->allpass_length[i]
            = cr_scaled_length(allpass_base[i],
                               sample_rate,
                               spread * (unsigned int)(i + 1u),
                               CR_REVERB_ALLPASS_CAPACITY);
    }

    reverb->damping = 0.24f;
    reverb->allpass_feedback = 0.53f;
    reverb->wet = 0.28f;
    reverb->dry = 0.72f;
}

void cr_schroeder_reverb_set_mix(cr_schroeder_reverb_t *reverb,
                                 float dry,
                                 float wet)
{
    reverb->dry = cr_clamp(dry, 0.0f, 1.0f);
    reverb->wet = cr_clamp(wet, 0.0f, 1.0f);
}

void cr_schroeder_reverb_set_damping(cr_schroeder_reverb_t *reverb,
                                     float damping)
{
    reverb->damping = cr_clamp(damping, 0.0f, 0.98f);
}

float cr_schroeder_reverb_process(cr_schroeder_reverb_t *reverb,
                                  float input)
{
    float parallel_sum = 0.0f;

    for(size_t i = 0; i < CR_REVERB_COMB_COUNT; ++i)
    {
        const size_t index = reverb->comb_index[i];
        const float delayed = reverb->comb_buffer[i][index];
        const float damped
            = delayed * (1.0f - reverb->damping)
              + reverb->comb_damped[i] * reverb->damping;

        reverb->comb_damped[i] = damped;
        reverb->comb_buffer[i][index]
            = input + damped * reverb->comb_feedback[i];

        reverb->comb_index[i] = index + 1u;
        if(reverb->comb_index[i] >= reverb->comb_length[i])
            reverb->comb_index[i] = 0u;

        parallel_sum += delayed;
    }

    float diffuse = parallel_sum * (1.0f / (float)CR_REVERB_COMB_COUNT);

    for(size_t i = 0; i < CR_REVERB_ALLPASS_COUNT; ++i)
    {
        const size_t index = reverb->allpass_index[i];
        const float delayed = reverb->allpass_buffer[i][index];
        const float next = diffuse + delayed * reverb->allpass_feedback;

        diffuse = delayed - diffuse;
        reverb->allpass_buffer[i][index] = next;

        reverb->allpass_index[i] = index + 1u;
        if(reverb->allpass_index[i] >= reverb->allpass_length[i])
            reverb->allpass_index[i] = 0u;
    }

    const float output = reverb->dry * input + reverb->wet * diffuse;
    return isfinite(output) ? output : 0.0f;
}
