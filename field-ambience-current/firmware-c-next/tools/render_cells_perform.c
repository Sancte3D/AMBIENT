/*
 * render_cells_perform.c — a ~60 s MUSICAL PERFORMANCE the way the instrument
 * would actually be played, through the full chain we designed:
 *
 *   drone bed (held)                ┐
 *   chords on the 5 cells (Felt)    ├─► dry mix ─┐
 *   sparse melody on top (Air)      ┘            │
 *                    └─ chord+melody ─► GRANULIZER ─► BLUR ─► REVERB ─► wet ─┘
 *
 * Not an A/B: this is one take with a human-ish arc — intimate intro, the
 * player opening the wash as it develops, a fuller peak, a floating outro —
 * with velocity dynamics and the chord COLOUR (vibe family) shifting at
 * musical points (add9 → maj7 → min11 → sus2), the way an encoder would.
 *
 * Voice roles (PAD_MAX = 12):
 *   drone : profile 0 (Bed)   sources 10,11   — low Cm root + 5th, held
 *   chords: profile 1 (Felt)  sources 0..7    — two 4-voice banks, alternating
 *   melody: profile 2 (Air)   source  9       — single high line
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_cells_perform.c \
 *      src/dsp.c src/pad.c src/brain.c src/reverb.c src/v2/diffuser.c \
 *      -lm -o /tmp/render_cells_perform
 *   /tmp/render_cells_perform /tmp/cells_perform.wav
 */

#include "pad.h"
#include "brain.h"
#include "dsp.h"
#include "reverb.h"
#include "v2/diffuser.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SR        44100
#define BLOCK     256
#define PART_SECS 60

/* ----------------------------- performance ----------------------------- */
/* C-dorian (key 60, mode 1). Degrees 1..5 = i, ii, bIII, IV, v. */
typedef struct { float t; int degree, vibe; float vel, hold; } chord_t;
static const chord_t CHORDS[] = {
    {  1.5f, 1, 0, 0.11f, 7.0f },   /* Cm  add9   (intro) */
    {  8.0f, 3, 0, 0.10f, 7.0f },   /* Eb  add9   */
    { 15.0f, 4, 1, 0.12f, 6.0f },   /* F   maj7   (bright enters) */
    { 21.0f, 5, 1, 0.11f, 6.0f },   /* Gm  maj7   */
    { 27.0f, 1, 1, 0.13f, 6.0f },   /* Cm  maj7   */
    { 33.0f, 3, 2, 0.12f, 6.0f },   /* Eb  min11  (deep) */
    { 39.0f, 4, 2, 0.12f, 6.0f },   /* F   min11  */
    { 45.0f, 5, 2, 0.11f, 5.0f },   /* Gm  min11  */
    { 49.5f, 1, 3, 0.10f, 9.0f },   /* Cm  sus2   (floating outro, long) */
};
#define N_CH (sizeof CHORDS / sizeof CHORDS[0])

typedef struct { float t; int midi; float vel, dur; } mel_t;
static const mel_t MELODY[] = {
    { 16.0f, 79, 0.12f, 2.0f }, { 19.0f, 75, 0.10f, 1.5f },
    { 23.0f, 82, 0.11f, 2.5f }, { 28.0f, 77, 0.10f, 2.0f },
    { 31.0f, 79, 0.12f, 2.0f }, { 35.0f, 84, 0.11f, 3.0f },
    { 41.0f, 81, 0.10f, 2.0f }, { 44.0f, 79, 0.11f, 2.0f },
    { 47.0f, 75, 0.10f, 2.5f }, { 53.0f, 72, 0.09f, 5.0f },
};
#define N_MEL (sizeof MELODY / sizeof MELODY[0])

/* the player opening / closing the wash over the take (wet 0..1) */
static float wet_at(float t){
    static const float bt[] = { 0, 12, 18, 34, 45, 52, 60 };
    static const float bv[] = { 0.15f,0.25f,0.55f,0.80f,0.85f,0.70f,0.60f };
    int n = 7;
    if (t <= bt[0]) return bv[0];
    for (int i=1;i<n;i++) if (t<=bt[i]){ float f=(t-bt[i-1])/(bt[i]-bt[i-1]); return bv[i-1]+(bv[i]-bv[i-1])*f; }
    return bv[n-1];
}

typedef struct { float t; uint8_t src; } pend_t;
static pend_t pend[PAD_MAX*2]; static int npend = 0;
static void sched_off(float t, uint8_t s){ if(npend<(int)(sizeof pend/sizeof pend[0])){pend[npend].t=t;pend[npend].src=s;npend++;} }

/* ----------------------------- granular -------------------------------- */
#define GBUF_LEN  (1u<<16)
#define GBUF_MASK (GBUF_LEN-1u)
#define MAXG      40
#define GR_SPACING   900     /* GR spacing — denser cloud for a full take */
#define WAVE_SPACING 900     /* wave spacing — forward read-scan */

typedef struct { int active; float pos, inc; int age, len; float pan, amp; } grain_t;
static float gbufL[GBUF_LEN], gbufR[GBUF_LEN];
static uint32_t gwrite = 0, scan = 0;
static grain_t grains[MAXG];
static int spawn_ctr = 1;
static uint32_t rng = 0x1234567u;
static inline uint32_t xr(void){ rng^=rng<<13; rng^=rng>>17; rng^=rng<<5; return rng; }
static inline float frand(void){ return (xr()>>8)*(1.0f/16777216.0f); }
static inline float gread(const float *b, float pos){
    uint32_t i0=(uint32_t)pos&GBUF_MASK, i1=(i0+1)&GBUF_MASK; float fr=pos-floorf(pos);
    return b[i0]*(1.0f-fr)+b[i1]*fr;
}
static void grain_spawn(void){
    for(int g=0; g<MAXG; ++g){
        if(grains[g].active) continue;
        grain_t *gr=&grains[g];
        gr->active=1; gr->age=0; gr->len=1500+(int)(frand()*3500.0f);
        uint32_t jit=(uint32_t)(frand()*4000.0f);
        gr->pos=(float)((gwrite-(GBUF_LEN-6000)+scan+jit)&GBUF_MASK);
        scan+=WAVE_SPACING; if(scan>(GBUF_LEN-12000)) scan=0;
        float r=frand();
        gr->inc=(r<0.62f)?1.0f:(r<0.82f)?2.0f:(r<0.92f)?0.5f:1.5f;
        gr->pan=frand(); gr->amp=0.55f; return;
    }
}
static void granular_block(const float *inL,const float *inR,float *outL,float *outR,int n){
    for(int i=0;i<n;++i){
        gbufL[gwrite&GBUF_MASK]=inL[i]; gbufR[gwrite&GBUF_MASK]=inR[i]; gwrite++;
        if(--spawn_ctr<=0){ grain_spawn(); spawn_ctr=(int)(GR_SPACING*(0.5f+frand())); }
        float l=0,r=0;
        for(int g=0;g<MAXG;++g){
            grain_t *gr=&grains[g]; if(!gr->active) continue;
            float w=0.5f-0.5f*cosf(6.2831853f*(float)gr->age/(float)gr->len);
            float s=gread(gbufL,gr->pos)*0.5f+gread(gbufR,gr->pos)*0.5f;
            float v=s*w*gr->amp; l+=v*(1.0f-gr->pan); r+=v*gr->pan;
            gr->pos+=gr->inc; if(++gr->age>=gr->len) gr->active=0;
        }
        outL[i]=l; outR[i]=r;
    }
}

int main(int argc, char **argv){
    const char *out=(argc>1)?argv[1]:"/tmp/cells_perform.wav";
    dsp_init();
    brain_init(); brain_set_key(60); brain_set_mode(1);
    pad_init();
    reverb_init(); reverb_set(0.90f,0.40f); reverb_set_drive(0.18f);
    diffuser_t df; diffuser_init(&df); diffuser_set_amount(&df,0.78f);
    memset(gbufL,0,sizeof gbufL); memset(gbufR,0,sizeof gbufR); memset(grains,0,sizeof grains);

    /* drone bed (Bed profile), held nearly the whole take */
    pad_set_profile(0);
    pad_note_on(10, dsp_midi_to_hz(36.0f), 0.11f);   /* C2  */
    pad_note_on(11, dsp_midi_to_hz(43.0f), 0.09f);   /* G2  */
    sched_off(57.0f, 10); sched_off(57.0f, 11);

    FILE *f=fopen(out,"wb"); if(!f){fprintf(stderr,"open %s\n",out);return 1;}
    uint32_t nf=(uint32_t)PART_SECS*SR, data=nf*4u, v; uint16_t h;
    fwrite("RIFF",1,4,f); v=36+data;fwrite(&v,4,1,f); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); v=16;fwrite(&v,4,1,f); h=1;fwrite(&h,2,1,f); h=2;fwrite(&h,2,1,f);
    v=SR;fwrite(&v,4,1,f); v=SR*4u;fwrite(&v,4,1,f); h=4;fwrite(&h,2,1,f); h=16;fwrite(&h,2,1,f);
    fwrite("data",1,4,f); fwrite(&data,4,1,f);

    float dryL[BLOCK],dryR[BLOCK],zL[BLOCK],zR[BLOCK],gL[BLOCK],gR[BLOCK],wL[BLOCK],wR[BLOCK];
    int16_t out16[BLOCK*2];
    uint32_t total=(uint32_t)PART_SECS*SR, frame=0; size_t ci=0, mi=0; int kchord=0;
    int peak=0; double sumsq=0; long sumN=0;

    while(frame<total){
        float t=(float)frame/SR;
        /* chord presses → up to 4 voiced notes on the alternating bank */
        while(ci<N_CH && CHORDS[ci].t<=t){
            const chord_t *c=&CHORDS[ci]; int bank=(kchord%2)*4;
            brain_set_vibe(c->vibe); pad_set_profile(1);
            int notes[BRAIN_MAX_CHORD], nn=brain_chord(c->degree,notes,BRAIN_MAX_CHORD);
            if(nn>4) nn=4;                                  /* cap → fits voice budget */
            float per=c->vel/sqrtf((float)(nn>0?nn:1));
            for(int j=0;j<nn;++j){ uint8_t s=(uint8_t)(bank+j); pad_note_on(s,dsp_midi_to_hz((float)notes[j]),per); sched_off(t+c->hold,s); }
            ++kchord; ++ci;
        }
        /* melody taps (Air) on the dedicated source */
        while(mi<N_MEL && MELODY[mi].t<=t){
            const mel_t *m=&MELODY[mi]; pad_set_profile(2);
            pad_note_on(9, dsp_midi_to_hz((float)m->midi), m->vel); sched_off(t+m->dur,9); ++mi;
        }
        for(int i=0;i<npend;){ if(pend[i].t<=t){ pad_note_off(pend[i].src); pend[i]=pend[--npend]; } else ++i; }

        int n=(int)((total-frame)<BLOCK?(total-frame):BLOCK);
        memset(dryL,0,sizeof(float)*n); memset(dryR,0,sizeof(float)*n);
        pad_render_mix(dryL,dryR,zL,zR,n,0.0f);
        granular_block(dryL,dryR,gL,gR,n);
        diffuser_process(&df,gL,gR,n);
        reverb_render(gL,gR,wL,wR,n);

        for(int i=0;i<n;++i){
            float wet=wet_at((float)(frame+i)/SR);
            float dry=1.0f-0.55f*wet;
            float L=(dryL[i]*dry + wL[i]*0.85f*wet)*1.40f;
            float R=(dryR[i]*dry + wR[i]*0.85f*wet)*1.40f;
            int li=(int)(L*32767.0f), ri=(int)(R*32767.0f);
            if(li>32767)li=32767; if(li<-32768)li=-32768;
            if(ri>32767)ri=32767; if(ri<-32768)ri=-32768;
            out16[i*2]=(int16_t)li; out16[i*2+1]=(int16_t)ri;
            int a=li<0?-li:li; if(a>peak)peak=a; double s=li/32768.0; sumsq+=s*s; ++sumN;
        }
        fwrite(out16,sizeof(int16_t),(size_t)n*2,f);
        frame+=(uint32_t)n;
    }
    fclose(f);
    printf("performance : %s\n  peak %d (%.1f dBFS), RMS %.4f (%.1f dBFS)\n",
           out, peak, 20.0*log10((double)(peak?peak:1)/32767.0),
           sqrt(sumsq/(double)sumN), 20.0*log10(sqrt(sumsq/(double)sumN)+1e-12));
    return 0;
}
