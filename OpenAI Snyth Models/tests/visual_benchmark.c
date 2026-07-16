#include "ambient_visuals.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

enum { FRAMES_PER_VISUAL = 360 };

int main(void)
{
    uint8_t framebuffer[AMBIENT_DISPLAY_BYTES];
    uint32_t checksum = 0u;
    double worst_us = 0.0;
    const char *worst_name = "";

    for (int visual_index = 0; visual_index < AMBIENT_VISUAL_COUNT; ++visual_index) {
        AmbientVisual visual = (AmbientVisual)visual_index;
        AmbientVisualState state;
        ambient_visual_init(&state, visual, 0x61c88647u + (uint32_t)visual_index);
        clock_t started = clock();
        for (int frame = 0; frame < FRAMES_PER_VISUAL; ++frame) {
            float t = (float)frame / 24.0f;
            AmbientVisualInput input = {
                .rms = 0.40f + 0.22f * sinf(t * 0.73f),
                .bass = 0.42f + 0.30f * sinf(t * 0.41f + 0.8f),
                .mid = 0.45f + 0.24f * sinf(t * 0.57f + 1.2f),
                .high = 0.38f + 0.28f * sinf(t * 1.17f + 0.2f),
                .centroid = 0.50f + 0.18f * sinf(t * 0.29f),
                .beat_phase = fmodf(t * 0.43f, 1.0f),
                .sound_model = (uint8_t)visual_index
            };
            ambient_visual_render(&state, framebuffer, input);
            checksum ^= (uint32_t)framebuffer[(unsigned)frame % AMBIENT_DISPLAY_BYTES]
                        << ((unsigned)frame & 15u);
        }
        clock_t finished = clock();
        double seconds = (double)(finished - started) / (double)CLOCKS_PER_SEC;
        double us_per_frame = seconds * 1000000.0 / (double)FRAMES_PER_VISUAL;
        if (us_per_frame > worst_us) {
            worst_us = us_per_frame;
            worst_name = ambient_visual_name(visual);
        }
        printf("visual %-18s %8.1f us/frame  %8.1f theoretical fps\n",
               ambient_visual_name(visual), us_per_frame,
               us_per_frame > 0.0 ? 1000000.0 / us_per_frame : 0.0);
    }
    printf("visual benchmark: worst=%s %.1f us/frame, state=%zu/%u bytes, checksum=%08x\n",
           worst_name, worst_us, ambient_visual_state_bytes(),
           AMBIENT_VISUAL_STATE_BUDGET_BYTES, checksum);
    return 0;
}
