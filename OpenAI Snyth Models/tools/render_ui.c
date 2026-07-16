#include "ambient_palettes.h"
#include "ambient_visuals.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int write_pgm(const char *path, const uint8_t *packed)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fprintf(f, "P5\n%u %u\n255\n", AMBIENT_DISPLAY_WIDTH, AMBIENT_DISPLAY_HEIGHT);
    for (unsigned y = 0; y < AMBIENT_DISPLAY_HEIGHT; ++y) {
        for (unsigned x = 0; x < AMBIENT_DISPLAY_WIDTH; ++x) {
            unsigned index = y * (AMBIENT_DISPLAY_WIDTH / 2u) + x / 2u;
            unsigned nibble = (x & 1u) ? packed[index] & 15u : packed[index] >> 4;
            fputc((int)(nibble * 17u), f);
        }
    }
    int ok = !ferror(f);
    fclose(f);
    return ok;
}

static int write_ppm(const char *path, const uint8_t *packed,
                     AmbientPalette palette, float phase)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fprintf(f, "P6\n%u %u\n255\n", AMBIENT_DISPLAY_WIDTH, AMBIENT_DISPLAY_HEIGHT);
    for (unsigned y = 0; y < AMBIENT_DISPLAY_HEIGHT; ++y) {
        for (unsigned x = 0; x < AMBIENT_DISPLAY_WIDTH; ++x) {
            unsigned index = y * (AMBIENT_DISPLAY_WIDTH / 2u) + x / 2u;
            uint8_t nibble = (uint8_t)((x & 1u) ? packed[index] & 15u : packed[index] >> 4);
            uint8_t r, g, b;
            ambient_palette_rgb(palette, nibble, phase, &r, &g, &b);
            fputc(r, f);
            fputc(g, f);
            fputc(b, f);
        }
    }
    int ok = !ferror(f);
    fclose(f);
    return ok;
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: %s VISUAL_INDEX OUTPUT_DIR FRAMES [PALETTE_INDEX]\n", argv[0]);
        return 2;
    }
    int visual_i = atoi(argv[1]);
    int frames = atoi(argv[3]);
    int palette_i = argc > 4 ? atoi(argv[4]) : -1;
    if (visual_i < 0 || visual_i >= AMBIENT_VISUAL_COUNT || frames < 1 || frames > 1200) {
        fprintf(stderr, "invalid arguments\n");
        return 2;
    }
    if (palette_i >= AMBIENT_PALETTE_COUNT) {
        fprintf(stderr, "invalid palette index\n");
        return 2;
    }
    AmbientVisual visual = (AmbientVisual)visual_i;
    AmbientVisualState state;
    uint8_t framebuffer[AMBIENT_DISPLAY_BYTES];
    ambient_visual_init(&state, visual, 0xa54ff53au + (uint32_t)visual_i * 1237u);

    for (int frame = 0; frame < frames; ++frame) {
        float t = (float)frame / 24.0f;
        float beat = fmodf(t * 0.43f, 1.0f);
        float pulse = expf(-beat * 7.0f);
        AmbientVisualInput input = {
            .rms = 0.30f + pulse * 0.38f + 0.08f * sinf(t * 1.31f),
            .bass = 0.26f + pulse * 0.60f,
            .mid = 0.42f + 0.20f * sinf(t * 0.83f + 0.8f),
            .high = 0.30f + 0.24f * sinf(t * 1.73f + 1.1f),
            .centroid = 0.46f + 0.18f * sinf(t * 0.37f),
            .beat_phase = beat,
            .sound_model = (uint8_t)(visual_i * 2)
        };
        ambient_visual_render(&state, framebuffer, input);
        char path[1024];
        int written;
        if (palette_i >= 0) {
            AmbientPalette palette = (AmbientPalette)palette_i;
            float palette_phase = (float)frame / (float)frames;
            snprintf(path, sizeof(path), "%s/%s-%s_%03d.ppm", argv[2],
                     ambient_visual_slug(visual), ambient_palette_slug(palette), frame);
            written = write_ppm(path, framebuffer, palette, palette_phase);
        } else {
            snprintf(path, sizeof(path), "%s/%s_%03d.pgm", argv[2],
                     ambient_visual_slug(visual), frame);
            written = write_pgm(path, framebuffer);
        }
        if (!written) {
            fprintf(stderr, "could not write %s\n", path);
            return 1;
        }
    }
    if (palette_i >= 0) {
        printf("%s + %s: %d colour frames, %zu visual state bytes, %u framebuffer bytes, %zu LUT bytes\n",
               ambient_visual_name(visual), ambient_palette_name((AmbientPalette)palette_i),
               frames, ambient_visual_state_bytes(), AMBIENT_DISPLAY_BYTES,
               ambient_palette_lut_bytes());
    } else {
        printf("%s: %d frames, %zu state bytes, %u framebuffer bytes\n",
               ambient_visual_name(visual), frames, ambient_visual_state_bytes(),
               AMBIENT_DISPLAY_BYTES);
    }
    return 0;
}
