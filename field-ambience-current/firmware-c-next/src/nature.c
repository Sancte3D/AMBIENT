#include "nature.h"
#include "dsp.h"
#include "world_grammar.h"
#include <stdatomic.h>
#include <string.h>
#include <stdbool.h>
static _Atomic uint32_t requested_seed;
static uint32_t current_seed;
static bool idle;
#define SR ((float)DSP_SAMPLE_RATE_HZ)
static uint32_t wnd_rng_L, wnd_rng_R, wnd_weather;
static dsp_svf_t wnd_lpL, wnd_lpR;
static float wnd_pink_L_b0, wnd_pink_L_b1, wnd_pink_L_b2;
static float wnd_pink_R_b0, wnd_pink_R_b1, wnd_pink_R_b2;
static float wnd_gust_env, wnd_gust_tgt, wnd_slew;
static float wnd_eddy, wnd_eddy_tgt, wnd_dcL, wnd_dcR;
static int wnd_gust_until, wnd_eddy_until;
static uint32_t wnd_ctrl;

static inline float wnd_white(uint32_t *r) {
    *r = (*r) * 1664525u + 1013904223u;
    return (float)((int32_t)*r) * (1.0f / 2147483648.0f);
}
static inline float wnd_random(void) { return 0.5f + 0.5f * wnd_white(&wnd_weather); }
static inline float wnd_pink(uint32_t *rng, float *b0, float *b1, float *b2) {
    float w = wnd_white(rng);
    *b0 = 0.99765f * (*b0) + w * 0.0990460f;
    *b1 = 0.96300f * (*b1) + w * 0.2965164f;
    *b2 = 0.57000f * (*b2) + w * 1.0526913f;
    return (*b0 + *b1 + *b2 + w * 0.1848f) * 0.18f;
}
static void wind_reset(void) {
    wnd_rng_L=0xACE12345u^current_seed;
    wnd_rng_R=0x7B19F88Au^((current_seed<<13)|(current_seed>>19));
    wnd_weather=0x91BC24E3u^current_seed;
    dsp_svf_reset(&wnd_lpL); dsp_svf_reset(&wnd_lpR);
    wnd_gust_tgt=.05f+.70f*wnd_random(); wnd_gust_env=.35f*wnd_gust_tgt;
    wnd_slew=1.0f/(SR*(1.0f+2.0f*wnd_random()));
    wnd_gust_until=(int)(SR*(2.0f+7.0f*wnd_random())); wnd_eddy_until=0; wnd_ctrl=0;
    wnd_eddy=wnd_eddy_tgt=wnd_dcL=wnd_dcR=0.0f;
    wnd_pink_L_b0=wnd_pink_L_b1=wnd_pink_L_b2=0.0f;
    wnd_pink_R_b0=wnd_pink_R_b1=wnd_pink_R_b2=0.0f;
}
static inline void wind_tick(float *outL, float *outR) {
    if (--wnd_gust_until<=0) {
        float r=wnd_random();
        wnd_gust_tgt = r<0.40f ? 0.02f+r*0.20f : 0.30f+wnd_random()*0.65f;
        /* Wide irregular intervals, including long calms; random rise/fall. */
        wnd_gust_until=(int)(SR*(2.0f+wnd_random()*15.0f));
        wnd_slew=1.0f/(SR*(wnd_gust_tgt>wnd_gust_env ?
                         0.6f+wnd_random()*2.4f : 1.2f+wnd_random()*3.0f));
    }
    if (--wnd_eddy_until<=0) {
        wnd_eddy_tgt=wnd_random()*2.0f-1.0f;
        wnd_eddy_until=(int)(SR*(0.18f+wnd_random()*1.9f));
    }
    wnd_gust_env+=wnd_slew*(wnd_gust_tgt-wnd_gust_env);
    wnd_eddy+=0.00012f*(wnd_eddy_tgt-wnd_eddy);
    float gust=wnd_gust_env*wnd_gust_env*(0.85f+0.15f*wnd_eddy);
    if ((wnd_ctrl++ & 63u)==0) {
        float fc=650.0f+wnd_gust_env*1900.0f+wnd_eddy*180.0f;
        dsp_svf_set(&wnd_lpL,fc,0.707f);
        dsp_svf_set(&wnd_lpR,fc*1.08f,0.707f);
    }
    float pL=wnd_pink(&wnd_rng_L,&wnd_pink_L_b0,&wnd_pink_L_b1,&wnd_pink_L_b2);
    float pR=wnd_pink(&wnd_rng_R,&wnd_pink_R_b0,&wnd_pink_R_b1,&wnd_pink_R_b2);
    float L=dsp_svf_lp(&wnd_lpL,pL), R=dsp_svf_lp(&wnd_lpR,pR);
    /* Remove infra/low rumble without a resonant bandpass centre. */
    wnd_dcL+=0.009f*(L-wnd_dcL); wnd_dcR+=0.009f*(R-wnd_dcR);
    *outL+=(L-wnd_dcL)*gust*0.70f;
    *outR+=(R-wnd_dcR)*gust*0.70f;
}


/* Optional geographic events. No sea hum, vinyl, heat haze, brown drone,
 * pitched drips or compulsory wind. Amount defaults to zero in every World. */
static _Atomic uint32_t amount_bits;
static _Atomic int requested_world;
static int current_world,wave_phase;
static float amount_cur,transition,env,env_target,rise;
static uint32_t events_rng,drop_clock;
static int clock_left;
static dsp_pink_t pink_l,pink_r;
static dsp_svf_t water_l,water_r;
static float dc_l,dc_r,drop_env[8];
static int drop_left[8];
static uint32_t drop_rng[8];
static uint32_t rnd(void) { events_rng=events_rng*1664525u+1013904223u; return events_rng>>8; }
static float seconds(float lo,float range) { return lo+range*(float)rnd()/16777216.0f; }
static void reset_events(int world,uint32_t seed) {
    current_world=world; current_seed=seed;
    events_rng=0xC011A57u^(uint32_t)(world*7919)^seed;
    wind_reset(); env=env_target=0; rise=1.0f/(SR*2); wave_phase=0;
    clock_left=(int)(SR*seconds(1,4)); dc_l=dc_r=0; drop_clock=0;
    dsp_pink_seed(&pink_l,0x56ED12u^seed); dsp_pink_seed(&pink_r,0x921CEu^(seed*0x9e3779b9u));
    dsp_svf_reset(&water_l); dsp_svf_reset(&water_r);
    dsp_svf_set(&water_l,world==WORLD_COAST ? 600.0f : 1400.0f,.707f);
    dsp_svf_set(&water_r,world==WORLD_COAST ? 650.0f : 1500.0f,.707f);
    memset(drop_env,0,sizeof drop_env); memset(drop_left,0,sizeof drop_left);
    for(int i=0;i<8;++i) drop_rng[i]=rnd()+1;
}
void nature_set_amount(float v) {
    if(!isfinite(v)) return;
    v=dsp_clampf(v,0,1); uint32_t bits; memcpy(&bits,&v,4);
    atomic_store_explicit(&amount_bits,bits,memory_order_release);
}
void nature_set_world(int world) {
    if(world>=0 && world<CORE_WORLD_COUNT) atomic_store_explicit(&requested_world,world,memory_order_release);
}
void nature_set_seed(uint32_t seed) {
    atomic_store_explicit(&requested_seed,seed ? seed : 0xA6B13E7Du,memory_order_release);
}
void nature_clear(void) { reset_events(current_world,current_seed); amount_cur=transition=0; idle=true; }
void nature_init(void) {
    nature_set_amount(0); nature_set_world(WORLD_COAST); nature_set_seed(0xA6B13E7Du);
    current_world=WORLD_COAST; current_seed=0xA6B13E7Du; nature_clear();
}
void nature_render(float *l,float *r,int frames) {
    uint32_t bits=atomic_load_explicit(&amount_bits,memory_order_acquire);
    float target; memcpy(&target,&bits,4);
    int wanted=atomic_load_explicit(&requested_world,memory_order_acquire);
    uint32_t wanted_seed=atomic_load_explicit(&requested_seed,memory_order_acquire);
    if(idle && target==0) {
        if(wanted!=current_world || wanted_seed!=current_seed) reset_events(wanted,wanted_seed);
        return; /* No noise/filter/weather work while the optional path is cold. */
    }
    if(idle) { reset_events(wanted,wanted_seed); transition=0; idle=false; }
    for(int n=0;n<frames;++n) {
        if(target==0 && amount_cur<1e-6f) { amount_cur=transition=0; idle=true; continue; }
        float t=(wanted==current_world && wanted_seed==current_seed) ? 1 : 0;
        float step=1.0f/(SR*(t>0 ? 2 : .080f));
        if(transition<t) transition=fminf(t,transition+step);
        else if(transition>t) transition=fmaxf(t,transition-step);
        if(transition==0 && (wanted!=current_world || wanted_seed!=current_seed)) reset_events(wanted,wanted_seed);
        amount_cur+=(target-amount_cur)*(1.0f/(SR*2));
        float L=0,R=0; wind_tick(&L,&R); L*=.035f; R*=.035f;
        if(current_world==WORLD_COAST) {
            if(--clock_left<=0) {
                if(wave_phase==0) { /* rounded rise */
                    wave_phase=1; env_target=seconds(.35f,.5f);
                    rise=1.0f/(SR*seconds(1.2f,2)); clock_left=(int)(SR*seconds(2,2));
                } else if(wave_phase==1) { /* longer fall */
                    wave_phase=2; env_target=0;
                    rise=1.0f/(SR*seconds(1,1)); clock_left=(int)(SR*seconds(5,4));
                } else { /* distinct, irregular quiet gap */
                    wave_phase=0; env_target=0; clock_left=(int)(SR*seconds(1,6));
                }
            }
            env+=(env_target-env)*rise;
            float a=dsp_svf_lp(&water_l,dsp_pink(&pink_l));
            float c=dsp_svf_lp(&water_r,dsp_pink(&pink_r));
            dc_l+=.014f*(a-dc_l); dc_r+=.014f*(c-dc_r);
            L+=(a-dc_l)*env*.09f; R+=(c-dc_r)*env*.09f;
        } else if(current_world==WORLD_WOODLAND) {
            if((drop_clock++&63u)==0 && rnd()%1000<11)
                for(int i=0;i<8;++i) if(!drop_left[i]) {
                    drop_left[i]=(int)(SR*.037f); drop_env[i]=seconds(.2f,.6f); break;
                }
            float dl=0,dr=0;
            for(int i=0;i<8;++i) if(drop_left[i]) {
                float age=.037f-(float)drop_left[i]/SR;
                float a=age<.002f ? age/.002f : (float)drop_left[i]/(SR*.035f);
                float noise=wnd_white(&drop_rng[i])*drop_env[i]*a*a;
                dl+=noise*(i&1 ? .35f : .65f); dr+=noise*(i&1 ? .65f : .35f); --drop_left[i];
            }
            L+=dsp_svf_lp(&water_l,dl)*.045f; R+=dsp_svf_lp(&water_r,dr)*.045f;
        }
        l[n]+=L*amount_cur*transition; r[n]+=R*amount_cur*transition;
    }
}
