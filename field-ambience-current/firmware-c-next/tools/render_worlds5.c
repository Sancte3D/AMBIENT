/*
 * render_worlds5.c — audition the FIVE r19.44 landscape worlds through the
 * REAL current engine (engine_render → DREAM effects chain, per-world macros,
 * remapped ambience). For each world it replicates the device's boot +
 * load_world_preset (push the world's macros), runs generative autoplay, and
 * writes one WAV. Also writes a combined file with all five back-to-back.
 *
 * Build (from firmware-c-next/): see tools/render_worlds5.sh
 */
#include "engine.h"
#include "worlds.h"
#include "dsp.h"
#include "brain.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SR          44100
#define BLOCK       256
#define WORLD_SECS  32

static void put_u32(FILE *f, uint32_t v){ for(int i=0;i<4;++i){ fputc(v&0xff,f); v>>=8; } }
static void put_u16(FILE *f, uint16_t v){ fputc(v&0xff,f); fputc((v>>8)&0xff,f); }
static void wav_header(FILE *f, uint32_t nframes){
    uint32_t data = nframes*4u;
    fwrite("RIFF",1,4,f); put_u32(f,36+data); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); put_u32(f,16); put_u16(f,1); put_u16(f,2);
    put_u32(f,SR); put_u32(f,SR*4u); put_u16(f,4); put_u16(f,16);
    fwrite("data",1,4,f); put_u32(f,data);
}

/* Mirror menu.c::load_world_preset — push a world's macro preset to the engine. */
static void apply_world(int i){
    const world_t *w = worlds_get(i);
    engine_set_world(i);                       /* key/mode/vibe + fx + ambience + colour/bass via bloom */
    engine_set_voice(w->voice);                /* r19.47 per-world character voice */
    engine_set_brightness((float)w->brightness_hz);   /* r19.45 per-world brightness */
    engine_set_space     (w->space_pct   / 100.0f);
    engine_set_shimmer   (w->shimmer_pct / 100.0f);
    engine_set_atmosphere(w->atmos_pct   / 100.0f);
    engine_set_motion    (w->motion_pct  / 100.0f);
    engine_set_age       (w->age_pct     / 100.0f);
    engine_set_echo      (w->echo_pct    / 100.0f);
    engine_set_blur      (w->blur_pct    / 100.0f);
}

int main(int argc, char **argv){
    const char *prefix = argc > 1 ? argv[1] : "/tmp/world5_";
    char path[256];
    int nworlds = worlds_count();

    /* combined file */
    snprintf(path, sizeof path, "%sALL.wav", prefix);
    FILE *fc = fopen(path, "wb");
    uint32_t frames_per = (uint32_t)WORLD_SECS * SR;
    wav_header(fc, frames_per * (uint32_t)nworlds);

    for (int wi = 0; wi < nworlds; ++wi){
        dsp_init(); brain_init(); engine_init();
        apply_world(wi);
        engine_set_generative(true, -1);       /* hands-off autoplay */

        snprintf(path, sizeof path, "%s%d_%s.wav", prefix, wi, worlds_get(wi)->name);
        for (char *p = path; *p; ++p) if (*p == ' ') *p = '_';
        FILE *fw = fopen(path, "wb");
        wav_header(fw, frames_per);

        int16_t buf[BLOCK*2];
        uint32_t now_ms = 0, done = 0;
        while (done < frames_per){
            engine_generative_tick(now_ms);
            engine_render(buf, BLOCK);
            fwrite(buf, sizeof(int16_t), BLOCK*2, fw);
            fwrite(buf, sizeof(int16_t), BLOCK*2, fc);
            done   += BLOCK;
            now_ms += (uint32_t)(BLOCK*1000ull/SR);
        }
        fclose(fw);
        printf("  %-12s key=%2d mode=%d vibe=%d  space=%2d atmos=%2d age=%2d\n",
               worlds_get(wi)->name, worlds_get(wi)->key_midi, worlds_get(wi)->mode,
               worlds_get(wi)->vibe, worlds_get(wi)->space_pct,
               worlds_get(wi)->atmos_pct, worlds_get(wi)->age_pct);
    }
    fclose(fc);
    printf("wrote %d worlds + combined, %d s each\n", nworlds, WORLD_SECS);
    return 0;
}
