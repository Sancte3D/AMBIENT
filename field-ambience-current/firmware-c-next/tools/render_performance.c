/*
 * render_performance.c — a dense, immersive ~5-minute play session.
 *
 * Procedural performance: a continuous melodic line that wanders across the
 * cells (a new note every ~0.4 s, never a silent gap), wechselnde Halte-Akkorde
 * underneath, octave-stack (Shift+Hold, ADR-0008 r2) accents, and live DRIVE /
 * BRIGHTNESS / TEXTURE / MOOD / SPACE automation per section. Deterministic
 * (fixed LCG seed) so it is reproducible, but musically varied.
 *
 * Builds + drives the real engine offline (no SDK), writes stereo 16-bit WAV.
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_performance.c \
 *      src/dsp.c src/pad.c src/texture.c src/bass.c src/drone.c \
 *      src/reverb.c src/reverb_presets.c src/brain.c src/generative.c \
 *      src/cells.c src/engine.c -lm -o /tmp/render_perf
 *   /tmp/render_perf performance.wav
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
#define TOTAL  300                  /* 5 min */

/* Source-ID scheme (pad pool, PAD_MAX=12):
 *   melodic taps      → sources 0..4   (the 5 cells, re-triggered/ringing)
 *   held chord layer  → sources 5,6,7  (sustained under the melody)
 *   generative bed    → source 8        (unused here)
 *   octave-stack (Shift+Hold) accents → sources 9..13  (root+12)            */

static void put_u32(FILE *f,uint32_t v){fputc(v&0xff,f);fputc((v>>8)&0xff,f);fputc((v>>16)&0xff,f);fputc((v>>24)&0xff,f);}
static void put_u16(FILE *f,uint16_t v){fputc(v&0xff,f);fputc((v>>8)&0xff,f);}
static void wav_header(FILE *f,uint32_t nframes){
    uint32_t data=nframes*4u;
    fwrite("RIFF",1,4,f);put_u32(f,36+data);fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f);put_u32(f,16);put_u16(f,1);put_u16(f,2);
    put_u32(f,SR);put_u32(f,SR*4u);put_u16(f,4);put_u16(f,16);
    fwrite("data",1,4,f);put_u32(f,data);
}

/* ---- Section automation: the "scene" that the hands are playing into. ----
 * Each row sets the macro state at time t. -1 = leave unchanged. bright in Hz. */
typedef struct {
    float t; int tex, drive, brightHz, mood, space, bass, drone, mode, vibe, key, voice;
    float density;   /* avg seconds between melodic notes (smaller = denser) */
} scene_t;

static const scene_t SC[] = {
/*    t   tex drv brHz mood spc bas drn mode vibe key  voc  dens */
    { 0,  20,  15,  -60, 35, 45, 12, 0,  0,  0, 60,  0, 0.85f}, /* in: sparse, warm */
    { 14, 40,  22,   40, 45, 50, 22, 1, -1, -1, -1, -1, 0.55f}, /* drone in, denser */
    { 40, 60,  45,  220, 55, 58, 35,-1, -1,  1, -1, -1, 0.42f}, /* brighter, drive up, bright vibe */
    { 80, 78,  62,  330, 62, 66, 45,-1,  1, -1, -1,  1, 0.38f}, /* dorian, strings, busy */
    {120,100,  72,  400, 70, 74, 55,-1, -1, -1, -1, -1, 0.34f}, /* texture 100%, intense */
    {165, 95,  55,  300, 68, 70, 60,-1, -1,  3, 67,  2, 0.36f}, /* key→G, floating, brass — peak energy */
    {210,100,  68,  380, 78, 80, 62,-1,  5,  2, 62,  0, 0.32f}, /* aeolian, deep, very dense climax */
    {255, 70,  40,  120, 55, 82, 45,-1, -1,  3, -1, -1, 0.50f}, /* easing, longer notes */
    {280, 45,  22, -120, 38, 90, 25,-1, -1, -1, -1, -1, 1.10f}, /* dissolve: sparse, dark */
    {292,  0,  12, -260, 30, 90, 10, 0, -1, -1, -1, -1, 9.99f}, /* bed out, let it ring */
};
#define NSC (sizeof SC / sizeof SC[0])

/* Pending note-offs so melodic taps gate + ring rather than pile up forever. */
typedef struct { float t; uint8_t src; } off_t_;
static off_t_ pend[96]; static int npend = 0;
static void sched_off(float t, uint8_t src){ if(npend<96){pend[npend].t=t;pend[npend].src=src;npend++;} }

static uint32_t rng = 0x1A2B3C4Du;
static inline uint32_t rnd(void){ rng = rng*1664525u + 1013904223u; return rng; }
static inline float frnd(void){ return (float)(rnd()>>8) / (float)(1u<<24); }  /* 0..1 */

int main(int argc,char**argv){
    const char *path=(argc>1)?argv[1]:"performance.wav";
    dsp_init(); brain_init(); engine_init();

    FILE *f=fopen(path,"wb"); if(!f){fprintf(stderr,"cannot open %s\n",path);return 1;}
    uint32_t total=(uint32_t)TOTAL*SR; wav_header(f,total);

    int16_t buf[BLOCK*2]; uint32_t frame=0; int peak=0; int min_log=-1;
    size_t sc=0;
    float density=0.85f;
    float next_note=1.2f;            /* first melodic note */
    int   mel_cur=0;                 /* current cell (melodic wander) */
    float next_chord=2.0f;           /* held-chord change cadence */
    int   chord_deg=1;

    while(frame<total){
        float t=(float)frame/SR;

        /* ---- apply due scene rows ---- */
        while(sc<NSC && SC[sc].t<=t){
            const scene_t *s=&SC[sc];
            if(s->tex>=0)    engine_set_texture(s->tex/100.0f);
            if(s->drive>=0)  engine_set_reverb_drive(s->drive/100.0f);
            engine_set_brightness((float)s->brightHz);
            if(s->mood>=0)   engine_set_mood(s->mood/100.0f);
            if(s->space>=0)  engine_set_space(s->space/100.0f);
            if(s->bass>=0)   engine_set_bass_depth(s->bass/100.0f);
            if(s->drone>=0)  engine_set_drone(s->drone!=0);
            if(s->mode>=0)   engine_set_mode(s->mode);
            if(s->vibe>=0)   engine_set_vibe(s->vibe);
            if(s->key>=0)    engine_set_key(s->key);
            if(s->voice>=0)  engine_set_pad_voice(s->voice);
            density=s->density;
            ++sc;
        }

        /* ---- fire due note-offs ---- */
        for(int i=0;i<npend;){
            if(pend[i].t<=t){ engine_note_off(pend[i].src); pend[i]=pend[--npend]; }
            else ++i;
        }

        /* ---- held-chord layer: change chord every ~10-14 s, 3 voices held --- */
        if(t>=next_chord && t<TOTAL-14){
            int notes[6]; int n=brain_chord(chord_deg, notes, 6);
            for(int v=0;v<3;v++){
                uint8_t src=(uint8_t)(5+v);
                engine_note_off(src);                       /* release previous */
                if(v<n) engine_note_on(src, dsp_midi_to_hz((float)notes[v]), 0.085f);
            }
            /* wander the chord degree musically: I, IV, VI, II, V … */
            static const int prog[]={1,4,6,2,5,3,4,1};
            static int pi=0; pi=(pi+1)%(int)(sizeof prog/sizeof prog[0]);
            chord_deg=prog[pi];
            next_chord = t + 10.0f + frnd()*4.0f;
        }

        /* ---- continuous melodic line: a tap roughly every `density` s ---- */
        if(t>=next_note && t<TOTAL-10){
            /* wander to a neighbouring cell (melodic, avoids static repetition) */
            int step=(int)(rnd()%3)-1;                      /* -1,0,+1 */
            if(step==0) step=(rnd()&1)?2:-2;                /* avoid same-note rut */
            mel_cur+=step; if(mel_cur<0)mel_cur+=5; if(mel_cur>4)mel_cur-=5;
            float vel=0.07f + frnd()*0.11f;                /* 0.07..0.18 dynamics */
            int midi=brain_cell_root(mel_cur);
            uint8_t src=(uint8_t)mel_cur;
            engine_note_on(src, dsp_midi_to_hz((float)midi), vel);
            float dur=0.5f + frnd()*1.6f;                  /* note rings 0.5..2.1 s */
            sched_off(t+dur, src);

            /* ~22 %: stack the shift-octave (Shift+Hold accent) for a fat hit */
            if(frnd()<0.22f){
                uint8_t s2=(uint8_t)(9+mel_cur);
                engine_note_on(s2, dsp_midi_to_hz((float)(midi+12)), vel*0.8f);
                sched_off(t+dur*0.8f, s2);
            }

            /* humanised, slightly swung spacing around the section density */
            float jitter=0.80f + frnd()*0.5f;              /* 0.80..1.30 × */
            next_note = t + density*jitter;
        }

        int n=(int)((total-frame)<BLOCK?(total-frame):BLOCK);
        engine_render(buf,n);
        for(int i=0;i<n*2;++i){int a=buf[i]<0?-buf[i]:buf[i]; if(a>peak)peak=a;}
        fwrite(buf,sizeof(int16_t),(size_t)n*2,f);
        frame+=(uint32_t)n;
        int m=(int)t/30; if(m!=min_log){min_log=m;fprintf(stderr,"  render %ds / %ds\n",m*30,TOTAL);}
    }
    fclose(f);
    printf("wrote %s (%ds, stereo 16-bit @ %dHz)\n",path,TOTAL,SR);
    printf("peak = %d/32767 (%.1f dBFS)\n",peak,20.0*log10((double)(peak?peak:1)/32767.0));
    return 0;
}
