// SPDX-License-Identifier: MIT-0
#ifndef CR_SCHROEDER_REVERB_H
#define CR_SCHROEDER_REVERB_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CR_REVERB_COMB_COUNT 4u
#define CR_REVERB_ALLPASS_COUNT 2u
#define CR_REVERB_COMB_CAPACITY 4096u
#define CR_REVERB_ALLPASS_CAPACITY 1536u

typedef struct
{
    float comb_buffer[CR_REVERB_COMB_COUNT][CR_REVERB_COMB_CAPACITY];
    float allpass_buffer[CR_REVERB_ALLPASS_COUNT][CR_REVERB_ALLPASS_CAPACITY];

    size_t comb_length[CR_REVERB_COMB_COUNT];
    size_t comb_index[CR_REVERB_COMB_COUNT];
    size_t allpass_length[CR_REVERB_ALLPASS_COUNT];
    size_t allpass_index[CR_REVERB_ALLPASS_COUNT];

    float comb_feedback[CR_REVERB_COMB_COUNT];
    float comb_damped[CR_REVERB_COMB_COUNT];
    float damping;
    float allpass_feedback;
    float wet;
    float dry;
} cr_schroeder_reverb_t;

void cr_schroeder_reverb_init(cr_schroeder_reverb_t *reverb,
                              float sample_rate,
                              unsigned int spread);

void cr_schroeder_reverb_reset(cr_schroeder_reverb_t *reverb);

void cr_schroeder_reverb_set_mix(cr_schroeder_reverb_t *reverb,
                                 float dry,
                                 float wet);

void cr_schroeder_reverb_set_damping(cr_schroeder_reverb_t *reverb,
                                     float damping);

float cr_schroeder_reverb_process(cr_schroeder_reverb_t *reverb,
                                  float input);

#ifdef __cplusplus
}
#endif

#endif
