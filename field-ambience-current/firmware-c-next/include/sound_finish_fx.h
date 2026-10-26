#ifndef FAM_SOUND_FINISH_FX_H
#define FAM_SOUND_FINISH_FX_H
#define SOUND_FINISH_MODEL 1
#include <stdatomic.h>
#include <string.h>
#include "dsp.h"
/* Independent float design, no Tonitor source/tables. Figure-eight plate,
 * stereo music delay, two unity-speed grains; optional soft color, crush and
 * phase texture. Nature is mixed afterwards. All storage belongs to the one
 * music bus, all histories clear together. No transposing read or feedback
 * from the grain output. Frozen content locks the actual pitch ledger. */
enum { SF_PLATE=11042,SF_DELAY=12288,SF_GRAIN=16384,SF_WINDOW=8192 };
typedef struct {
    float plate[SF_PLATE],delay[2][SF_DELAY],grain[SF_GRAIN];
    unsigned at[12],delay_at,grain_at,grain_read[2],grain_age[2],rng;
    float fb[2],damp[2],delay_lp[2],phase_z[2][4],phase;
    float delay_mix,grain_mix,drive_mix,crush_mix,phase_mix,decay;
    bool cold;
} sound_finish_fx_t;
static sound_finish_fx_t sf;
static _Atomic uint32_t sf_target[5]; /* Q16 independent macro targets */
static _Atomic bool sf_freeze,sf_plate_on,sf_content_held;
static float sf_controls[5];
static bool sf_is_frozen;
static void sf_accept(void) {
    for(int i=0;i<5;++i)sf_controls[i]=(float)atomic_load_explicit(&sf_target[i],memory_order_acquire)/65536;
    sf_is_frozen=atomic_load_explicit(&sf_freeze,memory_order_acquire);
    atomic_store_explicit(&sf_content_held,sf_is_frozen && !sf.cold,memory_order_release);
}
static void sf_parameters(float delay,float grain,bool freeze,float drive,float crush,float phaser,bool plate) {
    float v[5]={delay,grain,drive,crush,phaser};
    for(int i=0;i<5;++i)if(!isfinite(v[i]))return;
    for(int i=0;i<5;++i)atomic_store(&sf_target[i],(uint32_t)(dsp_clampf(v[i],0,1)*65536));
    atomic_store(&sf_plate_on,plate);atomic_store(&sf_freeze,freeze);
}
static void sf_clear(void) {
    if(sf.cold)return;
    atomic_store_explicit(&sf_content_held,false,memory_order_release);
    memset(&sf,0,sizeof sf);sf.cold=true;
}
static void sf_init(void) {
    sf.cold=false;sf_clear();sf.rng=0x61c88647u;
    sf_parameters(.16f,.12f,false,.04f,0,0,true);
}
static float sf_line(int k,float x,float g) {
    static const unsigned len[12]={149,211,263,293,347,2341,181,1867,421,2719,233,2017};
    static const unsigned off[12]={0,149,360,623,916,1263,3604,3785,5652,6073,8792,9025};
    unsigned pos=off[k]+sf.at[k];float d=sf.plate[pos],out=d-g*x;
    sf.plate[pos]=x+g*out;if(++sf.at[k]==len[k])sf.at[k]=0;
    return out;
}
static void sf_process(float *l,float *r,const float *sl,const float *sr,int frames,
                       float amount,float decay,bool enabled,bool plate) {
    const float *target=sf_controls;bool frozen=sf_is_frozen;
    if(!enabled || amount==0) {sf_clear();return;}
    if(sf.cold) {
        sf.grain_age[1]=SF_WINDOW/2;sf.cold=false;sf.rng=0x61c88647u;
        sf.grain_read[0]=SF_GRAIN-10000;sf.grain_read[1]=SF_GRAIN-12000;
    }
    const float smooth=1.0f/(.080f*DSP_SAMPLE_RATE_HZ);
    for(int n=0;n<frames;++n) {
        sf.delay_mix+=(target[0]-sf.delay_mix)*smooth;
        sf.grain_mix+=(target[1]-sf.grain_mix)*smooth;
        sf.drive_mix+=(target[2]-sf.drive_mix)*smooth;
        sf.crush_mix+=(target[3]-sf.crush_mix)*smooth;
        sf.phase_mix+=(target[4]-sf.phase_mix)*smooth;
        sf.decay+=(decay-sf.decay)*smooth;
        float in[2]={sl[n],sr[n]},wet[2]={0,0};
        float mono=.5f*(in[0]+in[1]);
        if(!frozen)sf.grain[sf.grain_at]=mono;
        float gs=0,ws=0;
        for(int k=0;k<2;++k) {
            float t=(float)sf.grain_age[k]/SF_WINDOW,w=4*t*(1-t);
            gs+=w*sf.grain[sf.grain_read[k]];ws+=w;
            sf.grain_read[k]=(sf.grain_read[k]+1u)%SF_GRAIN;
            if(++sf.grain_age[k]==SF_WINDOW) {
                sf.grain_age[k]=0;sf.rng=sf.rng*1664525u+1013904223u;
                unsigned back=8192u+(sf.rng>>20);
                sf.grain_read[k]=(sf.grain_at+SF_GRAIN-back)%SF_GRAIN;
            }
        }
        if(!frozen)sf.grain_at=(sf.grain_at+1u)%SF_GRAIN;
        float grain=ws>0 ? gs/ws : 0;
        unsigned dl=(sf.delay_at+SF_DELAY-9261u)%SF_DELAY;
        unsigned dr=(sf.delay_at+SF_DELAY-11775u)%SF_DELAY;
        float echoes[2]={sf.delay[0][dl],sf.delay[1][dr]};
        for(int c=0;c<2;++c) {
            sf.delay_lp[c]+=.23f*(in[c]+.32f*echoes[1-c]-sf.delay_lp[c]);
            sf.delay[c][sf.delay_at]=sf.delay_lp[c];
            wet[c]=.65f*sf.delay_mix*echoes[c]+.55f*sf.grain_mix*grain;
        }
        sf.delay_at=(sf.delay_at+1u)%SF_DELAY;
        if(plate) {
            float x=mono;
            for(int k=0;k<4;++k)x=sf_line(k,x,k<2 ? .6f : .5f);
            float input[2]={x*(1-sf.decay)+sf.fb[1]*sf.decay,
                            x*(1-sf.decay)+sf.fb[0]*sf.decay};
            float a[2],b[2];
            for(int c=0;c<2;++c) {
                int k=c ? 8 : 4;
                a[c]=sf_line(k+1,sf_line(k,input[c],.55f),0);
                sf.damp[c]+=.28f*(a[c]-sf.damp[c]);
                b[c]=sf_line(k+3,sf_line(k+2,sf.damp[c],.45f),0);
                sf.fb[c]=b[c];
            }
            wet[0]+=1.4f*(a[0]+b[0]+.22f*a[1]);
            wet[1]+=1.4f*(a[1]+b[1]+.22f*a[0]);
        }
        sf.phase+=.07f/DSP_SAMPLE_RATE_HZ;if(sf.phase>=1)sf.phase-=1;
        float coefficient=.35f+.12f*dsp_sin(sf.phase);
        for(int c=0;c<2;++c) {
            float x=wet[c],phase=x;
            for(int k=0;k<4;++k) {
                float y=-coefficient*phase+sf.phase_z[c][k];
                sf.phase_z[c][k]=phase+coefficient*y;phase=y;
            }
            x+=(.5f*(x+phase)-x)*sf.phase_mix;
            x=x/(1+2*sf.drive_mix*fabsf(x));
            float q=4096-4064*sf.crush_mix;
            if(sf.crush_mix>.0001f)x+=(floorf(x*q+.5f)/q-x)*sf.crush_mix;
            wet[c]=x*amount;
        }
        l[n]+=wet[0];r[n]+=wet[1];
    }
}
#endif
