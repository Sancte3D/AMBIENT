// Copyright 2014 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// C port for AMBIENT of two Mutable Instruments Rings effects
// (eurorack/rings/dsp/fx/reverb.h, ensemble.h on top of fx_engine.h).
// Algorithms, delay lengths, coefficients and 16-bit delay storage are those
// of the originals; the port runs one stereo sample per call, takes its sine
// from dsp_sin (the firmware's table) and keeps the LFO rates in Hz at
// 44.1 kHz. Caller-owned storage, no allocation, no libm, bounded work.

#ifndef FAM_MI_FX_H
#define FAM_MI_FX_H
#include <stdint.h>
#include <string.h>
#include "dsp.h"

/* One circular 16-bit buffer holds every delay line; a single write pointer
 * runs backwards, so the sample written at a line's base is found `offset`
 * samples later at base+offset (FxEngine). */
static inline float mi_fx_load(const uint16_t *buf,uint32_t mask,int32_t at) {
    return (float)(int16_t)buf[(uint32_t)at&mask]*(1.0f/32768.0f);
}
static inline void mi_fx_store(uint16_t *buf,uint32_t mask,int32_t at,float v) {
    int32_t q=(int32_t)(v*32768.0f);
    if(q>32767) q=32767; else if(q<-32768) q=-32768;
    buf[(uint32_t)at&mask]=(uint16_t)(int16_t)q;
}

/* ---------------------------------------------------------------- Reverb ---
 * Griesinger topology from the Dattorro paper: four input allpass diffusers,
 * then a figure-eight loop of two allpasses and a delay per side. The two
 * long delays are read with slow modulation (0.5 / 0.3 Hz), which smears the
 * tail into a gentle chorus instead of a metallic ring. 64 KB of storage. */
enum { MI_REVERB_SIZE=32768, MI_REVERB_LINES=10 };
static const uint16_t MI_REVERB_LEN[MI_REVERB_LINES]={150,214,319,527,2182,2690,4501,2525,2197,6312};
typedef struct {
    uint16_t *buf;
    int32_t write,base[MI_REVERB_LINES];
    float lp1,lp2,lfo_phase[2],lfo_value[2];
} mi_reverb_t;
static inline void mi_reverb_init(mi_reverb_t *r,uint16_t *storage) {
    memset(r,0,sizeof *r);
    r->buf=storage; memset(storage,0,MI_REVERB_SIZE*sizeof *storage);
    int32_t base=0;
    for(int k=0;k<MI_REVERB_LINES;++k) { r->base[k]=base; base+=MI_REVERB_LEN[k]+1; }
    r->lfo_value[0]=r->lfo_value[1]=.5f;
}
static inline float mi_reverb_tail(const mi_reverb_t *r,int k) {
    return mi_fx_load(r->buf,MI_REVERB_SIZE-1,r->write+r->base[k]+MI_REVERB_LEN[k]-1);
}
static inline void mi_reverb_put(mi_reverb_t *r,int k,float v) {
    mi_fx_store(r->buf,MI_REVERB_SIZE-1,r->write+r->base[k],v);
}
static inline float mi_reverb_read(const mi_reverb_t *r,int k,float offset) {
    int32_t i=(int32_t)offset; float f=offset-(float)i;
    float a=mi_fx_load(r->buf,MI_REVERB_SIZE-1,r->write+r->base[k]+i);
    float b=mi_fx_load(r->buf,MI_REVERB_SIZE-1,r->write+r->base[k]+i+1);
    return a+(b-a)*f;
}
/* in: the mono input (L+R times the input gain). diffusion (allpass gain),
 * lp (loop damping coefficient), time (loop gain). Writes the wet pair. */
static inline void mi_reverb_step(mi_reverb_t *r,float in,float diffusion,float lp,float time,float *wet_l,float *wet_r) {
    if(--r->write<0) r->write+=MI_REVERB_SIZE;
    if((r->write&31)==0) {
        /* The cosine LFOs advance every 32 samples, as in FxEngine. */
        static const float inc[2]={.5f*32.0f/DSP_SAMPLE_RATE_HZ,.3f*32.0f/DSP_SAMPLE_RATE_HZ};
        for(int k=0;k<2;++k) {
            r->lfo_phase[k]+=inc[k]; if(r->lfo_phase[k]>=1.0f) r->lfo_phase[k]-=1.0f;
            r->lfo_value[k]=.5f+.5f*dsp_sin(r->lfo_phase[k]+.25f);
        }
    }
    const float kap=diffusion;
    float acc=in,d;
    for(int k=0;k<4;++k) { d=mi_reverb_tail(r,k); acc+=kap*d; mi_reverb_put(r,k,acc); acc=acc*-kap+d; }
    const float apout=acc;
    /* Left half: the right half's long delay, modulated, into two allpasses. */
    acc=apout+time*mi_reverb_read(r,9,6261.0f+50.0f*r->lfo_value[1]);
    r->lp1+=lp*(acc-r->lp1); acc=r->lp1;
    d=mi_reverb_tail(r,4); acc+=-kap*d; mi_reverb_put(r,4,acc); acc=acc*kap+d;
    d=mi_reverb_tail(r,5); acc+=kap*d; mi_reverb_put(r,5,acc); acc=acc*-kap+d;
    mi_reverb_put(r,6,acc); *wet_l=2.0f*acc;
    /* Right half: the left half's long delay. */
    acc=apout+time*mi_reverb_read(r,6,4460.0f+40.0f*r->lfo_value[0]);
    r->lp2+=lp*(acc-r->lp2); acc=r->lp2;
    d=mi_reverb_tail(r,7); acc+=kap*d; mi_reverb_put(r,7,acc); acc=acc*-kap+d;
    d=mi_reverb_tail(r,8); acc+=-kap*d; mi_reverb_put(r,8,acc); acc=acc*kap+d;
    mi_reverb_put(r,9,acc); *wet_r=2.0f*acc;
    /* The 16-bit lines already flush to zero; the two damping states too. */
    if(r->lp1<1e-20f && r->lp1>-1e-20f) r->lp1=0;
    if(r->lp2<1e-20f && r->lp2>-1e-20f) r->lp2=0;
}

/* -------------------------------------------------------------- Ensemble ---
 * A string-machine ensemble: each side is read three times around 23 ms,
 * the taps moved by a slow (0.75 Hz) and a fast (6.6 Hz) LFO at 0/120/240
 * degrees, one tap borrowed from the other side. 8 KB of storage. */
enum { MI_ENSEMBLE_SIZE=4096 };
typedef struct {
    uint16_t buf[MI_ENSEMBLE_SIZE];
    int32_t write;
    float phase_1,phase_2;
} mi_ensemble_t;
static inline float mi_ensemble_read(const mi_ensemble_t *e,int32_t base,float offset) {
    int32_t i=(int32_t)offset; float f=offset-(float)i;
    float a=mi_fx_load(e->buf,MI_ENSEMBLE_SIZE-1,e->write+base+i);
    float b=mi_fx_load(e->buf,MI_ENSEMBLE_SIZE-1,e->write+base+i+1);
    return a+(b-a)*f;
}
/* amount: wet share (dry at 1-amount/2), depth 0..1 (128 samples slow sweep). */
static inline void mi_ensemble_step(mi_ensemble_t *e,float *left,float *right,float amount,float depth) {
    if(--e->write<0) e->write+=MI_ENSEMBLE_SIZE;
    e->phase_1+=.754f/DSP_SAMPLE_RATE_HZ; if(e->phase_1>=1.0f) e->phase_1-=1.0f;
    e->phase_2+=6.58f/DSP_SAMPLE_RATE_HZ; if(e->phase_2>=1.0f) e->phase_2-=1.0f;
    const float a=depth*128.0f,b=depth*12.8f;
    float m1=dsp_sin(e->phase_1)*a+dsp_sin(e->phase_2)*b;
    float m2=dsp_sin(e->phase_1+1.0f/3)*a+dsp_sin(e->phase_2+1.0f/3)*b;
    float m3=dsp_sin(e->phase_1+2.0f/3)*a+dsp_sin(e->phase_2+2.0f/3)*b;
    mi_fx_store(e->buf,MI_ENSEMBLE_SIZE-1,e->write,*left);
    mi_fx_store(e->buf,MI_ENSEMBLE_SIZE-1,e->write+2048,*right);
    float wl=.33f*(mi_ensemble_read(e,0,m1+1024)+mi_ensemble_read(e,0,m2+1024)+mi_ensemble_read(e,2048,m3+1024));
    float wr=.33f*(mi_ensemble_read(e,2048,m1+1024)+mi_ensemble_read(e,2048,m2+1024)+mi_ensemble_read(e,0,m3+1024));
    const float dry=1.0f-amount*.5f;
    *left=wl*amount+*left*dry;
    *right=wr*amount+*right*dry;
}
#endif
