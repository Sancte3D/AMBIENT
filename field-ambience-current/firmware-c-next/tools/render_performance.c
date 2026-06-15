/*
 * render_performance.c — a ~5-minute "playing around" performance render.
 *
 * Unlike render_wav.c (a calm 8-min reference travel), this is an ACTIVE
 * play session: someone exploring the instrument — sweeping DRIVE and
 * BRIGHTNESS while playing, holding cells (Hold mode), stacking octaves
 * (Shift+Hold, ADR-0008 r2), pushing TEXTURE to 100 %, riding MOOD.
 *
 * Builds + drives the real engine offline (no SDK), writes stereo 16-bit WAV.
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_performance.c \
 *      src/dsp.c src/pad.c src/texture.c src/bass.c src/drone.c \
 *      src/reverb.c src/reverb_presets.c src/brain.c src/generative.c \
 *      src/engine.c -lm -o /tmp/render_perf
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
#define CELL_AMP 0.12f

/* Source-ID scheme (pad pool, PAD_MAX=12):
 *   base octave of cell c → source c        (0..4)
 *   generative bed        → source 8         (engine-internal)
 *   shift octave of cell c → source c + 9    (9..13)  [root + 12 semitones]  */
#define SHIFT_SRC(c) ((c) + 9)

static void put_u32(FILE *f, uint32_t v){fputc(v&0xff,f);fputc((v>>8)&0xff,f);fputc((v>>16)&0xff,f);fputc((v>>24)&0xff,f);}
static void put_u16(FILE *f, uint16_t v){fputc(v&0xff,f);fputc((v>>8)&0xff,f);}
static void write_wav_header(FILE *f, uint32_t nframes){
    uint32_t data=nframes*4u;
    fwrite("RIFF",1,4,f);put_u32(f,36+data);fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f);put_u32(f,16);put_u16(f,1);put_u16(f,2);
    put_u32(f,SR);put_u32(f,SR*4u);put_u16(f,4);put_u16(f,16);
    fwrite("data",1,4,f);put_u32(f,data);
}

typedef struct { float t; int kind; int a; float b; } evt_t;
enum {
    EV_CELL_ON, EV_CELL_OFF,        /* a=cell, b=amp(0→default) — HOLD until OFF */
    EV_SHIFT_ON, EV_SHIFT_OFF,      /* a=cell — octave-stacked hold (root+12)    */
    EV_TEXTURE, EV_BASS, EV_SPACE, EV_MOOD, EV_DRIVE,  /* a=0..100 (%)           */
    EV_BRIGHTNESS,                  /* b=Hz offset                               */
    EV_DRONE, EV_VOICE, EV_KEY, EV_MODE, EV_VIBE,
    EV_ALL_OFF
};
#define C_I 0
#define C_II 1
#define C_III 2
#define C_IV 3
#define C_V 4
#define C_VI 5
#define C_VII 6

static const evt_t TL[] = {
/* ===== I. Warm-up (0–45) — bed in, first taps, brightness wiggle ===== */
{0,EV_TEXTURE,18,0},{0,EV_SPACE,50,0},{0,EV_MOOD,40,0},{0,EV_BASS,10,0},{0,EV_DRIVE,12,0},
{3,EV_TEXTURE,28,0},
{6,EV_CELL_ON,C_I,0.10f},
{12,EV_CELL_ON,C_V,0.13f},
{18,EV_BRIGHTNESS,0,80.0f},{18,EV_CELL_ON,C_IV,0.11f},
{22,EV_CELL_OFF,C_I,0},
{26,EV_CELL_ON,C_VI,0.12f},
{30,EV_CELL_OFF,C_V,0},{30,EV_CELL_OFF,C_IV,0},
{36,EV_BRIGHTNESS,0,170.0f},
{40,EV_CELL_OFF,C_VI,0},

/* ===== II. Drive + Brightness play (45–105) — riding the knobs ===== */
{46,EV_CELL_ON,C_I,0.12f},
{50,EV_CELL_ON,C_III,0.12f},
{52,EV_DRIVE,28,0},
{56,EV_DRIVE,46,0},
{60,EV_BRIGHTNESS,0,270.0f},
{64,EV_DRIVE,66,0},                 /* push the reverb drive — gritty bloom */
{68,EV_CELL_ON,C_V,0.15f},          /* hard tap on top */
{72,EV_BRIGHTNESS,0,410.0f},        /* very bright */
{76,EV_DRIVE,82,0},
{80,EV_CELL_OFF,C_III,0},
{84,EV_DRIVE,38,0},                 /* pull the drive back down */
{88,EV_BRIGHTNESS,0,120.0f},
{92,EV_CELL_ON,C_IV,0.12f},
{96,EV_CELL_OFF,C_I,0},{96,EV_CELL_OFF,C_V,0},
{100,EV_CELL_OFF,C_IV,0},
{104,EV_DRIVE,20,0},

/* ===== III. Hold mode + texture to 100 % (105–165) — sustained triad ===== */
{106,EV_DRONE,1,0},
{110,EV_CELL_ON,C_I,0.11f},
{114,EV_CELL_ON,C_III,0.11f},
{118,EV_CELL_ON,C_V,0.11f},         /* I–III–V held (Hold mode) */
{122,EV_TEXTURE,60,0},
{128,EV_TEXTURE,85,0},
{134,EV_TEXTURE,100,0},             /* full bed */
{138,EV_BASS,45,0},
{142,EV_MOOD,55,0},
{148,EV_CELL_OFF,C_III,0},
{152,EV_CELL_ON,C_IV,0.11f},        /* swap III→IV (sus colour), still held */
{158,EV_CELL_OFF,C_V,0},
{162,EV_MOOD,62,0},

/* ===== IV. Shift+Hold octave stacks (165–225) — fat, ADR-0008 r2 ===== */
{166,EV_SHIFT_ON,C_I,0},            /* stack +12 on the held I → octave-fat */
{172,EV_SHIFT_ON,C_IV,0},
{176,EV_BRIGHTNESS,0,300.0f},
{180,EV_CELL_ON,C_VI,0.12f},
{184,EV_SHIFT_ON,C_VI,0},           /* I+IV+VI all octave-stacked */
{188,EV_MOOD,70,0},
{192,EV_CELL_OFF,C_I,0},            /* drop base I, leave its shift ringing */
{196,EV_SHIFT_OFF,C_I,0},
{200,EV_CELL_ON,C_V,0.13f},{200,EV_SHIFT_ON,C_V,0},  /* V base+shift together */
{206,EV_DRIVE,55,0},
{210,EV_CELL_OFF,C_IV,0},{210,EV_SHIFT_OFF,C_IV,0},
{214,EV_CELL_OFF,C_VI,0},{214,EV_SHIFT_OFF,C_VI,0},
{220,EV_CELL_OFF,C_V,0},{220,EV_SHIFT_OFF,C_V,0},

/* ===== V. Peak — full stack triad (225–275) ===== */
{226,EV_TEXTURE,100,0},{226,EV_MOOD,75,0},{226,EV_BRIGHTNESS,0,380.0f},{226,EV_DRIVE,58,0},
{230,EV_CELL_ON,C_I,0.12f},{230,EV_CELL_ON,C_III,0.12f},{230,EV_CELL_ON,C_V,0.12f},
{234,EV_SHIFT_ON,C_I,0},{235,EV_SHIFT_ON,C_III,0},{236,EV_SHIFT_ON,C_V,0}, /* octave-stacked triad */
{240,EV_BASS,60,0},
{246,EV_SPACE,82,0},                /* huge hall */
{252,EV_MOOD,80,0},
{258,EV_CELL_OFF,C_III,0},{258,EV_SHIFT_OFF,C_III,0},
{264,EV_CELL_ON,C_VI,0.16f},        /* hard accent */
{268,EV_CELL_OFF,C_I,0},{268,EV_SHIFT_OFF,C_I,0},
{272,EV_CELL_OFF,C_V,0},{272,EV_SHIFT_OFF,C_V,0},{272,EV_CELL_OFF,C_VI,0},

/* ===== VI. Dissolve (275–300) — release into the tail ===== */
{276,EV_DRONE,0,0},
{280,EV_TEXTURE,50,0},
{284,EV_BRIGHTNESS,0,-200.0f},{284,EV_DRIVE,18,0},
{288,EV_TEXTURE,15,0},{288,EV_BASS,10,0},
{292,EV_TEXTURE,0,0},
{294,EV_ALL_OFF,0,0},               /* let the reverb ring out */
};
#define N (sizeof TL / sizeof TL[0])
#define TOTAL 300                    /* 5 min; last ~6 s = pure tail */

int main(int argc, char **argv){
    const char *path = (argc>1)?argv[1]:"performance.wav";
    dsp_init(); brain_init(); engine_init();

    FILE *f = fopen(path,"wb");
    if(!f){fprintf(stderr,"cannot open %s\n",path);return 1;}
    uint32_t total=(uint32_t)TOTAL*SR;
    write_wav_header(f,total);

    int16_t buf[BLOCK*2];
    uint32_t frame=0; size_t ev=0; int peak=0; int min_log=-1;
    while(frame<total){
        float t=(float)frame/SR;
        while(ev<N && TL[ev].t<=t){
            const evt_t *e=&TL[ev];
            int midi;
            switch(e->kind){
                case EV_CELL_ON: midi=brain_cell_root(e->a);
                    engine_note_on((uint8_t)e->a, dsp_midi_to_hz((float)midi), e->b>0?e->b:CELL_AMP); break;
                case EV_CELL_OFF: engine_note_off((uint8_t)e->a); break;
                case EV_SHIFT_ON: midi=brain_cell_root(e->a)+12;
                    engine_note_on((uint8_t)SHIFT_SRC(e->a), dsp_midi_to_hz((float)midi), CELL_AMP*0.85f); break;
                case EV_SHIFT_OFF: engine_note_off((uint8_t)SHIFT_SRC(e->a)); break;
                case EV_TEXTURE: engine_set_texture(e->a/100.0f); break;
                case EV_BASS: engine_set_bass_depth(e->a/100.0f); break;
                case EV_SPACE: engine_set_space(e->a/100.0f); break;
                case EV_MOOD: engine_set_mood(e->a/100.0f); break;
                case EV_DRIVE: engine_set_reverb_drive(e->a/100.0f); break;
                case EV_BRIGHTNESS: engine_set_brightness(e->b); break;
                case EV_DRONE: engine_set_drone(e->a!=0); break;
                case EV_VOICE: engine_set_pad_voice(e->a); break;
                case EV_KEY: engine_set_key(e->a); break;
                case EV_MODE: engine_set_mode(e->a); break;
                case EV_VIBE: engine_set_vibe(e->a); break;
                case EV_ALL_OFF: engine_all_off(); break;
                default: break;
            }
            ++ev;
        }
        int n=(int)((total-frame)<BLOCK?(total-frame):BLOCK);
        engine_render(buf,n);
        for(int i=0;i<n*2;++i){int a=buf[i]<0?-buf[i]:buf[i]; if(a>peak)peak=a;}
        fwrite(buf,sizeof(int16_t),(size_t)n*2,f);
        frame+=(uint32_t)n;
        int m=(int)t/30; if(m!=min_log){min_log=m; fprintf(stderr,"  render %ds / %ds\n",m*30,TOTAL);}
    }
    fclose(f);
    printf("wrote %s (%ds, stereo 16-bit @ %dHz)\n",path,TOTAL,SR);
    printf("peak = %d/32767 (%.1f dBFS)\n",peak,20.0*log10((double)(peak?peak:1)/32767.0));
    return 0;
}
