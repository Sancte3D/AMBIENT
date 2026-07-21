#include "ambient_effects.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double seconds_now(void)
{
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

int main(void)
{
    AmbientFxConfig config = ambient_fx_default_config();
    size_t arena_bytes = ambient_fx_memory_required(&config);
    void *arena = malloc(arena_bytes);
    if (!arena) {
        fprintf(stderr, "effect benchmark: arena allocation failed\n");
        return 1;
    }

    const unsigned render_seconds = 10u;
    float block[512u * 2u];
    printf("effects host benchmark (relative only; target DWT measurement remains mandatory)\n");
    for (int mode = AMBIENT_FX_DARK_REVERB; mode < AMBIENT_FX_MODE_COUNT; ++mode) {
        AmbientFxStorage storage;
        AmbientFx *fx = ambient_fx_init(&storage, arena, arena_bytes, &config);
        if (!fx) {
            free(arena);
            return 1;
        }
        ambient_fx_set_world(fx, AMBIENT_FX_AFTER_HOURS);
        ambient_fx_set_mode(fx, (AmbientFxMode)mode);
        if (mode == AMBIENT_FX_REVERSE_SWELL || mode == AMBIENT_FX_DREAM_CHAIN) {
            ambient_fx_trigger_reverse_swell(fx, 220.0f, 0.5f, 1.0f);
        }

        float phase_l = 0.0f;
        float phase_r = 0.0f;
        unsigned remaining = AMBIENT_FX_SAMPLE_RATE * render_seconds;
        double start = seconds_now();
        while (remaining) {
            unsigned count = remaining > 512u ? 512u : remaining;
            for (unsigned i = 0u; i < count; ++i) {
                block[i * 2u] = sinf(phase_l) * 0.16f;
                block[i * 2u + 1u] = sinf(phase_r) * 0.13f;
                phase_l += 6.28318530718f * 164.81f / (float)AMBIENT_FX_SAMPLE_RATE;
                phase_r += 6.28318530718f * 246.94f / (float)AMBIENT_FX_SAMPLE_RATE;
                if (phase_l >= 6.28318530718f) phase_l -= 6.28318530718f;
                if (phase_r >= 6.28318530718f) phase_r -= 6.28318530718f;
            }
            ambient_fx_process_f32(fx, block, count);
            remaining -= count;
        }
        double elapsed = seconds_now() - start;
        printf("%-20s %7.2fx realtime (%7.3f s CPU for %u s audio)\n",
               ambient_fx_mode_name((AmbientFxMode)mode),
               elapsed > 0.0 ? (double)render_seconds / elapsed : 0.0,
               elapsed, render_seconds);
    }
    printf("state=%zu bytes, arena=%zu bytes, DMA block=512 frames\n",
           ambient_fx_state_bytes(), arena_bytes);
    free(arena);
    return 0;
}
