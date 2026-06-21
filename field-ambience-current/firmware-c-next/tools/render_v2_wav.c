/*
 * render_v2_wav.c — offline-Renderer für Engine V2.
 *
 * Lädt eine kleine „Performance"-Sequenz: durchschwenkt alle 6 Worlds,
 * variiert Center + Macros, holt den Klang aus engine_v2_render() und
 * schreibt einen 44.1 kHz / 16-bit / stereo WAV nach argv[1] (default
 * /tmp/field_v2.wav).
 *
 * Manuelle Kompile-Zeile (CMake hat keinen tools/-Target):
 *
 *   cc -O2 -std=c11 -Iinclude \
 *      tools/render_v2_wav.c \
 *      src/dsp.c src/reverb.c \
 *      src/v2/*.c \
 *      -lm -o /tmp/render_v2 && /tmp/render_v2 /tmp/field_v2.wav
 */

#include "v2/engine_v2.h"
#include "v2/worlds.h"
#include "v2/harmony_field.h"
#include "dsp.h"
#include "audio.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SR        44100
#define BLOCK     AUDIO_BUFFER_FRAMES

/* Minimal RIFF/WAVE writer — PCM int16, stereo, 44.1k. */
static int wav_write(const char *path, const int16_t *samples, uint32_t nframes) {
    FILE *fp = fopen(path, "wb");
    if (!fp) return -1;

    uint32_t data_bytes = nframes * 2u * 2u;     /* stereo, 16-bit */
    uint32_t riff_size  = 36u + data_bytes;
    uint32_t sr   = SR;
    uint32_t br   = sr * 2 * 2;                  /* byte rate */
    uint16_t bs   = 4;                           /* block align */
    uint16_t bps  = 16;
    uint16_t ch   = 2;
    uint16_t fmt  = 1;                           /* PCM */
    uint32_t fmt_sz = 16;

    fwrite("RIFF", 1, 4, fp);
    fwrite(&riff_size, 4, 1, fp);
    fwrite("WAVE", 1, 4, fp);
    fwrite("fmt ", 1, 4, fp);
    fwrite(&fmt_sz, 4, 1, fp);
    fwrite(&fmt, 2, 1, fp);
    fwrite(&ch,  2, 1, fp);
    fwrite(&sr,  4, 1, fp);
    fwrite(&br,  4, 1, fp);
    fwrite(&bs,  2, 1, fp);
    fwrite(&bps, 2, 1, fp);
    fwrite("data", 1, 4, fp);
    fwrite(&data_bytes, 4, 1, fp);
    fwrite(samples, sizeof(int16_t), nframes * 2, fp);

    fclose(fp);
    return 0;
}

/* A short script — each (start_s, end_s, world, center, density, motion,
 * color, blur, texture, glow). Sequenced through the 6 Worlds. */
typedef struct {
    float t_start, t_end;
    int   world;
    int   center;       /* MIDI */
    float density, motion, color, blur, texture, glow;
} seg_t;

/* Crystal-Castles "Entfaltung" arc: ambient intro → the beat drops → gritty
 * half-time → brighter four-on-floor build → full climax → ambient outro. */
static const seg_t SCRIPT[] = {
    /*    start  end  world          center    D     M     C     B     T     G   */
    {   0.0f,  14.0f, WORLD_FOG,        53,  0.40f, 0.30f, 0.40f, 0.60f, 0.20f, 0.10f }, /* intro atmosphere */
    {  14.0f,  30.0f, WORLD_CRYSTAL,    53,  0.45f, 0.60f, 0.65f, 0.40f, 0.30f, 0.35f }, /* beat drops in */
    {  30.0f,  48.0f, WORLD_CRYSTAL,    55,  0.70f, 0.80f, 0.80f, 0.40f, 0.40f, 0.55f }, /* driving, opening up */
    {  48.0f,  68.0f, WORLD_TAPE,       50,  0.60f, 0.55f, 0.45f, 0.65f, 0.45f, 0.25f }, /* gritty half-time */
    {  68.0f,  88.0f, WORLD_DUST,       57,  0.65f, 0.75f, 0.70f, 0.55f, 0.40f, 0.35f }, /* brighter four-on-floor */
    {  88.0f, 110.0f, WORLD_CRYSTAL,    57,  0.85f, 0.90f, 0.88f, 0.45f, 0.45f, 0.70f }, /* climax */
    { 110.0f, 126.0f, WORLD_CRYSTAL,    52,  0.75f, 0.85f, 0.75f, 0.40f, 0.40f, 0.55f }, /* second wave */
    { 126.0f, 150.0f, WORLD_WARM,       55,  0.45f, 0.40f, 0.55f, 0.55f, 0.25f, 0.20f }, /* ambient outro */
};
#define NSEG (int)(sizeof SCRIPT / sizeof SCRIPT[0])

int main(int argc, char **argv) {
    const char *out_path = (argc >= 2) ? argv[1] : "/tmp/field_v2.wav";
    float duration_s = SCRIPT[NSEG - 1].t_end;
    uint32_t total_frames = (uint32_t)(duration_s * SR);

    int16_t *out = (int16_t *)malloc(total_frames * 2 * sizeof(int16_t));
    if (!out) { fprintf(stderr, "OOM\n"); return 1; }

    dsp_init();
    engine_v2_init(0xFEEDF00Du);
    engine_v2_set_master_volume(0.75f);

    int seg_idx = -1;
    uint32_t frames_written = 0;
    int16_t blk[BLOCK * 2];

    while (frames_written < total_frames) {
        float t = (float)frames_written / (float)SR;

        /* Find current segment + apply when entering. */
        int new_idx = -1;
        for (int i = 0; i < NSEG; ++i) {
            if (t >= SCRIPT[i].t_start && t < SCRIPT[i].t_end) { new_idx = i; break; }
        }
        if (new_idx < 0) new_idx = NSEG - 1;
        if (new_idx != seg_idx) {
            const seg_t *s = &SCRIPT[new_idx];
            printf("  t=%.1fs  world=%s  center=%d  D=%.2f M=%.2f C=%.2f B=%.2f T=%.2f G=%.2f\n",
                   (double)t, worlds_get(s->world)->name, s->center,
                   (double)s->density, (double)s->motion, (double)s->color,
                   (double)s->blur, (double)s->texture, (double)s->glow);
            engine_v2_set_world(s->world);
            engine_v2_set_center(s->center);
            engine_v2_set_density(s->density);
            engine_v2_set_motion(s->motion);
            engine_v2_set_color(s->color);
            engine_v2_set_blur(s->blur);
            engine_v2_set_texture(s->texture);
            engine_v2_set_glow(s->glow);
            seg_idx = new_idx;
        }

        uint32_t remaining = total_frames - frames_written;
        uint32_t n = remaining < BLOCK ? remaining : BLOCK;
        engine_v2_render(blk, (int)n);
        memcpy(out + frames_written * 2, blk, n * 2 * sizeof(int16_t));
        frames_written += n;
    }

    if (wav_write(out_path, out, total_frames) != 0) {
        fprintf(stderr, "wav write failed\n");
        free(out);
        return 2;
    }
    printf("Wrote %s — %u frames (%.1fs) stereo 16-bit 44.1k\n",
           out_path, total_frames, (double)total_frames / (double)SR);
    free(out);
    return 0;
}
