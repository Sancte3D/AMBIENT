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
#include "texture.h"
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
/* C-dorian (key 60, mode 1). Long holds, simple progression — ambient beauty
 * comes from VERWEILEN, not abwechslung. 5 chord changes in 60 s, not 9. */
typedef struct { float t; int degree, vibe; float vel, hold; } chord_t;
static const chord_t CHORDS[] = {
    {  1.0f, 1, 0, 0.12f, 15.0f },   /* Cm  add9   (intro, long hold) */
    { 14.0f, 6, 0, 0.11f, 14.0f },   /* Am  add9   (vi — relative minor open) */
    { 26.0f, 3, 1, 0.10f, 12.0f },   /* Eb  maj7   (gentle major — colour shift) */
    { 36.0f, 4, 3, 0.11f, 12.0f },   /* F   sus2   (suspension) */
    { 46.0f, 1, 0, 0.10f, 14.0f },   /* Cm  add9   (return home) */
};
#define N_CH (sizeof CHORDS / sizeof CHORDS[0])

/* sparse melody: 4 notes in 60 s — leerstellen are part of the music */
typedef struct { float t; int midi; float vel, dur; } mel_t;
static const mel_t MELODY[] = {
    { 18.0f, 79, 0.11f, 4.0f },   /* G5  — over Am */
    { 28.0f, 75, 0.10f, 5.0f },   /* Eb5 — on the Eb change */
    { 38.0f, 77, 0.10f, 5.0f },   /* F5  — on the F change */
    { 50.0f, 72, 0.09f, 8.0f },   /* C5  — resolution, long tail */
};
#define N_MEL (sizeof MELODY / sizeof MELODY[0])

/* wet automation — pro-mix rule: 15–25 % is "tasteful hall", over 30 % is
 * "I love my reverb". For wind/wald/nebel territory we live at ~20 % with a
 * slow open to 28 %, never higher. Dry stays >= 90 %. */
static float wet_at(float t){
    static const float bt[] = {  0.0f, 16.0f, 38.0f, 60.0f };
    static const float bv[] = { 0.10f, 0.20f, 0.28f, 0.22f };
    int n = 4;
    if (t <= bt[0]) return bv[0];
    for (int i=1;i<n;i++) if (t<=bt[i]){ float f=(t-bt[i-1])/(bt[i]-bt[i-1]); return bv[i-1]+(bv[i]-bv[i-1])*f; }
    return bv[n-1];
}

/* soft-clip saturation — "Orgel → Material". Light; only colours the peaks. */
static inline float sat(float x){ return tanhf(x * 1.25f) * 0.85f; }

typedef struct { float t; uint8_t src; } pend_t;
static pend_t pend[PAD_MAX*2]; static int npend = 0;
static void sched_off(float t, uint8_t s){ if(npend<(int)(sizeof pend/sizeof pend[0])){pend[npend].t=t;pend[npend].src=s;npend++;} }

/* ----------------------------- granular -------------------------------- */
/* The cloud is ATMOSPHERIC SPICE, not the main sound. Tuned for ambient
 * beauty (Eno / Stars-of-the-Lid territory) rather than FM-noise density:
 *   - dense cloud was 49 grains/s → now ~11/s (audible as a wash, not a brei)
 *   - grains were 34..113 ms → now 200..500 ms (proper clouds, not knister)
 *   - pitch scatter was unison/+oct/-oct/+5th → now unison/+oct/-oct ONLY.
 *     +5th transposes "ghost" reads of the OLD chord into the new key during
 *     transitions — that was the main "falsche Töne im Hintergrund" source.
 *   - random pan was 0..1 per grain → now stereo-pair pattern (left grain
 *     pans left, right grain pans right) so the bus stays phase-stable. */
#define GBUF_LEN  (1u<<16)
#define GBUF_MASK (GBUF_LEN-1u)
#define MAXG      24
#define GR_SPACING   6000    /* ~7 grains/s — wind-sparse, lets dry chord breathe */
#define WAVE_SPACING 200     /* slow forward scan — grains sample neighbours */

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
static int grain_pair = 0;     /* alternates L/R for stereo-pair panning */
static void grain_spawn(void){
    for(int g=0; g<MAXG; ++g){
        if(grains[g].active) continue;
        grain_t *gr=&grains[g];
        gr->active=1; gr->age=0;
        gr->len   =13000+(int)(frand()*18000.0f);   /* 300..700 ms — longer wash */
        /* read into recent history (NEVER past gwrite); slow forward scan */
        uint32_t jit=(uint32_t)(frand()*8000.0f);   /* ±180 ms time-jitter */
        gr->pos=(float)((gwrite-(GBUF_LEN-16000)+scan+jit)&GBUF_MASK);
        scan+=WAVE_SPACING; if(scan>(GBUF_LEN-20000)) scan=0;
        /* pitch: unison dominant + occasional -oct for body. NO +oct — that
         * is functionally a "shimmer" layer and the #1 trigger for the
         * Kirchen-/Klangschalen-Charakter we are trying to escape. */
        gr->inc = (frand() < 0.82f) ? 1.0f : 0.5f;
        /* stereo-pair pan: alternating L/R, narrow spread → phase-stable bus */
        gr->pan=(grain_pair^=1) ? (0.22f+frand()*0.18f)   /* 0.22..0.40 L */
                                : (0.60f+frand()*0.18f);  /* 0.60..0.78 R */
        gr->amp=0.55f; return;
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
    /* Producer-rule: when reverb sounds "geil" it's already too much. Wind/
     * Wald/Nebel lives on a DRY foreground with a hint of room — small hall
     * (size 0.50, decay ~3 s), well-damped highs, soft drive. Diffuser only
     * smooths the grain edges; it's not a second wash. */
    reverb_init(); reverb_set(0.50f,0.62f); reverb_set_drive(0.05f);
    diffuser_t df; diffuser_init(&df); diffuser_set_amount(&df,0.30f);
    /* Brown-noise wind/body texture — the "70% atmosphere" the take was
     * missing. texture.c renders Air + Body bands stereo-decorrelated. */
    texture_init(); texture_set_amount(0.35f);
    memset(gbufL,0,sizeof gbufL); memset(gbufR,0,sizeof gbufR); memset(grains,0,sizeof grains);

    /* All voices on Bed (the darkest profile, cutoff 380 Hz). Same character
     * across drone + chords = one cohesive "wood/wind body" instead of three
     * different timbres competing. */
    pad_set_profile(0);
    /* Drone bed, held nearly the whole take */
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
        /* chord presses → up to 3 voiced notes (capped), Bed profile, voicings
         * transposed -7 (perfect 4th down) so the chord sits in the warm mid-
         * low band instead of the Klangschalen register. Melody removed —
         * 70 % atmosphere / 20 % harmony / 10 % melody → for the demo, 0 %. */
        while(ci<N_CH && CHORDS[ci].t<=t){
            const chord_t *c=&CHORDS[ci]; int bank=(kchord%2)*4;
            brain_set_vibe(c->vibe);
            int notes[BRAIN_MAX_CHORD], nn=brain_chord(c->degree,notes,BRAIN_MAX_CHORD);
            if(nn>3) nn=3;                                  /* 3 voices keeps it open */
            float per=c->vel/sqrtf((float)(nn>0?nn:1));
            for(int j=0;j<nn;++j){ uint8_t s=(uint8_t)(bank+j); pad_note_on(s,dsp_midi_to_hz((float)(notes[j]-7)),per); sched_off(t+c->hold,s); }
            ++kchord; ++ci;
        }
        (void)mi;   /* melody intentionally disabled in this revision */
        for(int i=0;i<npend;){ if(pend[i].t<=t){ pad_note_off(pend[i].src); pend[i]=pend[--npend]; } else ++i; }

        int n=(int)((total-frame)<BLOCK?(total-frame):BLOCK);
        memset(dryL,0,sizeof(float)*n); memset(dryR,0,sizeof(float)*n);
        memset(zL,0,sizeof(float)*n);   memset(zR,0,sizeof(float)*n);
        pad_render_mix(dryL,dryR,zL,zR,n,0.0f);
        texture_render_mix(dryL,dryR,zL,zR,n,0.0f);     /* wind/body bed INTO dry */
        granular_block(dryL,dryR,gL,gR,n);              /* grains read chord+texture */
        diffuser_process(&df,gL,gR,n);
        reverb_render(gL,gR,wL,wR,n);

        for(int i=0;i<n;++i){
            float wet=wet_at((float)(frame+i)/SR);
            /* dry-floor: never drop dry below 92 % — wet hangs off the chord
             * as a hint of room, never replaces it. */
            float dry=1.0f-0.08f*wet;
            float L = sat( (dryL[i]*dry + wL[i]*0.40f*wet) * 1.20f );
            float R = sat( (dryR[i]*dry + wR[i]*0.40f*wet) * 1.20f );
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
