#ifndef FAM_SOUND_FINISH_FX_H
#define FAM_SOUND_FINISH_FX_H
#define SOUND_FINISH_MODEL 1
#include <stdatomic.h>
#include <string.h>
#include "dsp.h"
#include "sound_parameters.h"
/* Independent float design, no Tonitor source/tables. Figure-eight plate,
 * stereo music delay, two unity-speed grains; optional soft color, crush and
 * phase texture. Nature is mixed afterwards. All storage belongs to the one
 * music bus, all histories clear together. No transposing read or feedback
 * from the grain output. Frozen content locks the actual pitch ledger. */
enum { SF_PLATE=11042,SF_DELAY=32768,SF_GRAIN=16384,SF_WINDOW=8192 };
/* D1, deliberately outside the room's D2 arena. Never bulk-cleared in the
 * IRQ: delay_valid makes every pre-Clear sample unreachable until rewritten. */
static float sf_delay[2][SF_DELAY] __attribute__((section(".sound_delay_bss"),aligned(32)));
typedef struct {
    float plate[SF_PLATE],grain[SF_GRAIN];
    unsigned at[12],delay_at,grain_at,grain_read[2],grain_age[2],rng;
    unsigned delay_valid,delay_frames[2],delay_from[2],grain_window;
    float fb[2],damp[2],delay_lp[2],phase_z[2][4],phase;
    float delay_mix,grain_mix,drive_mix,crush_mix,phase_mix,decay;
    float delay_fade,feedback,grain_gain,phase_rate;
    bool cold;
} sound_finish_fx_t;
static sound_finish_fx_t sf;
static _Atomic uint32_t sf_target[5]; /* Q16 independent macro targets */
static _Atomic uint32_t sf_detail[5]; /* float32 control values, same transaction */
static _Atomic bool sf_freeze,sf_plate_on,sf_content_held;
static _Atomic unsigned sf_version;
static bool sf_accepted_plate=true;
static float sf_controls[10];
static sound_parameters_t sf_desired;
static bool sf_is_frozen;
static void sf_accept(void) {
    unsigned before=atomic_load_explicit(&sf_version,memory_order_seq_cst);
    if(before&1u)return;
    float next[10];
    for(int i=0;i<5;++i)next[i]=(float)atomic_load_explicit(&sf_target[i],memory_order_seq_cst)/65536;
    for(int i=0;i<5;++i) {uint32_t n=atomic_load_explicit(&sf_detail[i],memory_order_seq_cst);memcpy(next+5+i,&n,4);}
    bool frozen=atomic_load_explicit(&sf_freeze,memory_order_seq_cst);
    bool plate=atomic_load_explicit(&sf_plate_on,memory_order_seq_cst);
    if(before!=atomic_load_explicit(&sf_version,memory_order_seq_cst))return;
    memcpy(sf_controls,next,sizeof next);sf_is_frozen=frozen;sf_accepted_plate=plate;
    atomic_store_explicit(&sf_content_held,sf_is_frozen && !sf.cold,memory_order_release);
}
static bool sf_apply_parameters(const sound_parameters_t *p) {
    sound_parameters_t normalized;if(!sound_parameters_normalize(p,&normalized))return false;
    sf_desired=normalized;p=&normalized;
    float v[10]={p->delay,p->grain,p->drive,p->crush,p->phaser,
        p->delay_left_ms,p->delay_right_ms,p->feedback,p->grain_ms,p->phaser_hz};
    unsigned version=atomic_load_explicit(&sf_version,memory_order_seq_cst);
    atomic_store_explicit(&sf_version,version+1u,memory_order_seq_cst);
    for(int i=0;i<5;++i)atomic_store(&sf_target[i],(uint32_t)(dsp_clampf(v[i],0,1)*65536));
    for(int i=0;i<5;++i) {uint32_t n;memcpy(&n,v+5+i,4);atomic_store(&sf_detail[i],n);}
    atomic_store(&sf_plate_on,p->plate);atomic_store(&sf_freeze,p->freeze);
    atomic_store_explicit(&sf_version,version+2u,memory_order_seq_cst);
    return true;
}
static inline void sf_parameters(float delay,float grain,bool freeze,float drive,float crush,float phaser,bool plate) {
    sound_parameters_t p=sf_desired;
    p.delay=delay;p.grain=grain;p.drive=drive;p.crush=crush;p.phaser=phaser;p.freeze=freeze;p.plate=plate;
    (void)sf_apply_parameters(&p);
}
static void sf_clear(void) {
    if(sf.cold)return;
    atomic_store_explicit(&sf_content_held,false,memory_order_release);
    memset(&sf,0,sizeof sf);sf.cold=true;
}
static void sf_init(void) {
    sf.cold=false;sf_clear();sf.rng=0x61c88647u;
    sound_parameters_t p=sound_parameters_default();(void)sf_apply_parameters(&p);
}
static unsigned sf_frames(float ms) {return (unsigned)(ms*(DSP_SAMPLE_RATE_HZ/1000.0f)+.5f);}
static float sf_echo(int channel,unsigned lag) {
    return lag<=sf.delay_valid ? sf_delay[channel][(sf.delay_at+SF_DELAY-lag)%SF_DELAY] : 0;
}
static unsigned sf_grain_back(unsigned window,unsigned random) {
    unsigned span=SF_GRAIN-window;if(span>4096)span=4096;
    return window+((random>>20)*span)/4096u;
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
        sf.grain_window=sf_frames(target[8]);sf.grain_age[1]=sf.grain_window/2;
        sf.grain_gain=1;sf.cold=false;sf.rng=0x61c88647u;
        unsigned a=sf.grain_window+1808u,b=sf.grain_window+3808u;
        if(a>=SF_GRAIN)a=SF_GRAIN-1;
        if(b>=SF_GRAIN)b=SF_GRAIN-1;
        sf.grain_read[0]=SF_GRAIN-a;sf.grain_read[1]=SF_GRAIN-b;
        for(int c=0;c<2;++c)sf.delay_from[c]=sf.delay_frames[c]=sf_frames(target[5+c]);
        sf.delay_fade=1;sf.feedback=target[7];sf.phase_rate=target[9];
    }
    const float smooth=1.0f/(.080f*DSP_SAMPLE_RATE_HZ);
    for(int n=0;n<frames;++n) {
        sf.delay_mix+=(target[0]-sf.delay_mix)*smooth;
        sf.grain_mix+=(target[1]-sf.grain_mix)*smooth;
        sf.drive_mix+=(target[2]-sf.drive_mix)*smooth;
        sf.crush_mix+=(target[3]-sf.crush_mix)*smooth;
        sf.phase_mix+=(target[4]-sf.phase_mix)*smooth;
        sf.decay+=(decay-sf.decay)*smooth;
        sf.feedback+=(target[7]-sf.feedback)*smooth;
        sf.phase_rate+=(target[9]-sf.phase_rate)*smooth;
        /* Fixed read heads, crossfaded on real samples. No continuously
         * moving delay tap (and consequently no Doppler pitch slide). */
        if(sf.delay_fade>=1 && (sf_frames(target[5])!=sf.delay_frames[0] || sf_frames(target[6])!=sf.delay_frames[1])) {
            for(int c=0;c<2;++c) {sf.delay_from[c]=sf.delay_frames[c];sf.delay_frames[c]=sf_frames(target[5+c]);}
            sf.delay_fade=0;
        }
        unsigned window=sf_frames(target[8]);
        float grain_goal=window==sf.grain_window ? 1 : 0;
        const float fade_step=1.0f/(.040f*DSP_SAMPLE_RATE_HZ);
        sf.grain_gain+=dsp_clampf(grain_goal-sf.grain_gain,-fade_step,fade_step);
        if(sf.grain_gain==0 && window!=sf.grain_window) {
            sf.grain_window=window;sf.grain_age[0]=0;sf.grain_age[1]=window/2;
            for(int k=0;k<2;++k) {
                sf.rng=sf.rng*1664525u+1013904223u;
                sf.grain_read[k]=(sf.grain_at+SF_GRAIN-sf_grain_back(window,sf.rng))%SF_GRAIN;
            }
        }
        float in[2]={sl[n],sr[n]},wet[2]={0,0};
        float mono=.5f*(in[0]+in[1]);
        if(!frozen)sf.grain[sf.grain_at]=mono;
        float gs=0,ws=0;
        for(int k=0;k<2;++k) {
            float t=(float)sf.grain_age[k]/sf.grain_window,w=4*t*(1-t);
            gs+=w*sf.grain[sf.grain_read[k]];ws+=w;
            sf.grain_read[k]=(sf.grain_read[k]+1u)%SF_GRAIN;
            if(++sf.grain_age[k]==sf.grain_window) {
                sf.grain_age[k]=0;sf.rng=sf.rng*1664525u+1013904223u;
                unsigned back=sf_grain_back(sf.grain_window,sf.rng);
                sf.grain_read[k]=(sf.grain_at+SF_GRAIN-back)%SF_GRAIN;
            }
        }
        if(!frozen)sf.grain_at=(sf.grain_at+1u)%SF_GRAIN;
        float grain=(ws>0 ? gs/ws : 0)*sf.grain_gain;
        float echoes[2];
        for(int c=0;c<2;++c) {
            float old=sf_echo(c,sf.delay_from[c]),next=sf_echo(c,sf.delay_frames[c]);
            echoes[c]=old+(next-old)*sf.delay_fade;
        }
        for(int c=0;c<2;++c) {
            sf.delay_lp[c]+=.23f*(in[c]+sf.feedback*echoes[1-c]-sf.delay_lp[c]);
            sf_delay[c][sf.delay_at]=sf.delay_lp[c];
            wet[c]=.65f*sf.delay_mix*echoes[c]+.55f*sf.grain_mix*grain;
        }
        sf.delay_at=(sf.delay_at+1u)%SF_DELAY;
        if(sf.delay_valid<SF_DELAY)++sf.delay_valid;
        if(sf.delay_fade<1)sf.delay_fade=fminf(1,sf.delay_fade+fade_step);
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
        sf.phase+=sf.phase_rate/DSP_SAMPLE_RATE_HZ;if(sf.phase>=1)sf.phase-=1;
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
