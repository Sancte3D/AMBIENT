#ifndef FAM_WOODLAND_BODY_H
#define FAM_WOODLAND_BODY_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "dsp.h"

/* Model 1: two quiet, string-driven harmonic body modes. Independent float
 * design inspired by DRUID's resonant-body principle, no Tonitor code/tables.
 * Main prepares fixed unity-peak bandpasses at 2f/3f before voice publication.
 * Audio owns all filter histories. No excitation, pitch/loop feedback, extra
 * voice, delay bank, free-running tone or tail outside the owning string.
 * Clear/reuse clears the histories; future-note switches never edit them. */
#define WOODLAND_BODY_MODEL 1
typedef struct {
    float a1[2],a2[2],b[2],y1[2],y2[2],x1,x2;
    uint8_t modes;
} woodland_body_t;

static inline void woodland_body_prepare(woodland_body_t *b,float hz,bool on) {
    memset(b,0,sizeof *b);
    if(!on || !isfinite(hz) || hz<=0.0f) return;
    for(int m=0;m<2;++m) {
        float f=hz*(float)(m+2);
        /* Out-of-band modes are omitted, never clamped to a wrong pitch. */
        if(f>=.45f*DSP_SAMPLE_RATE_HZ) continue;
        float w=6.283185307179586f*f/DSP_SAMPLE_RATE_HZ;
        float alpha=sinf(w)/12.0f; /* Q=6; unity peak, short owned memory */
        float norm=1.0f/(1.0f+alpha);
        b->b[m]=alpha*norm;
        b->a1[m]=2.0f*cosf(w)*norm;
        b->a2[m]=(1.0f-alpha)*norm;
        b->modes|=(uint8_t)(1u<<m);
    }
}
static inline float woodland_body_step(woodland_body_t *b,float x) {
    if(!b->modes) return x; /* exact bypass, no state movement */
    float body=0.0f;
    for(int m=0;m<2;++m) {
        if(!(b->modes&(1u<<m))) continue;
        float y=b->b[m]*(x-b->x2)+b->a1[m]*b->y1[m]-b->a2[m]*b->y2[m];
        b->y2[m]=b->y1[m];b->y1[m]=y;
        body+=(m==0 ? .10f : .06f)*y;
    }
    b->x2=b->x1;b->x1=x;
    return x+body;
}
#endif
