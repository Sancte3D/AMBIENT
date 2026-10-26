#ifndef FAM_SOFT_PAD_H
#define FAM_SOFT_PAD_H
#include "dsp.h"
/* Independent HIVE-inspired family-limited design: seven strictly harmonic
 * sine components at the admitted Hz. No detuning, pitch LFO, noise or extra
 * source. The existing owner/envelope/color/physical expression surrounds it. */
static inline float soft_pad_sample(float phase,float hz,float shade) {
    static const float weight[7]={.72f,.15f,.065f,.035f,.017f,.009f,.004f};
    float out=0,norm=0;
    for(int k=0;k<7;++k) {
        if(hz*(k+1)>=.45f*DSP_SAMPLE_RATE_HZ)break;
        float w=weight[k]*(k==0 ? 1 : 1+.32f*shade);
        out+=w*dsp_sin(phase*(k+1));norm+=weight[k];
    }
    return out*(.82f/norm);
}
#endif
