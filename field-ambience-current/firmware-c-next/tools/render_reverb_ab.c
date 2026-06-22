/*
 * render_reverb_ab.c — 3-way reverb audition on the accepted dreamy/warm-pop
 * phrase (same pad/texture/hiss/saturation, only the reverb differs):
 *
 *   0–20 s : FREEVERB        — the current src/reverb.c (static combs)
 *   20–40 s: LUSH            — 8 MODULATED combs (slow per-line LFO, frac read)
 *                              + pre-delay. Tail detunes → 3D, not metallic.
 *   40–60 s: LIQUID          — LUSH + Greyhole-style MODULATED ALLPASS
 *                              DIFFUSION (input diffusers + modulated output
 *                              allpasses). The smeared, dense, "liquid"
 *                              ambient tail.
 *
 * Clean-room C. Three libraries were checked for borrowable reverb code:
 *   - DaisySP (MIT): no reverb module at all → nothing to take.
 *   - Surge sst-effects (GPL3): reverb exists but GPL3 + desktop SSE2 → can't
 *     link into product firmware; reference only.
 *   - SuperCollider / sc3-plugins (GPL3): GVerb/JPverb/Greyhole — same GPL3
 *     story. Greyhole's IDEA (modulated allpass diffusion network) is what
 *     LIQUID reimplements here in clean C. Algorithms aren't copyrightable;
 *     the code is — so this is written from the technique, not copied.
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_reverb_ab.c \
 *      src/dsp.c src/pad.c src/texture.c src/reverb.c -lm \
 *      -o /tmp/render_reverb_ab
 *   /tmp/render_reverb_ab /tmp/reverb_ab.wav
 */

#include "pad.h"
#include "texture.h"
#include "reverb.h"
#include "dsp.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SR        44100
#define BLOCK     256
#define SECT_SECS 20

/* ---- tape hiss + warm saturation ---- */
static uint32_t hr = 0xC0FFEE11u;
static inline float hn(void){ hr = hr*1664525u + 1013904223u; return ((int32_t)hr)*(1.0f/2147483648.0f); }
static inline void hiss_block(float *L, float *R, int n, float amp){
    for (int i=0;i<n;++i){ L[i]+=hn()*amp; R[i]+=hn()*amp; }
}

/* =========================================================================
 * LUSH / LIQUID reverb. 8 modulated combs + (LIQUID) a modulated allpass
 * diffusion network, per channel, behind a pre-delay.
 * ========================================================================= */
#define RV_SPREAD   23
#define RV_FIXGAIN  0.015f
#define RV_MAXCOMB  1800
#define RV_MAXAP    1024
#define RV_PREDELAY 1100          /* ~25 ms */

static const int RVC[8] = { 1116,1188,1277,1356,1422,1491,1557,1617 };
static const int RVA[4] = {  556, 441, 341, 225 };   /* output allpasses */
static const int RVD[2] = {  142,  379 };            /* input diffusers (LIQUID) */
static const float RV_LFO_HZ[8] = { 0.50f,0.63f,0.78f,0.92f,1.07f,1.18f,1.31f,1.44f };

typedef struct { float buf[RV_MAXCOMB]; int size,pos; float store; float lfo,inc,depth; } mcomb_t;
/* modulated allpass: fractional read at (pos - lfo*depth), Schroeder structure */
typedef struct { float buf[RV_MAXAP]; int size,pos; float lfo,inc,depth; float fb; } mallp_t;

typedef struct {
    mcomb_t comb[8];
    mallp_t ap[4];        /* output allpasses (modulated in LIQUID) */
    mallp_t diff[2];      /* input diffusers (LIQUID only) */
    float pre[RV_PREDELAY]; int pre_pos;
} mchan_t;

static mchan_t LCH, RCH;
static float rv_fb, rv_damp;
static int   rv_liquid;        /* 0 = LUSH, 1 = LIQUID (diffusion on) */

static void mallp_init(mallp_t *a, int size, float hz, float depth, float fb){
    memset(a->buf,0,sizeof a->buf);
    a->size=size; a->pos=0; a->lfo=0.0f; a->inc=hz/(float)SR; a->depth=depth; a->fb=fb;
}
static void mchan_init(mchan_t *c, int spread){
    for (int i=0;i<8;++i){
        memset(c->comb[i].buf,0,sizeof c->comb[i].buf);
        c->comb[i].size=RVC[i]+spread; c->comb[i].pos=0; c->comb[i].store=0.0f;
        c->comb[i].lfo=(float)i*0.13f; c->comb[i].inc=RV_LFO_HZ[i]/(float)SR;
        c->comb[i].depth=6.0f+(float)i*0.6f;
    }
    for (int i=0;i<4;++i) mallp_init(&c->ap[i], RVA[i]+spread, 0.30f+0.07f*i, rv_liquid?3.5f:0.0f, 0.5f);
    for (int i=0;i<2;++i) mallp_init(&c->diff[i], RVD[i]+spread, 0.20f+0.05f*i, rv_liquid?2.5f:0.0f, 0.62f);
    memset(c->pre,0,sizeof c->pre); c->pre_pos=0;
}
static void lush_init(float size_0_1, float damp_0_1, int liquid){
    rv_liquid = liquid;
    rv_fb   = 0.70f + 0.28f * size_0_1;
    rv_damp = 0.40f * damp_0_1;
    mchan_init(&LCH, 0);
    mchan_init(&RCH, RV_SPREAD);
}

static inline float mcomb_process(mcomb_t *c, float in, float fb, float damp){
    c->lfo += c->inc; if (c->lfo>=1.0f) c->lfo-=1.0f;
    float mod = (0.5f+0.5f*sinf(c->lfo*6.2831853f))*c->depth;
    float rp = (float)c->pos - mod; while (rp<0.0f) rp+=(float)c->size;
    int i0=(int)rp, i1=(i0+1)%c->size; float fr=rp-(float)i0;
    float y = c->buf[i0]*(1.0f-fr)+c->buf[i1]*fr;
    c->store = y*(1.0f-damp)+c->store*damp;
    c->buf[c->pos]=in+c->store*fb;
    if (++c->pos>=c->size) c->pos=0;
    return y;
}
static inline float mallp_process(mallp_t *a, float in){
    float rp;
    if (a->depth > 0.0f){
        a->lfo += a->inc; if (a->lfo>=1.0f) a->lfo-=1.0f;
        float mod=(0.5f+0.5f*sinf(a->lfo*6.2831853f))*a->depth;
        rp=(float)a->pos-mod; while (rp<0.0f) rp+=(float)a->size;
    } else {
        rp=(float)a->pos;
    }
    int i0=(int)rp, i1=(i0+1)%a->size; float fr=rp-(float)i0;
    float y = a->buf[i0]*(1.0f-fr)+a->buf[i1]*fr;
    float v = in + y*a->fb;
    a->buf[a->pos]=v;
    if (++a->pos>=a->size) a->pos=0;
    return y - in*a->fb;
}
static inline float chan_process(mchan_t *c, float in){
    float pd=c->pre[c->pre_pos]; c->pre[c->pre_pos]=in;
    if (++c->pre_pos>=RV_PREDELAY) c->pre_pos=0;
    float x = pd * RV_FIXGAIN;
    if (rv_liquid){ x = mallp_process(&c->diff[0], x); x = mallp_process(&c->diff[1], x); }
    float y=0.0f;
    for (int i=0;i<8;++i) y += mcomb_process(&c->comb[i], x, rv_fb, rv_damp);
    for (int i=0;i<4;++i) y = mallp_process(&c->ap[i], y);
    return y;
}
static void lush_render(const float *inL, const float *inR, float *outL, float *outR, int n){
    for (int i=0;i<n;++i){ outL[i]=chan_process(&LCH,inL[i]); outR[i]=chan_process(&RCH,inR[i]); }
}

/* ---- WAV ---- */
static void wav_header(FILE *f, uint32_t nf){
    uint32_t data=nf*4u, v; uint16_t h;
    fwrite("RIFF",1,4,f); v=36+data;fwrite(&v,4,1,f); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); v=16;fwrite(&v,4,1,f); h=1;fwrite(&h,2,1,f); h=2;fwrite(&h,2,1,f);
    v=SR;fwrite(&v,4,1,f); v=SR*4u;fwrite(&v,4,1,f); h=4;fwrite(&h,2,1,f); h=16;fwrite(&h,2,1,f);
    fwrite("data",1,4,f); fwrite(&data,4,1,f);
}

typedef struct { float t; int midi; float vel; } evt_t;
static const evt_t EV[] = {
    {  1.0f, 69, 0.20f }, {  5.0f, 73, 0.18f }, {  9.0f, 76, 0.17f },
    { 13.0f, 71, 0.18f }, { 16.0f, 78, 0.16f },
};
#define N_EV (sizeof EV/sizeof EV[0])

typedef struct { float t; uint8_t src; } pend_t;
static pend_t pend[32]; static int npend=0;
static void sched_off(float t, uint8_t s){ if(npend<32){pend[npend].t=t;pend[npend].src=s;npend++;} }

/* mode: 0=freeverb, 1=lush, 2=liquid */
static void render_section(int mode, FILE *f, int *peak, double *sumsq, long *sumN){
    pad_init();
    texture_init(); texture_set_amount(0.12f);
    if (mode==0){ reverb_init(); reverb_set(0.60f,0.40f); reverb_set_drive(0.08f); }
    else        { lush_init(0.60f, 0.40f, mode==2 ? 1 : 0); }
    npend=0;
    pad_note_on(10, dsp_midi_to_hz(33.0f), 0.13f);
    pad_note_on(11, dsp_midi_to_hz(40.0f), 0.10f);
    sched_off((float)SECT_SECS-2.0f, 10); sched_off((float)SECT_SECS-2.0f, 11);

    float dryL[BLOCK],dryR[BLOCK],sndL[BLOCK],sndR[BLOCK],wL[BLOCK],wR[BLOCK];
    int16_t out16[BLOCK*2];
    uint32_t total=(uint32_t)SECT_SECS*SR, frame=0; size_t ei=0;
    const float WET=0.42f, HISS=0.005f, SAT=1.10f;

    while (frame<total){
        float t=(float)frame/SR;
        while (ei<N_EV && EV[ei].t<=t){ uint8_t s=(uint8_t)(ei%6);
            pad_note_on(s, dsp_midi_to_hz((float)EV[ei].midi), EV[ei].vel); sched_off(t+5.5f,s); ++ei; }
        for (int i=0;i<npend;){ if(pend[i].t<=t){pad_note_off(pend[i].src);pend[i]=pend[--npend];} else ++i; }

        int n=(int)((total-frame)<BLOCK?(total-frame):BLOCK);
        memset(dryL,0,sizeof(float)*n); memset(dryR,0,sizeof(float)*n);
        memset(sndL,0,sizeof(float)*n); memset(sndR,0,sizeof(float)*n);
        pad_render_mix(dryL,dryR,sndL,sndR,n,0.75f);
        texture_render_mix(dryL,dryR,sndL,sndR,n,0.55f);
        hiss_block(dryL,dryR,n,HISS);

        if (mode==0) reverb_render(sndL,sndR,wL,wR,n);
        else         lush_render (sndL,sndR,wL,wR,n);

        for (int i=0;i<n;++i){
            float L=(dryL[i]+wL[i]*WET)*0.95f, Rr=(dryR[i]+wR[i]*WET)*0.95f;
            L=tanhf(L*SAT)*0.78f; Rr=tanhf(Rr*SAT)*0.78f;
            int li=(int)(L*32767.0f), ri=(int)(Rr*32767.0f);
            if(li>32767)li=32767; if(li<-32768)li=-32768;
            if(ri>32767)ri=32767; if(ri<-32768)ri=-32768;
            out16[i*2]=(int16_t)li; out16[i*2+1]=(int16_t)ri;
            int a=li<0?-li:li; if(a>*peak)*peak=a; double s=li/32768.0; *sumsq+=s*s; ++*sumN;
        }
        fwrite(out16,sizeof(int16_t),(size_t)n*2,f);
        frame+=(uint32_t)n;
    }
    pad_all_off();
}

int main(int argc, char **argv){
    const char *out=(argc>1)?argv[1]:"/tmp/reverb_ab.wav";
    dsp_init();
    FILE *f=fopen(out,"wb"); if(!f){fprintf(stderr,"open %s\n",out);return 1;}
    wav_header(f,(uint32_t)SECT_SECS*SR*3);
    int peak=0; double sumsq=0; long sumN=0;
    render_section(0, f, &peak, &sumsq, &sumN);   /* FREEVERB */
    render_section(1, f, &peak, &sumsq, &sumN);   /* LUSH     */
    render_section(2, f, &peak, &sumsq, &sumN);   /* LIQUID   */
    fclose(f);
    printf("reverb 3-way : %s\n  peak %d (%.1f dBFS), RMS %.4f (%.1f dBFS)\n",
           out, peak, 20.0*log10((double)(peak?peak:1)/32767.0),
           sqrt(sumsq/(double)sumN), 20.0*log10(sqrt(sumsq/(double)sumN)+1e-12));
    return 0;
}
