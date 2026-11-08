#ifndef FAM_SOUND_FINISH_FX_H
#define FAM_SOUND_FINISH_FX_H
#define SOUND_FINISH_MODEL 2
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
    unsigned delay_valid,delay_frames[2],delay_from[2],grain_window,grain_nonzero,room_tick;
    float fb[2],damp[2],delay_lp[2],phase_z[2][4],phase;
    float delay_mix,grain_mix,drive_mix,crush_mix,phase_mix,decay;
    float delay_fade,feedback,grain_gain,phase_rate;
    float room_power[2],room_dc[2][2],room_gain,room_goal;
#ifdef FAM_SOUND_AMBIENT_V3
    unsigned wg_read[2],wg_age[2],wg_window; int wg_speed;
    float vast,shimmer,echo,wgrain,dlp;
#endif
    bool cold;
} sound_finish_fx_t;
static sound_finish_fx_t sf;
static _Atomic uint32_t sf_target[5]; /* Q16 independent macro targets */
static _Atomic uint32_t sf_detail[5]; /* float32 control values, same transaction */
static _Atomic bool sf_freeze,sf_plate_on,sf_content_held,sf_content_available;
static _Atomic unsigned sf_version;
static bool sf_accepted_plate=true;
static float sf_controls[10];
static sound_parameters_t sf_desired;
static bool sf_is_frozen;
static void sf_publish_content(void) {
    bool available=!sf.cold && sf.grain_nonzero>0;
    atomic_store_explicit(&sf_content_available,available,memory_order_release);
    atomic_store_explicit(&sf_content_held,available && sf_is_frozen && sf_controls[1]>0,memory_order_release);
}
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
    sf_publish_content();
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
    atomic_store_explicit(&sf_content_available,false,memory_order_release);
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
/* Level-match the room return, never the direct voices or extra textures.
 * Same -24/-18/-14.5/-12 dB nominal Dry ratio at Room .25/.5/.75/1 for both
 * tanks. A bounded, slow scalar retains each tank's spectrum/stereo/decay.
 * Stop adapting below the input floor so an outgoing tail keeps its gain.
 * sqrt runs once per 64 real samples, independent of render partition. */
#ifdef FAM_SOUND_AMBIENT_V3
/* Instrument Worlds sculpt the shared music effects, after the S-4: their
 * own echo times, more echo and grains, echoes that feed the plate ("Vast")
 * and a second pair of grains an octave up (FJORD, GLACIER) or reversed
 * (MEADOW). Octave grains keep every pitch class, reversed ones every pitch,
 * so the admission ledger stays true. The World adds its own echo, grain and
 * shimmer returns (relative to the send) on top of the user's amounts, which
 * keep scaling them; the classic Worlds use the stored values unchanged.
 * First echoes sit about 11-14 dB under the dry voices, as an S-4 Vast. */
typedef struct {
    float echo,delay_ms[2],feedback,delay_lp; /* extra echo return, times, feedback, loop tone */
    float grain,grain_ms;                      /* extra grain return, grain length */
    float vast;                                /* echoes and shimmer into the plate */
    float shimmer,shimmer_ms; int speed;       /* world grains: return, length, 2 octave / -1 reverse */
} sf_world_t;
static const sf_world_t SF_WORLD[4]={
    {0,{0,0},0,.23f,0,0,0,0,0,1},
    /* FJORD   */ {.70f,{480,620},.46f,.30f,.45f,240,1.0f,.35f,160, 2},
    /* MEADOW  */ {.60f,{330,495},.40f,.25f,.55f, 70,.30f,.55f,120,-1},
    /* GLACIER */ {.80f,{610,690},.44f,.38f,.35f,160,1.0f,.70f,140, 2},
};
static _Atomic uint32_t sf_world_wet_bits,sf_world_sel;
static float sf_world_controls[10],sf_world_vast,sf_world_shimmer,sf_world_echo,sf_world_grain,sf_world_dlp=.23f;
static int sf_world_on,sf_world_speed;
static unsigned sf_world_window;
/* Main: the selected instrument World (0 = a classic World) and its room return. */
static inline void sf_set_world(int sel,float wet) {
    uint32_t b; memcpy(&b,&wet,4); atomic_store_explicit(&sf_world_wet_bits,b,memory_order_release);
    atomic_store_explicit(&sf_world_sel,(uint32_t)(sel>0 && sel<4 ? sel : 0),memory_order_release);
}
/* Audio, once per block after sf_accept: the World's effective targets. */
static void sf_world_accept(void) {
    int sel=(int)atomic_load_explicit(&sf_world_sel,memory_order_acquire);
    if(sel<=0 || sel>=4) { sf_world_on=0; sf_world_vast=sf_world_shimmer=sf_world_echo=sf_world_grain=0; return; }
    const sf_world_t *w=&SF_WORLD[sel]; const float *u=sf_controls;
    memcpy(sf_world_controls,u,sizeof sf_world_controls);
    sf_world_controls[5]=w->delay_ms[0]; sf_world_controls[6]=w->delay_ms[1];
    sf_world_controls[7]=dsp_clampf(w->feedback+(u[7]-.32f),0,.5f);
    sf_world_controls[8]=w->grain_ms;
    const float echo_scale=dsp_clampf(u[0]*(1.0f/.16f),0,2),grain_scale=dsp_clampf(u[1]*(1.0f/.12f),0,2);
    sf_world_echo=w->echo*echo_scale; sf_world_grain=w->grain*grain_scale;
    sf_world_vast=w->vast; sf_world_shimmer=w->shimmer*grain_scale; sf_world_dlp=w->delay_lp;
    sf_world_speed=w->speed; sf_world_window=sf_frames(w->shimmer_ms); sf_world_on=sel;
}
/* A world grain's start: far enough behind (octave) or ahead of the reverse
 * path (reversed) that its read never meets the write head, frozen or not. */
static unsigned sf_world_back(unsigned window,int speed,unsigned random) {
    unsigned lo,hi;
    if(speed>1) { lo=2*window+64; hi=SF_GRAIN-1; if(hi>lo+4096) hi=lo+4096; }
    else { lo=64; hi=SF_GRAIN-1-2*window; if(hi>lo+4096) hi=lo+4096; }
    if(hi<=lo) return lo;
    return lo+((random>>20)*(hi-lo))/4096u;
}
static float sf_world_grains(float *side) {
    if(sf.wg_window!=sf_world_window || sf.wg_speed!=sf_world_speed) {
        /* A new World starts its grains afresh (they are faded in by shimmer). */
        sf.wg_window=sf_world_window; sf.wg_speed=sf_world_speed;
        for(int k=0;k<2;++k) {
            sf.wg_age[k]=k ? sf.wg_window/2 : 0;
            sf.rng=sf.rng*1664525u+1013904223u;
            sf.wg_read[k]=(sf.grain_at+SF_GRAIN-sf_world_back(sf.wg_window,sf.wg_speed,sf.rng))%SF_GRAIN;
        }
    }
    if(sf.wg_window<64) { *side=0; return 0; }
    float g[2],w[2],ws=0;
    for(int k=0;k<2;++k) {
        float t=(float)sf.wg_age[k]/sf.wg_window; w[k]=4*t*(1-t); ws+=w[k];
        g[k]=sf.grain[sf.wg_read[k]];
        sf.wg_read[k]=(sf.wg_read[k]+(unsigned)(SF_GRAIN+sf.wg_speed))%SF_GRAIN;
        if(++sf.wg_age[k]>=sf.wg_window) {
            sf.wg_age[k]=0; sf.rng=sf.rng*1664525u+1013904223u;
            sf.wg_read[k]=(sf.grain_at+SF_GRAIN-sf_world_back(sf.wg_window,sf.wg_speed,sf.rng))%SF_GRAIN;
        }
    }
    if(ws<=0) { *side=0; return 0; }
    float inv=1.0f/(.7f*ws);
    *side=(.4f*w[0]*g[0]+w[1]*g[1])*inv;
    return (w[0]*g[0]+.4f*w[1]*g[1])*inv;
}
#endif
static float sf_room_level(const float *in,const float *room) {
    float power[2]={0,0};
    for(int c=0;c<2;++c) {
        sf.room_dc[0][c]+=.00228f*(in[c]-sf.room_dc[0][c]);
        sf.room_dc[1][c]+=.00228f*(room[c]-sf.room_dc[1][c]);
        float a=in[c]-sf.room_dc[0][c],b=room[c]-sf.room_dc[1][c];
        power[0]+=.5f*a*a;power[1]+=.5f*b*b;
    }
    for(int k=0;k<2;++k)sf.room_power[k]+=(power[k]-sf.room_power[k])*(1.0f/(.250f*DSP_SAMPLE_RATE_HZ));
    if((sf.room_tick++&63u)==0 && power[0]>1e-12f && sf.room_power[0]>1e-10f && sf.room_power[1]>1e-12f)
#ifdef FAM_SOUND_AMBIENT_V3
        /* Ambient voicing: the room is part of the instrument. Nominal return
         * .9*Room of Dry (about -7 dB at Room .5, -1 dB at 1) instead of .25. */
    {
        /* The World's room return: 1 in the classic Worlds. */
        uint32_t wb=atomic_load_explicit(&sf_world_wet_bits,memory_order_acquire); float wet; memcpy(&wet,&wb,4);
        if(!(wet>0)) wet=1;
        sf.room_goal=dsp_clampf(wet*sqrtf((.90f/.35f)*(.90f/.35f)*sf.room_power[0]/sf.room_power[1]),.125f,32);
    }
#else
        sf.room_goal=dsp_clampf(sqrtf((.25f/.35f)*(.25f/.35f)*sf.room_power[0]/sf.room_power[1]),.125f,8);
#endif
    sf.room_gain+=(sf.room_goal-sf.room_gain)*(1.0f/(.500f*DSP_SAMPLE_RATE_HZ));
    return sf.room_gain;
}
static void sf_process(float *l,float *r,const float *sl,const float *sr,int frames,
                       float amount,float decay,bool enabled,bool plate,float fdn_l,float fdn_r) {
    const float *target=sf_controls;bool frozen=sf_is_frozen;
#ifdef FAM_SOUND_AMBIENT_V3
    if(sf_world_on) target=sf_world_controls;
#endif
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
        sf.room_gain=sf.room_goal=plate ? 1 : 4;
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
        if(!frozen) {
            if(fabsf(sf.grain[sf.grain_at])>1e-7f)--sf.grain_nonzero;
            sf.grain[sf.grain_at]=mono;
            if(fabsf(mono)>1e-7f)++sf.grain_nonzero;
        }
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
#ifdef FAM_SOUND_AMBIENT_V3
        float shim[2]={0,0},vast=0,wecho=0,wgrain=0,dlp=.23f;
        const bool world_fx=sf_world_on || sf.shimmer>0 || sf.vast>0 || sf.echo>0 || sf.wgrain>0;
        if(world_fx) {
            if(sf.dlp==0) sf.dlp=.23f;
            sf.shimmer+=(sf_world_shimmer-sf.shimmer)*smooth;
            sf.vast+=(sf_world_vast-sf.vast)*smooth;
            sf.echo+=(sf_world_echo-sf.echo)*smooth;
            sf.wgrain+=(sf_world_grain-sf.wgrain)*smooth;
            sf.dlp+=((sf_world_on ? sf_world_dlp : .23f)-sf.dlp)*smooth;
            if(!sf_world_on && sf.shimmer<1e-6f && sf.vast<1e-6f && sf.echo<1e-6f && sf.wgrain<1e-6f) {
                sf.shimmer=sf.vast=sf.echo=sf.wgrain=0; sf.dlp=.23f;
            }
            float side,s=sf_world_grains(&side);
            shim[0]=s*sf.shimmer; shim[1]=side*sf.shimmer; vast=sf.vast;
            wecho=sf.echo; wgrain=sf.wgrain; dlp=sf.dlp;
        }
#endif
        float echoes[2];
        for(int c=0;c<2;++c) {
            float old=sf_echo(c,sf.delay_from[c]),next=sf_echo(c,sf.delay_frames[c]);
            echoes[c]=old+(next-old)*sf.delay_fade;
        }
        for(int c=0;c<2;++c) {
#ifdef FAM_SOUND_AMBIENT_V3
            if(world_fx) sf.delay_lp[c]+=dlp*(in[c]+sf.feedback*echoes[1-c]-sf.delay_lp[c]);
            else
#endif
            sf.delay_lp[c]+=.23f*(in[c]+sf.feedback*echoes[1-c]-sf.delay_lp[c]);
            sf_delay[c][sf.delay_at]=sf.delay_lp[c];
            wet[c]=.65f*sf.delay_mix*echoes[c]+.55f*sf.grain_mix*grain;
#ifdef FAM_SOUND_AMBIENT_V3
            if(world_fx) wet[c]+=wecho*echoes[c]+wgrain*grain+shim[c];
#endif
        }
        sf.delay_at=(sf.delay_at+1u)%SF_DELAY;
        if(sf.delay_valid<SF_DELAY)++sf.delay_valid;
        if(sf.delay_fade<1)sf.delay_fade=fminf(1,sf.delay_fade+fade_step);
        float room[2]={fdn_l,fdn_r};
        if(plate) {
            float x=mono;
#ifdef FAM_SOUND_AMBIENT_V3
            /* Vast: the echoes and the octave grains also sing in the plate. */
            if(vast>0 || shim[0]!=0) x+=vast*.5f*(echoes[0]+echoes[1])+.5f*(shim[0]+shim[1]);
#endif
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
            room[0]=1.4f*(a[0]+b[0]+.22f*a[1]);
            room[1]=1.4f*(a[1]+b[1]+.22f*a[0]);
        }
        float room_gain=sf_room_level(in,room);
        wet[0]+=room[0]*room_gain;wet[1]+=room[1]*room_gain;
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
