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
#endif
