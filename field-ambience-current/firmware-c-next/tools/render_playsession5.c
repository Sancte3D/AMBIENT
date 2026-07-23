/*
 * render_playsession5.c — a ~5-minute HUMAN play session across all five
 * landscape worlds, through the REAL engine (r19.47). This is "playing the
 * instrument the way you actually would": pick a world, drop the DRONE in,
 * let GENERATE run the ambient bed + melody (which now sings through each
 * world's CHARACTER VOICE — Open Sea/Fjords = bowed lyra), and over the top a
 * player latches held chords, taps the odd cell, and rides SPACE / ATMOSPHERE /
 * BRIGHTNESS. Every ~60 s the world changes so all five are heard.
 *
 * Nothing pokes engine internals — cells go through engine_note_on/off, chords
 * through the harmonic brain, macros through the public setters (the engine
 * smooths them per block, no zipper). Deterministic (fixed LCG seed).
 *
 * Build: tools/render_playsession5.sh
 */
#include "engine.h"
#include "brain.h"
#include "tuning.h"
#include "worlds.h"
#include "dsp.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SR          44100
#define BLOCK       256
#define WORLD_SECS  60
#define NWORLD_MAX  8

static void put_u32(FILE *f, uint32_t v){ for(int i=0;i<4;++i){ fputc(v&0xff,f); v>>=8; } }
static void put_u16(FILE *f, uint16_t v){ fputc(v&0xff,f); fputc((v>>8)&0xff,f); }
static void wav_header(FILE *f, uint32_t nframes){
    uint32_t data = nframes*4u;
    fwrite("RIFF",1,4,f); put_u32(f,36+data); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); put_u32(f,16); put_u16(f,1); put_u16(f,2);
    put_u32(f,SR); put_u32(f,SR*4u); put_u16(f,4); put_u16(f,16);
    fwrite("data",1,4,f); put_u32(f,data);
}

static uint32_t rng = 0xA17C0DE5u;
static float frand(void){ rng = rng*1664525u + 1013904223u; return (float)(rng>>8)*(1.0f/16777216.0f); }
static int   irand(int lo, int hi){ return lo + (int)(frand()*(float)(hi-lo+1)); }

/* Mirror menu.c::load_world_preset — push a world's full preset (incl. the
 * r19.47 character voice) to the engine, exactly as a world-change does. */
static void apply_world(int i){
    const world_t *w = worlds_get(i);
    engine_set_world(i);
    engine_set_voice(w->voice);
    engine_set_brightness((float)w->brightness_hz);
    engine_set_space     (w->space_pct   / 100.0f);
    engine_set_shimmer   (w->shimmer_pct / 100.0f);
    engine_set_atmosphere(w->atmos_pct   / 100.0f);
    engine_set_motion    (w->motion_pct  / 100.0f);
    engine_set_age       (w->age_pct     / 100.0f);
    engine_set_echo      (w->echo_pct    / 100.0f);
    engine_set_blur      (w->blur_pct    / 100.0f);
}

/* Play a held chord (colour of the current world) on the pad pool. */
static void chord_on(int color, int degree, float amp){
    int notes[4];
    int n = brain_color_chord(degree, color, notes, 4);
    for (int i = 0; i < n && i < 3; ++i)
        engine_note_on((uint8_t)(1 + i), tuning_hz((float)notes[i]), amp);
}
static void chord_off(void){ for (int i=0;i<3;++i) engine_note_off((uint8_t)(1+i)); }

/* A single hand tap on a cell root. */
static void tap(int cell, float amp){
    int root = brain_cell_root(cell);
    engine_note_on(0, tuning_hz((float)root), amp);
}
static void tap_off(void){ engine_note_off(0); }

int main(int argc, char **argv){
    const char *out = argc>1 ? argv[1] : "/tmp/playsession5.wav";
    int nworlds = worlds_count();
    if (nworlds > NWORLD_MAX) nworlds = NWORLD_MAX;

    dsp_init(); brain_init(); engine_init();
    engine_set_master_volume(0.62f);

    FILE *f = fopen(out, "wb");
    const uint32_t blocks_per = (uint32_t)WORLD_SECS * SR / BLOCK;   /* per section */
    uint32_t frames_total = blocks_per * BLOCK * (uint32_t)nworlds;
    wav_header(f, frames_total);

    int16_t buf[BLOCK*2];
    uint64_t frame_ctr = 0;                 /* absolute frames written */
    const uint32_t sec_ms = (uint32_t)WORLD_SECS * 1000u;

    for (int wi = 0; wi < nworlds; ++wi){
        apply_world(wi);
        engine_set_generative(true, -1);      /* let the device autoplay the bed + melody */
        int drone = (wi % 2 == 0);            /* grounded drone on the odd-numbered worlds */
        engine_set_drone(drone);

        int color = worlds_get(wi)->chord_color;

        /* Humanised event schedule for this ~60 s section (ms from section
         * start). A player: bloom in, latch a chord, tap over it, ride the
         * macros, open SPACE near the end, release into the transition. */
        uint64_t sec_frame0 = frame_ctr;
        uint32_t chord1_on  = 6000  + (uint32_t)irand(0,1500);
        uint32_t chord1_off = 20000 + (uint32_t)irand(0,2500);
        uint32_t tap1       = 12000 + (uint32_t)irand(0,1500);
        uint32_t tap1_off   = tap1 + 2200;
        uint32_t tap2       = 16000 + (uint32_t)irand(0,1500);
        uint32_t tap2_off   = tap2 + 2600;
        uint32_t chord2_on  = 30000 + (uint32_t)irand(0,3000);
        uint32_t chord2_off = 46000 + (uint32_t)irand(0,3000);
        uint32_t bright_ev  = 24000 + (uint32_t)irand(0,3000);
        uint32_t space_ev   = 40000 + (uint32_t)irand(0,3000);
        uint32_t quiet_ev   = 54000;
        int fired = 0;   /* bitmask of one-shot events already done */
        float base_bright = (float)worlds_get(wi)->brightness_hz;
        float base_atmos  = worlds_get(wi)->atmos_pct / 100.0f;
        float base_space  = worlds_get(wi)->space_pct / 100.0f;

        for (uint32_t b = 0; b < blocks_per; ++b){
            uint32_t now_ms = (uint32_t)(frame_ctr * 1000ull / SR);
            uint32_t t = (uint32_t)((frame_ctr - sec_frame0) * 1000ull / SR);

            /* --- scheduled human actions --------------------------------- */
            if (!(fired&(1<<0)) && t>=chord1_on){  fired|=1<<0; chord_on(color, irand(0,4), 0.16f); }
            if (!(fired&(1<<1)) && t>=chord1_off){ fired|=1<<1; chord_off(); }
            if (!(fired&(1<<2)) && t>=tap1){       fired|=1<<2; tap(irand(0,4), 0.15f); }
            if (!(fired&(1<<3)) && t>=tap1_off){   fired|=1<<3; tap_off(); }
            if (!(fired&(1<<4)) && t>=tap2){       fired|=1<<4; tap(irand(0,4), 0.14f); }
            if (!(fired&(1<<5)) && t>=tap2_off){   fired|=1<<5; tap_off(); }
            if (!(fired&(1<<6)) && t>=chord2_on){  fired|=1<<6; chord_on(color, irand(0,4), 0.15f); }
            if (!(fired&(1<<7)) && t>=chord2_off){ fired|=1<<7; chord_off(); }
            if (!(fired&(1<<8)) && t>=bright_ev){  fired|=1<<8; engine_set_brightness(base_bright + 180.0f); }
            if (!(fired&(1<<9)) && t>=space_ev){   fired|=1<<9; engine_set_space(base_space + 0.18f > 1.0f ? 1.0f : base_space + 0.18f); }
            if (!(fired&(1<<10)) && t>=(sec_ms-20000)){ fired|=1<<10; engine_set_atmosphere(base_atmos + 0.12f > 1.0f ? 1.0f : base_atmos + 0.12f); }
            if (!(fired&(1<<11)) && t>=quiet_ev){  fired|=1<<11; engine_set_brightness(base_bright); }

            engine_generative_tick(now_ms);
            engine_render(buf, BLOCK);
            fwrite(buf, sizeof(int16_t), BLOCK*2, f);

            frame_ctr += BLOCK;
        }

        /* clean release into the world change (audio pump never stops) */
        chord_off(); tap_off();
    }

    fclose(f);
    printf("wrote %s (%d worlds x %d s = %d:%02d)\n",
           out, nworlds, WORLD_SECS,
           (nworlds*WORLD_SECS)/60, (nworlds*WORLD_SECS)%60);
    return 0;
}
