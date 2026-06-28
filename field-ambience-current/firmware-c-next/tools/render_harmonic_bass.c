/*
 * render_harmonic_bass.c — offline audition of the +28 "5th-harmonic" bass.
 *
 * Plays an E-minor electro-house bassline that shows the two characters: the
 * aggressive singing bite of the +28 oscillator, and the bendy portamento
 * slides (legato notes glide; struck notes re-pluck). Drive ramps up across
 * the take so you can hear soft → aggressive.
 *
 * Build + run:
 *   cc -std=c11 -O2 -Iinclude tools/render_harmonic_bass.c \
 *      src/dsp.c src/harmonic_bass.c -lm -o /tmp/render_hb
 *   /tmp/render_hb /tmp/harmonic_bass.wav
 */
#include "dsp.h"
#include "harmonic_bass.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SR     44100
#define BLOCK  256
#define SECS   18

static void u32(FILE *f, uint32_t v){ for(int i=0;i<4;++i) fputc((v>>(8*i))&0xff,f); }
static void u16(FILE *f, uint16_t v){ fputc(v&0xff,f); fputc((v>>8)&0xff,f); }
static void wav_header(FILE *f, uint32_t nf){
    uint32_t data = nf * 4u;
    fwrite("RIFF",1,4,f); u32(f,36+data); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); u32(f,16); u16(f,1); u16(f,2);
    u32(f,SR); u32(f,SR*4u); u16(f,4); u16(f,16);
    fwrite("data",1,4,f); u32(f,data);
}

/* one bar (4 s) of E-minor bassline. type: 0 = struck (release+note → pluck),
 * 1 = legato (note only → glide). midi: E1=28 B1=35 G1=31 D1=26 E2=40. */
typedef struct { float t; int type; int midi; } step_t;
static const step_t BAR[] = {
    { 0.00f, 0, 28 }, { 0.50f, 0, 28 }, { 1.00f, 1, 35 }, { 1.50f, 0, 28 },
    { 2.00f, 0, 31 }, { 2.50f, 1, 26 }, { 3.00f, 0, 28 }, { 3.50f, 1, 40 },
};
#define N_BAR (sizeof BAR / sizeof BAR[0])

int main(int argc, char **argv){
    const char *out = (argc > 1) ? argv[1] : "/tmp/harmonic_bass.wav";
    dsp_init();
    harmonic_bass_init();
    harmonic_bass_set_bite(0.6f);
    harmonic_bass_set_tone(0.45f);
    harmonic_bass_set_glide(0.35f);
    harmonic_bass_set_level(0.95f);

    FILE *f = fopen(out, "wb");
    if (!f){ fprintf(stderr,"open %s\n", out); return 1; }
    const uint32_t total = (uint32_t)SECS * SR;
    wav_header(f, total);

    float dL[BLOCK], dR[BLOCK], sL[BLOCK], sR[BLOCK];
    int16_t o16[BLOCK*2];
    int peak = 0;
    uint32_t frame = 0;

    /* build the absolute event list: 4 bars, each +4 s; drive ramps 0.3→1.0 */
    while (frame < total){
        float t0 = (float)frame / SR;
        float t1 = (float)(frame + BLOCK) / SR;
        /* schedule events that fall in this block */
        for (int bar = 0; bar < 4; ++bar){
            float boff = bar * 4.0f;
            harmonic_bass_set_drive(0.3f + bar * 0.23f);   /* soft → aggressive */
            for (size_t i = 0; i < N_BAR; ++i){
                float et = boff + BAR[i].t;
                if (et >= t0 && et < t1){
                    if (BAR[i].type == 0) harmonic_bass_release();   /* re-pluck */
                    harmonic_bass_note(dsp_midi_to_hz((float)BAR[i].midi));
                }
            }
        }
        int n = (int)((total - frame) < BLOCK ? (total - frame) : BLOCK);
        memset(dL,0,sizeof(float)*n); memset(dR,0,sizeof(float)*n);
        memset(sL,0,sizeof(float)*n); memset(sR,0,sizeof(float)*n);
        harmonic_bass_render_mix(dL, dR, sL, sR, n);
        for (int i=0;i<n;++i){
            float s = dL[i] * 0.9f;          /* the engine self-limits; small headroom only */
            int li = (int)lrintf(s * 32767.0f);
            if (li >  32767) li =  32767;
            if (li < -32768) li = -32768;
            o16[i*2]=(int16_t)li; o16[i*2+1]=(int16_t)li;
            int a = li<0?-li:li; if (a>peak) peak=a;
        }
        fwrite(o16, sizeof(int16_t), (size_t)n*2, f);
        frame += (uint32_t)n;
    }
    fclose(f);
    printf("harmonic_bass → %s  (%d s, peak %d = %.1f dBFS)\n",
           out, SECS, peak, 20.0*log10((double)(peak?peak:1)/32767.0));
    return 0;
}
