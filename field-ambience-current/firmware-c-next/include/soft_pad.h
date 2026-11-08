#ifndef FAM_SOFT_PAD_H
#define FAM_SOFT_PAD_H
#include "dsp.h"
/* Material model 2: continuously driven, even-rich silk pad. A broad second
 * harmonic and a smaller fourth move in opposite directions; Color can never
 * reduce this material to the previous fundamental-dominated sine. All seven
 * components belong to the admitted Hz. No detune, pitch LFO or extra owner. */
static inline float soft_pad_sample(float phase,float hz,float shade) {
    static const float weight[7]={.44f,.30f,.045f,.115f,.014f,.035f,.008f};
    shade=dsp_clampf(shade,-1.0f,1.0f);
    float out=0,norm=0;
    for(int k=0;k<7;++k) {
        if(hz*(k+1)>=.45f*DSP_SAMPLE_RATE_HZ)break;
        float movement=k==1 ? .22f : k==3 ? -.20f : k>0 ? .12f : 0;
        float w=weight[k]*(1+movement*shade);
        out+=w*dsp_sin(phase*(k+1));norm+=w;
    }
    return out*(.95f/norm);
}
#ifdef FAM_SOUND_AMBIENT_V3
/* Ambient voicing: a warm string-ensemble bed. Eight partials on one smooth
 * 1/k^1.4 slope: rich and clearly brighter than the HIGHLANDS whistle, but
 * without the strong second/fourth bumps that made the low pad read as a
 * horn. Color tilts the upper partials only; the fundamental always leads. */
static inline float soft_pad_v3_sample(float phase,float hz,float shade) {
    static const float weight[8]={1.0f,.379f,.215f,.144f,.105f,.081f,.066f,.054f};
    shade=dsp_clampf(shade,-1.0f,1.0f);
    float out=0,norm=0;
    for(int k=0;k<8;++k) {
        if(hz*(k+1)>=.45f*DSP_SAMPLE_RATE_HZ)break;
        float w=weight[k]*(k ? 1.0f+.35f*shade*(float)k/7.0f : 1.0f);
        out+=w*dsp_sin(phase*(k+1));norm+=w;
    }
    return out*(.95f/norm);
}
/* The ensemble partner: first and second partial of a companion a few cents
 * away, panned to the other side. Slow beating and width, Juno-style. */
static inline float soft_pad_v3_partner(float phase,float hz) {
    float out=dsp_sin(phase),norm=1.0f;
    if(2.0f*hz<.45f*DSP_SAMPLE_RATE_HZ) {out+=.35f*dsp_sin(2.0f*phase);norm+=.35f;}
    return out*(.95f/norm);
}
#endif
#endif
