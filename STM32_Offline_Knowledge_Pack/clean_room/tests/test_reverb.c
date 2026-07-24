// SPDX-License-Identifier: MIT-0
#include "schroeder_reverb.h"

#include <math.h>
#include <stdio.h>

static cr_schroeder_reverb_t reverb;

int main(void)
{
    cr_schroeder_reverb_init(&reverb, 48000.0f, 0u);
    cr_schroeder_reverb_set_mix(&reverb, 0.0f, 1.0f);

    float peak = 0.0f;
    double energy = 0.0;

    for(size_t i = 0; i < 48000u * 4u; ++i)
    {
        const float input = i == 0u ? 1.0f : 0.0f;
        const float output = cr_schroeder_reverb_process(&reverb, input);

        if(!isfinite(output))
        {
            fprintf(stderr, "non-finite output at sample %zu\n", i);
            return 1;
        }

        const float magnitude = fabsf(output);
        if(magnitude > peak)
            peak = magnitude;
        energy += (double)output * (double)output;
    }

    if(peak <= 0.0f || energy <= 0.0)
    {
        fprintf(stderr, "impulse response is empty\n");
        return 2;
    }

    if(peak > 2.0f)
    {
        fprintf(stderr, "unstable peak: %.6f\n", peak);
        return 3;
    }

    printf("ok peak=%.6f energy=%.6f\n", peak, energy);
    return 0;
}
