#include "ambient_models.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>

static double seconds_now(void)
{
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

int main(void)
{
    const unsigned render_seconds = 5u;
    int16_t block[512 * 2];
    printf("host benchmark (relative smoke test; not an STM32 cycle measurement)\n");
    for (int model = 0; model < AMBIENT_MODEL_COUNT; ++model) {
        AmbientSynth synth;
        ambient_synth_init(&synth, (AmbientModel)model, 0x31415926u + (uint32_t)model);
        ambient_synth_note_on(&synth, 82.41f, 0.7f, -0.7f);
        ambient_synth_note_on(&synth, 123.47f, 0.6f, -0.2f);
        ambient_synth_note_on(&synth, 185.00f, 0.5f, 0.3f);
        ambient_synth_note_on(&synth, 277.18f, 0.4f, 0.7f);
        unsigned remaining = AMBIENT_SAMPLE_RATE * render_seconds;
        double start = seconds_now();
        while (remaining) {
            unsigned n = remaining > 512u ? 512u : remaining;
            ambient_synth_render(&synth, block, n);
            remaining -= n;
        }
        double elapsed = seconds_now() - start;
        printf("%-16s %6.2fx realtime (%7.3f s CPU for %u s audio)\n",
               ambient_model_name((AmbientModel)model),
               elapsed > 0.0 ? (double)render_seconds / elapsed : 0.0,
               elapsed, render_seconds);
    }
    return 0;
}
