/*
 * render_autoplay.c — 3-minute render of the PASSIVE mode (r18.89): press
 * GENERATE, touch nothing. Exercises exactly the device path — the
 * engine_generative_tick() scheduler (humanized bars, Markov degrees,
 * Karplus-Strong sparkle plucks), plus a lived-in macro setting so the new
 * r18.89 colour is audible: AGE 0.45 (hiss + vinyl crackle + saturation),
 * DRIVE 0.30 (master saturator), SPACE 0.62, ATMOS 0.35, world Tokyo.
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_autoplay.c \
 *      src/dsp.c src/dsp_ladder.c src/pad.c src/texture.c src/ambience.c \
 *      src/tape.c src/echo.c src/blur.c src/bass.c src/drone.c \
 *      src/reverb.c src/reverb_presets.c src/brain.c src/worlds.c \
 *      src/generative.c src/cells.c src/pluck.c src/engine.c \
 *      -lm -o /tmp/render_autoplay
 *   /tmp/render_autoplay autoplay.wav
 */

#include "engine.h"
#include "brain.h"
#include "dsp.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SR     44100
#define BLOCK  256
#define TOTAL  300                  /* 5 min — one full composer cycle */

static void put_u32(FILE *f, uint32_t v) { fputc(v & 0xff, f); fputc((v >> 8) & 0xff, f); fputc((v >> 16) & 0xff, f); fputc((v >> 24) & 0xff, f); }
static void put_u16(FILE *f, uint16_t v) { fputc(v & 0xff, f); fputc((v >> 8) & 0xff, f); }

static void wav_header(FILE *f, uint32_t nframes) {
    uint32_t data = nframes * 4u;
    fwrite("RIFF", 1, 4, f); put_u32(f, 36 + data); fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); put_u32(f, 16); put_u16(f, 1); put_u16(f, 2);
    put_u32(f, SR); put_u32(f, SR * 4u); put_u16(f, 4); put_u16(f, 16);
    fwrite("data", 1, 4, f); put_u32(f, data);
}

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "field_ambience_autoplay.wav";
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return 1; }

    dsp_init(); brain_init(); engine_init();

    engine_set_world(0);            /* Tokyo — night, rain */
    engine_set_space(0.62f);
    engine_set_atmosphere(0.35f);
    engine_set_motion(0.55f);
    engine_set_age(0.45f);          /* hiss + vinyl crackle + sat (r18.89) */
    engine_set_drive(0.30f);        /* master saturator colour (r18.89) */
    engine_set_texture(0.20f);
    engine_set_generative(true, -1);   /* Markov autoplay — hands off */

    const uint32_t frames_total = (uint32_t)SR * TOTAL;
    wav_header(f, frames_total);

    int16_t buf[BLOCK * 2];
    uint32_t now_ms = 0, done = 0;
    while (done < frames_total) {
        engine_generative_tick(now_ms);            /* the device UI-loop call */
        int n = (int)(frames_total - done < BLOCK ? frames_total - done : BLOCK);
        engine_render(buf, n);
        fwrite(buf, sizeof(int16_t), (size_t)n * 2, f);
        done  += (uint32_t)n;
        now_ms = (uint32_t)((uint64_t)done * 1000u / SR);
        if ((done % (SR * 30)) < BLOCK)
            fprintf(stderr, "  render %us / %us\n", done / SR, TOTAL);
    }
    fclose(f);
    fprintf(stderr, "wrote %s (%u s stereo 16-bit @ %d Hz)\n", path, TOTAL, SR);
    return 0;
}
