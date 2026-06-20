/*
 * render_cells_granular.c — audition the full ambient chain the user sketched
 * in FL-Studio terms:
 *
 *   chord per cell  →  GRANULIZER (randomizer 100%, GR-spacing, wave-spacing)
 *                   →  BLUR  (Fruity-Convolver "blur white" ≈ allpass diffuser)
 *                   →  REVERB
 *
 * Source : brain_chord() voiced diatonic chords (the HiChord cell model).
 * Grain  : a real granular PROCESSOR of the chord bus — not synthesis. Reads
 *          windowed grains from a ~1.5 s circular buffer of the live chord,
 *          scattered in time / position / pan / octave (randomizer = 100%).
 * Blur   : src/v2/diffuser.c — turns hard grain edges into a cloud, real-time,
 *          the cheap embedded substitute for white-IR convolution.
 * Reverb : src/reverb.c.
 *
 * Layout of the render (one file, 30 s):
 *   0–11 s   DRY chord-per-cell (reference — what the cells sound like clean)
 *   11–12 s  crossfade
 *   12–30 s  the same chords, now granulated → blurred → reverberated (cloud)
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_cells_granular.c \
 *      src/dsp.c src/pad.c src/brain.c src/reverb.c src/v2/diffuser.c \
 *      -lm -o /tmp/render_cells_granular
 *   /tmp/render_cells_granular /tmp/cells_grain.wav
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

#define SR          44100
#define BLOCK       256
#define PART_SECS   30
#define HOLD_S      6.0f

/* ---- the chord phrase (same as the A/B tool, degrees within 5 cells) ---- */
typedef struct { float t; int degree; int vibe; } press_t;
static const press_t SEQ[] = {
    {  0.5f, 1, 0 }, {  4.5f, 5, 0 }, {  8.5f, 4, 1 },
    { 12.5f, 3, 1 }, { 16.5f, 2, 2 }, { 20.5f, 1, 3 }, { 24.5f, 4, 1 },
};
#define N_SEQ (sizeof SEQ / sizeof SEQ[0])

typedef struct { float t; uint8_t src; } pend_t;
static pend_t pend[PAD_MAX*2]; static int npend = 0;
static void sched_off(float t, uint8_t s){ if(npend<(int)(sizeof pend/sizeof pend[0])){pend[npend].t=t;pend[npend].src=s;npend++;} }

/* ----------------------------- granular -------------------------------- */
#define GBUF_LEN  (1u<<16)          /* 65536 ≈ 1.49 s */
#define GBUF_MASK (GBUF_LEN-1u)
#define MAXG      40

/* "GR spacing" — samples between grain onsets (smaller = denser cloud).      */
#define GR_SPACING   1100
/* "wave spacing" — how far the grain read-head steps through the material per
 * grain (gives the cloud forward motion instead of freezing on one moment).  */
#define WAVE_SPACING 900

typedef struct {
    int   active;
    float pos;      /* read position (fractional) into gbuf */
    float inc;      /* pitch ratio */
    int   age, len;
    float pan;      /* 0..1 */
    float amp;
} grain_t;

static float gbufL[GBUF_LEN], gbufR[GBUF_LEN];
static uint32_t gwrite = 0;
static grain_t grains[MAXG];
static int spawn_ctr = 0;
static uint32_t scan = 0;             /* forward read-scan ("wave spacing") */

static uint32_t rng = 0x1234567u;
static inline uint32_t xr(void){ rng ^= rng<<13; rng ^= rng>>17; rng ^= rng<<5; return rng; }
static inline float frand(void){ return (xr() >> 8) * (1.0f/16777216.0f); }   /* 0..1 */

static inline float gread(const float *b, float pos){
    uint32_t i0 = (uint32_t)pos & GBUF_MASK;
    uint32_t i1 = (i0+1) & GBUF_MASK;
    float fr = pos - floorf(pos);
    return b[i0]*(1.0f-fr) + b[i1]*fr;
}

static void grain_spawn(void){
    for (int g=0; g<MAXG; ++g){
        if (grains[g].active) continue;
        grain_t *gr = &grains[g];
        gr->active = 1;
        gr->age    = 0;
        gr->len    = 1500 + (int)(frand()*3500.0f);         /* 34..113 ms */
        /* read start: scan forward through the last ~1.3 s of chord history
         * (wave-spacing) + random jitter (randomizer 100%), always behind the
         * write head so a grain never reads unwritten material. */
        uint32_t jit   = (uint32_t)(frand()*4000.0f);
        uint32_t start = (gwrite - (GBUF_LEN-6000) + scan + jit) & GBUF_MASK;
        gr->pos        = (float)start;
        scan += WAVE_SPACING;
        if (scan > (GBUF_LEN-12000)) scan = 0;
        /* randomizer 100%: octave scatter for shimmer, mostly unison */
        float r = frand();
        gr->inc    = (r<0.62f) ? 1.0f : (r<0.82f) ? 2.0f : (r<0.92f) ? 0.5f : 1.5f;
        gr->pan    = frand();
        gr->amp    = 0.55f;
        return;
    }
}

static void granular_block(const float *inL, const float *inR,
                           float *outL, float *outR, int n){
    for (int i=0; i<n; ++i){
        gbufL[gwrite & GBUF_MASK] = inL[i];
        gbufR[gwrite & GBUF_MASK] = inR[i];
        gwrite++;

        if (--spawn_ctr <= 0){
            grain_spawn();
            /* GR spacing with randomizer-100% jitter (±50%) */
            spawn_ctr = (int)(GR_SPACING * (0.5f + frand()));
        }

        float l=0.0f, r=0.0f;
        for (int g=0; g<MAXG; ++g){
            grain_t *gr = &grains[g];
            if (!gr->active) continue;
            float w = 0.5f - 0.5f*cosf(6.2831853f * (float)gr->age / (float)gr->len); /* Hann */
            float s = gread(gbufL, gr->pos) * 0.5f + gread(gbufR, gr->pos) * 0.5f;
            float v = s * w * gr->amp;
            l += v * (1.0f - gr->pan);
            r += v * gr->pan;
            gr->pos += gr->inc;
            if (++gr->age >= gr->len) gr->active = 0;
        }
        outL[i] = l;
        outR[i] = r;
    }
}

int main(int argc, char **argv){
    const char *out = (argc>1) ? argv[1] : "/tmp/cells_grain.wav";
    dsp_init();
    brain_init(); brain_set_key(60); brain_set_mode(1);   /* C dorian */
    pad_init();   pad_set_profile(1);                     /* Felt */
    reverb_init(); reverb_set(0.88f, 0.42f); reverb_set_drive(0.2f);
    diffuser_t df; diffuser_init(&df); diffuser_set_amount(&df, 0.80f);
    memset(gbufL,0,sizeof gbufL); memset(gbufR,0,sizeof gbufR);
    memset(grains,0,sizeof grains);
    spawn_ctr = 1;

    FILE *f = fopen(out,"wb"); if(!f){ fprintf(stderr,"open %s\n",out); return 1; }
    /* wav header */
    uint32_t nf = (uint32_t)PART_SECS*SR, data = nf*4u;
    fwrite("RIFF",1,4,f); uint32_t v;
    v=36+data; fwrite(&v,4,1,f); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); v=16; fwrite(&v,4,1,f);
    uint16_t h; h=1;fwrite(&h,2,1,f); h=2;fwrite(&h,2,1,f);
    v=SR;fwrite(&v,4,1,f); v=SR*4u;fwrite(&v,4,1,f); h=4;fwrite(&h,2,1,f); h=16;fwrite(&h,2,1,f);
    fwrite("data",1,4,f); fwrite(&data,4,1,f);

    float dryL[BLOCK],dryR[BLOCK],zL[BLOCK],zR[BLOCK];
    float gL[BLOCK],gR[BLOCK],wL[BLOCK],wR[BLOCK];
    int16_t out16[BLOCK*2];

    uint32_t total=(uint32_t)PART_SECS*SR, frame=0; size_t si=0;
    int peak=0; double sumsq=0; long sumN=0;
    const float T_DRY=11.0f, T_XF=1.0f;     /* dry until 11 s, 1 s crossfade */

    while (frame<total){
        float t=(float)frame/SR;
        while (si<N_SEQ && SEQ[si].t<=t){
            const press_t *p=&SEQ[si]; int bank=(int)(si%2)*6;
            brain_set_vibe(p->vibe);
            int notes[BRAIN_MAX_CHORD], nn=brain_chord(p->degree,notes,BRAIN_MAX_CHORD);
            float per=0.13f/sqrtf((float)(nn>0?nn:1));
            for(int j=0;j<nn;++j){ uint8_t s=(uint8_t)(bank+j); pad_note_on(s,dsp_midi_to_hz((float)notes[j]),per); sched_off(t+HOLD_S,s); }
            ++si;
        }
        for(int i=0;i<npend;){ if(pend[i].t<=t){ pad_note_off(pend[i].src); pend[i]=pend[--npend]; } else ++i; }

        int n=(int)((total-frame)<BLOCK?(total-frame):BLOCK);
        memset(dryL,0,sizeof(float)*n); memset(dryR,0,sizeof(float)*n);
        pad_render_mix(dryL,dryR,zL,zR,n,0.0f);          /* chord bus (float) */

        granular_block(dryL,dryR,gL,gR,n);               /* GRANULIZER */
        diffuser_process(&df,gL,gR,n);                   /* BLUR white */
        reverb_render(gL,gR,wL,wR,n);                    /* REVERB (wet) */

        for(int i=0;i<n;++i){
            float tt=(float)(frame+i)/SR;
            float wet = (tt<T_DRY)?0.0f : (tt<T_DRY+T_XF)?(tt-T_DRY)/T_XF : 1.0f;
            float dry = 1.0f - 0.75f*wet;                /* keep some dry chord under cloud */
            float L = dryL[i]*dry + wL[i]*(0.9f*wet);
            float R = dryR[i]*dry + wR[i]*(0.9f*wet);
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
    printf("chord→granular→blur→reverb : %s\n  peak %d (%.1f dBFS), RMS %.4f (%.1f dBFS)\n",
           out, peak, 20.0*log10((double)(peak?peak:1)/32767.0),
           sqrt(sumsq/(double)sumN), 20.0*log10(sqrt(sumsq/(double)sumN)+1e-12));
    return 0;
}
