#ifndef FAM_AMBIENT_CHOIR_H
#define FAM_AMBIENT_CHOIR_H
/* Ambient voicing: a soft choir. Three singers per note, each a band-limited
 * saw (a full harmonic voice source) detuned -7 / 0 / +6 cents, with its own
 * natural vibrato (4.6-5.6 Hz, about +-7 cents) that fades in after 0.4 s.
 * A cascade of four resonant low-passes at the vowel's formants is the vocal
 * tract (Klatt's cascade form): unity gain below the first formant keeps the
 * fundamental, so the voice is full rather than thin. A little breath rides
 * through the same tract. vowel 0 is a warm "aah", 1 a round "ooh".
 * Vibrato at control rate (every 32 samples); per sample 3 saws, 5 filters.
 * No per-sample transcendental (dsp_sin is a LUT). */
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "dsp.h"

#define AMBIENT_CHOIR_SINGERS 3
#define AMBIENT_CHOIR_REF_POWER 11.682f /* tract response power of a D4 "aah" */
typedef struct {
    float ph[AMBIENT_CHOIR_SINGERS],inc[AMBIENT_CHOIR_SINGERS],base[AMBIENT_CHOIR_SINGERS];
    float vib_ph[AMBIENT_CHOIR_SINGERS],vib_inc[AMBIENT_CHOIR_SINGERS];
    float vib,vib_target,norm,det[AMBIENT_CHOIR_SINGERS];
    int vib_delay,ctl;
    dsp_svf_t f1,f2,f3,f4,side;
    uint32_t rng;
} ambient_choir_t;

static inline float ambient_choir_unit(uint32_t *h) {
    *h=*h*1664525u+1013904223u; return (float)(*h>>8)*(1.0f/16777216.0f);
}
static inline void ambient_choir_prepare(ambient_choir_t *c,float hz,uint32_t seed,float vowel) {
    memset(c,0,sizeof *c);
    if(!(hz>20) || hz>4000) return;
    vowel=dsp_clampf(vowel,0,1);
    static const float detune[AMBIENT_CHOIR_SINGERS]={-.0040f,0,.0035f};
    uint32_t h=seed|1u;
    for(int s=0;s<AMBIENT_CHOIR_SINGERS;++s) {
        c->det[s]=detune[s];
        c->base[s]=c->inc[s]=hz*(1.0f+detune[s])/(float)DSP_SAMPLE_RATE_HZ;
        c->ph[s]=ambient_choir_unit(&h);
        c->vib_ph[s]=ambient_choir_unit(&h);
        c->vib_inc[s]=(4.6f+1.0f*ambient_choir_unit(&h))*32.0f/(float)DSP_SAMPLE_RATE_HZ;
    }
    c->vib_target=.0040f; c->vib_delay=(int)(.40f*DSP_SAMPLE_RATE_HZ/32.0f);
    /* Formants: female "aah" 800/1150/2900/3900 Hz, "ooh" 400/800/2800/3700 Hz,
     * soft bandwidths (a choir pad rather than speech). */
    float F1=800.0f-400.0f*vowel,F2=1150.0f-350.0f*vowel,F3=2900.0f-100.0f*vowel,F4=3900.0f-200.0f*vowel;
    dsp_svf_set(&c->f1,F1,4.0f); dsp_svf_set(&c->f2,F2,5.0f);
    dsp_svf_set(&c->f3,F3,6.0f); dsp_svf_set(&c->f4,F4,6.0f);
    dsp_svf_set(&c->side,F1,2.0f);
    /* Equal loudness at every pitch: the tract boosts the harmonics that meet
     * its formants, so a low note sounds far louder than a high one. Sum the
     * saw's harmonics (1/k) through the analog tract response once, here in
     * Main, and normalise to the response of a D4 "aah" (measured .16). */
    const float F[4]={F1,F2,F3,F4},Q[4]={4.0f,5.0f,6.0f,6.0f};
    float power=0;
    for(int k=1;k<=10;++k) {
        float g=1.0f/(float)(k*k);
        for(int j=0;j<4;++j) {
            float r=hz*(float)k/F[j],d=1.0f-r*r,q=r/Q[j];
            g/=d*d+q*q;
        }
        power+=g;
    }
    c->norm=.16f*sqrtf(AMBIENT_CHOIR_REF_POWER/(power+1e-12f));
    if(c->norm>.6f) c->norm=.6f;
    c->rng=h;
}
/* Follow a gliding pitch (block rate); vibrato applies at the next tick. */
static inline void ambient_choir_retune(ambient_choir_t *c,float hz) {
    for(int s=0;s<AMBIENT_CHOIR_SINGERS;++s) c->base[s]=hz*(1.0f+c->det[s])/(float)DSP_SAMPLE_RATE_HZ;
}
/* One sample of the note; *side is a decorrelated width signal (the outer
 * singers' difference through the first formant) for the opposite channel. */
static inline float ambient_choir_sample(ambient_choir_t *c,float shade,float *side) {
    if(--c->ctl<0) {
        c->ctl=31;
        if(c->vib_delay>0) --c->vib_delay; else c->vib+=(c->vib_target-c->vib)*.02f;
        for(int s=0;s<AMBIENT_CHOIR_SINGERS;++s) {
            c->vib_ph[s]+=c->vib_inc[s]; if(c->vib_ph[s]>=1.0f) c->vib_ph[s]-=1.0f;
            c->inc[s]=c->base[s]*(1.0f+c->vib*dsp_sin(c->vib_ph[s]));
        }
    }
    float s0=dsp_poly_saw(c->ph[0],c->inc[0]),s1=dsp_poly_saw(c->ph[1],c->inc[1]),s2=dsp_poly_saw(c->ph[2],c->inc[2]);
    for(int s=0;s<AMBIENT_CHOIR_SINGERS;++s) { c->ph[s]+=c->inc[s]; if(c->ph[s]>=1.0f) c->ph[s]-=1.0f; }
    c->rng=c->rng*1664525u+1013904223u;
    float breath=(float)(int32_t)c->rng*(1.0f/2147483648.0f)*.05f;
    float x=(s0+s1+s2)*(1.0f/3.0f)+breath;
    float y=dsp_svf_lp(&c->f1,x);
    y=dsp_svf_lp(&c->f2,y);
    y=dsp_svf_lp(&c->f3,y*(1.0f+.25f*shade));
    y=dsp_svf_lp(&c->f4,y);
    *side=dsp_svf_bp(&c->side,(s0-s2)*.5f)*.30f*c->norm;
    return y*c->norm;
}
#endif
