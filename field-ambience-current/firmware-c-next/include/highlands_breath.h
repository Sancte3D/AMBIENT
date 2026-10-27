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
#endif
