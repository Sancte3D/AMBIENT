/*
 * Offline render harness — renders the real engine to a stereo 16-bit WAV on
 * the host, with NO Pico SDK and NO hardware. This is the reference-audio
 * loop (the "export reference audio" step): listen to the WAV on a computer
 * and A/B it against field_ambience_webapp.html. If the WAV matches the
 * webapp, any remaining difference on the device is the hardware chain
 * (DAC / amp / speaker / ground / supply), not the DSP code.
 *
 * It drives the engine exactly like the firmware would (engine_note_on/off,
 * engine_set_*, engine_render) so the output is bit-for-bit what the Pico
 * computes before the I²S/DAC.
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_wav.c \
 *      src/dsp.c src/pad.c src/texture.c src/bass.c src/drone.c \
 *      src/reverb.c src/reverb_presets.c src/brain.c src/generative.c \
 *      src/engine.c -lm -o /tmp/render_wav
 *   /tmp/render_wav field_ambience.wav
 */

#include "engine.h"
#include "brain.h"
#include "dsp.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SR     44100
#define BLOCK  256

static void put_u32(FILE *f, uint32_t v) { fputc(v & 0xff, f); fputc((v >> 8) & 0xff, f); fputc((v >> 16) & 0xff, f); fputc((v >> 24) & 0xff, f); }
static void put_u16(FILE *f, uint16_t v) { fputc(v & 0xff, f); fputc((v >> 8) & 0xff, f); }

static void write_wav_header(FILE *f, uint32_t nframes) {
    uint32_t data_bytes = nframes * 2u * 2u;     /* stereo, 16-bit */
    fwrite("RIFF", 1, 4, f); put_u32(f, 36 + data_bytes);
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); put_u32(f, 16);
    put_u16(f, 1);                               /* PCM */
    put_u16(f, 2);                               /* channels */
    put_u32(f, SR);
    put_u32(f, SR * 2u * 2u);                    /* byte rate */
    put_u16(f, 4);                               /* block align */
    put_u16(f, 16);                              /* bits */
    fwrite("data", 1, 4, f); put_u32(f, data_bytes);
}

/* A simple control-event timeline so the render shows the instrument's
 * character: texture bloom, cells played as a gentle phrase, the drone, and
 * the generative bed. Times are in seconds. */
typedef struct { float t; int kind; int a; int b; } evt_t;
enum { EV_CELL_ON, EV_CELL_OFF, EV_TEXTURE, EV_DRONE, EV_GEN, EV_GEN_STEP, EV_VOICE, EV_KEY };

static const evt_t TIMELINE[] = {
    { 0.0f,  EV_TEXTURE, 20, 0 },     /* texture 0.20 — the ambient bed */
    { 2.0f,  EV_CELL_ON,  0, 0 },     /* phrase: I */
    { 5.0f,  EV_CELL_ON,  2, 0 },     /* + III on top */
    { 8.0f,  EV_CELL_OFF, 0, 0 },
    { 8.0f,  EV_CELL_ON,  3, 0 },     /* IV */
    { 11.0f, EV_CELL_OFF, 2, 0 },
    { 11.0f, EV_CELL_ON,  4, 0 },     /* V */
    { 12.0f, EV_DRONE,    1, 0 },     /* bring in the root drone */
    { 14.0f, EV_CELL_OFF, 3, 0 },
    { 14.0f, EV_CELL_OFF, 4, 0 },
    { 15.0f, EV_VOICE,    1, 0 },     /* glide pad voice warm→strings */
    { 16.0f, EV_CELL_ON,  1, 0 },     /* II */
    { 19.0f, EV_CELL_OFF, 1, 0 },
    { 20.0f, EV_DRONE,    0, 0 },     /* drone out */
    { 30.0f, EV_TEXTURE,  0, 0 },     /* fade texture for the tail */
};
#define N_EVENTS (sizeof TIMELINE / sizeof TIMELINE[0])
#define TOTAL_SECONDS 34

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "field_ambience.wav";

    dsp_init();
    brain_init();
    engine_init();
    /* defaults already: C4 ionian / warm; engine boots texture at 0 */

    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); return 1; }

    uint32_t total_frames = (uint32_t)TOTAL_SECONDS * SR;
    write_wav_header(f, total_frames);

    int16_t buf[BLOCK * 2];
    uint32_t frame = 0;
    size_t ev = 0;
    int peak = 0;

    while (frame < total_frames) {
        float t = (float)frame / SR;
        /* fire any due events */
        while (ev < N_EVENTS && TIMELINE[ev].t <= t) {
            const evt_t *e = &TIMELINE[ev];
            switch (e->kind) {
                case EV_CELL_ON: {
                    int midi = brain_cell_root(e->a);
                    engine_note_on((uint8_t)e->a, dsp_midi_to_hz((float)midi), 0.10f);
                } break;
                case EV_CELL_OFF: engine_note_off((uint8_t)e->a); break;
                case EV_TEXTURE:  engine_set_texture(e->a / 100.0f); break;
                case EV_DRONE: {
                    if (e->a) engine_set_key(60);     /* drone roots on the key */
                    engine_set_drone(e->a != 0);
                } break;
                case EV_VOICE:    engine_set_pad_voice(e->a); break;
                case EV_GEN:      engine_set_generative(e->a != 0, e->b); break;
                case EV_KEY:      engine_set_key(e->a); break;
                default: break;
            }
            ++ev;
        }

        int n = (int)((total_frames - frame) < BLOCK ? (total_frames - frame) : BLOCK);
        engine_render(buf, n);
        for (int i = 0; i < n * 2; ++i) { int a = buf[i] < 0 ? -buf[i] : buf[i]; if (a > peak) peak = a; }
        fwrite(buf, sizeof(int16_t), (size_t)n * 2, f);
        frame += (uint32_t)n;
    }

    fclose(f);
    printf("wrote %s  (%d s, %u frames, stereo 16-bit @ %d Hz)\n",
           path, TOTAL_SECONDS, total_frames, SR);
    printf("peak sample = %d / 32767  (%.1f dBFS)\n",
           peak, 20.0 * log10((double)(peak ? peak : 1) / 32767.0));
    return 0;
}
