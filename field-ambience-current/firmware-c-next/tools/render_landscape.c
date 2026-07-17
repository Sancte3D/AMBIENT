/*
 * render_landscape.c — hear the Landscape cell mode (r19.27) offline.
 *
 * Drives the REAL engine through the REAL landscape.c role state machine: the
 * layer callbacks are wired to the same engine calls the device uses, and a
 * scripted "performance" presses/latches the four audible layers
 * (Drone/Bed/Motif/Atmos) across two worlds. Writes stereo 16-bit WAV.
 * Memory (cell 5) is an event loop, not a timbre, so it is not exercised here.
 *
 * Build: see tools/render_landscape.sh
 */
#include "engine.h"
#include "brain.h"
#include "tuning.h"
#include "landscape.h"
#include "dsp.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SR    44100
#define BLOCK 256

/* ---- Landscape layer interface → real engine calls (device-identical) ---- */
#define BED_AMP   0.16f
#define MOTIF_AMP 0.20f
#define ATMOS_HI  0.72f
static float g_atmos_base = 0.35f;
static void ls_drone(bool on)            { engine_set_drone(on); }
static void ls_bed  (bool on, uint8_t c) {
    if (on) engine_note_on(c, tuning_hz((float)brain_cell_root(c)), BED_AMP);
    else    engine_note_off(c);
}
static void ls_motif(uint8_t c)          {
    engine_motif_strike(tuning_hz((float)brain_cell_root(c) + 12.0f), MOTIF_AMP);
}
static void ls_atmos(bool on)            {
    engine_set_atmosphere(on ? ATMOS_HI : g_atmos_base);
}
static void ls_memory(uint32_t now)      { (void)now; }
static const landscape_iface_t IFACE = { ls_drone, ls_bed, ls_motif, ls_atmos, ls_memory };

/* ---- WAV ---- */
static void pu32(FILE *f,uint32_t v){fputc(v&255,f);fputc((v>>8)&255,f);fputc((v>>16)&255,f);fputc((v>>24)&255,f);}
static void pu16(FILE *f,uint16_t v){fputc(v&255,f);fputc((v>>8)&255,f);}
static void wav_hdr(FILE *f,uint32_t nfr){uint32_t d=nfr*4u;
    fwrite("RIFF",1,4,f);pu32(f,36+d);fwrite("WAVE",1,4,f);fwrite("fmt ",1,4,f);
    pu32(f,16);pu16(f,1);pu16(f,2);pu32(f,SR);pu32(f,SR*4u);pu16(f,4);pu16(f,16);
    fwrite("data",1,4,f);pu32(f,d);}

/* ---- Script: a list of timed layer events (ms). hold=latch. ---- */
typedef enum { EV_WORLD, EV_PRESS, EV_REL, EV_ATMOSBASE } evk_t;
typedef struct { float t; evk_t k; int a; int hold; } ev_t;

static const ev_t SCRIPT[] = {
    /* --- Tokyo City: build the field layer by layer --- */
    { 0.0f, EV_WORLD, 0, 0 }, { 0.0f, EV_ATMOSBASE, 35, 0 },
    { 0.5f, EV_PRESS, 0, 1 },                       /* latch DRONE           */
    { 5.0f, EV_PRESS, 1, 1 },                       /* latch BED (cell 1)    */
    {11.0f, EV_PRESS, 2, 0 }, {11.15f, EV_REL, 2, 0 },   /* one bell, room to bloom */
    {15.0f, EV_PRESS, 3, 1 },                       /* latch ATMOS (wind up) */
    {21.0f, EV_PRESS, 2, 0 }, {21.15f, EV_REL, 2, 0 },   /* a second, distant  */
    {26.0f, EV_PRESS, 3, 1 },                       /* second HOLD = ATMOS off */
    {29.0f, EV_PRESS, 1, 1 },                       /* BED off               */
    {31.0f, EV_PRESS, 0, 1 },                       /* DRONE off             */
    /* --- After Hours: darker minor world --- */
    {34.0f, EV_WORLD, 3, 0 }, {34.0f, EV_ATMOSBASE, 50, 0 },
    {34.5f, EV_PRESS, 0, 1 },                       /* latch DRONE           */
    {38.0f, EV_PRESS, 1, 1 },                       /* latch BED             */
    {42.0f, EV_PRESS, 3, 1 },                       /* latch ATMOS           */
    {47.0f, EV_PRESS, 2, 0 }, {47.15f, EV_REL, 2, 0 },  /* a lone bell, far off */
};
#define NEV (int)(sizeof(SCRIPT)/sizeof(SCRIPT[0]))
#define TOTAL_S 56

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "/tmp/landscape.wav";
    dsp_init();
    engine_init();
    engine_set_space(0.5f); engine_set_brightness(-40.0f);
    landscape_init(&IFACE);

    FILE *f = fopen(path, "wb"); if (!f) { perror("fopen"); return 1; }
    wav_hdr(f, 0);
    uint32_t total = (uint32_t)SR * TOTAL_S, frame = 0; int ev = 0, peak = 1;
    static int16_t buf[BLOCK * 2];

    while (frame < total) {
        float t = (float)frame / SR;
        while (ev < NEV && SCRIPT[ev].t <= t) {
            const ev_t *e = &SCRIPT[ev++];
            uint32_t now = (uint32_t)(e->t * 1000.0f);
            switch (e->k) {
                case EV_WORLD:     engine_set_world(e->a); break;
                case EV_ATMOSBASE: g_atmos_base = e->a / 100.0f;
                                   engine_set_atmosphere(g_atmos_base); break;
                case EV_PRESS:     landscape_press((uint8_t)e->a, e->hold != 0, now); break;
                case EV_REL:       landscape_release((uint8_t)e->a, now); break;
            }
        }
        int n = (int)(total - frame); if (n > BLOCK) n = BLOCK;
        engine_render(buf, n);
        for (int i = 0; i < n * 2; ++i) { int a = buf[i] < 0 ? -buf[i] : buf[i]; if (a > peak) peak = a; }
        fwrite(buf, sizeof(int16_t), (size_t)n * 2, f);
        frame += (uint32_t)n;
    }
    uint32_t nfr = frame;
    fseek(f, 0, SEEK_SET); wav_hdr(f, nfr); fclose(f);
    fprintf(stderr, "wrote %s (%.1fs, peak=%d)\n", path, (double)nfr / SR, peak);
    return 0;
}
