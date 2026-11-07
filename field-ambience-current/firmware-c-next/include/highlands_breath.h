#ifndef FAM_HIGHLANDS_BREATH_H
#define FAM_HIGHLANDS_BREATH_H
#include "dsp.h"
/* Material model 2: hollow odd-harmonic pipe. The third is a defining part
 * of the sustained note, not a short onset detail. Sparse upper partials
 * roll off rapidly; no sub-octave, detuning, fixed formant pitch or FM ratio.
 * Breath noise and its envelope live in the same owning horn voice. */
static inline float highlands_breath_sample(float phase,float hz,float shade) {
    static const float weight[5]={.36f,.30f,.085f,.018f,.004f};
    shade=dsp_clampf(shade,-1,1);
    float out=0,norm=0;
    for(int k=0;k<5;++k) {
        int harmonic=2*k+1;
        if(hz*harmonic>=.45f*DSP_SAMPLE_RATE_HZ)break;
        float w=weight[k]*(k==0 ? 1 : 1+.14f*shade);
        out+=w*dsp_sin(phase*harmonic);norm+=w;
    }
    return norm>0 ? .90f*out/norm : 0;
}
#ifdef FAM_SOUND_AMBIENT_V3
/* Ambient voicing: a pure ocarina/whistle tone. The fundamental carries the
 * note, upper partials are faint, so it reads as air rather than a clarinet
 * or horn; the breathy colour comes from tuned breath noise in horn.c.
 * Color tilts the upper partials only. Same admitted Hz. The purer tone
 * concentrates its energy in the fundamental, so the peak is .73 instead of
 * .90: this keeps the World loudness balance of the complete sound. */
static inline float highlands_air_sample(float phase,float hz,float shade) {
    static const float weight[5]={1.0f,.14f,.06f,.025f,.01f};
    shade=dsp_clampf(shade,-1,1);
    float out=0,norm=0;
    for(int k=0;k<5;++k) {
        if(hz*(float)(k+1)>=.45f*DSP_SAMPLE_RATE_HZ)break;
        float w=weight[k]*(k ? 1+.30f*shade : 1);
        out+=w*dsp_sin(phase*(float)(k+1));norm+=w;
    }
    return norm>0 ? .73f*out/norm : 0;
}
#endif
#endif
