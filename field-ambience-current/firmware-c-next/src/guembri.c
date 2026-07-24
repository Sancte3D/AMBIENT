/*
 * guembri.c — plucked low-lute voice (Desert). See guembri.h.
 *
 * Signal per voice:
 *   string : two lightly detuned band-limited saws + a sub sine = the low body
 *   body   : resonant SVF lowpass whose cutoff DECAYS with a filter envelope
 *            (bright pluck attack → warm dark sustain), forced low (Desert)
 *   buzz   : a short attack-only rattle — bandpassed noise + a fast-decaying
 *            high resonance = the sintir bridge buzz
 *   amp    : pluck envelope — near-instant attack, exponential decay (dry)
 *
 * Control-rate coeff updates every CTL samples; per-sample stays two saws + one
 * sine + one LP + one BP + adds. Alias-free, no per-sample transcendental.
 */
#include "guembri.h"
#include "dsp.h"
#include <string.h>

#define SR   ((float)DSP_SAMPLE_RATE_HZ)
#define CTL  32
#define VMAX 3

typedef struct {
    int      active;
    float    freq, amp;
    float    ph1, ph2, inc1, inc2, subPh, subInc;
    dsp_svf_t body, buzzbp;
    uint32_t rng;
    float    env, envCoef;                 /* pluck amp decay        */
    float    fenv, fenvCoef;               /* filter (brightness) decay */
    int      buzz_left;                    /* samples of attack buzz */
    float    body_base;
    float    panL, panR;
} gvoice_t;

static gvoice_t V[VMAX];
static int      ctl;

static inline float wnoise(uint32_t *r){ *r=(*r)*1664525u+1013904223u; return (float)((int32_t)*r)*(1.0f/2147483648.0f); }

void guembri_init(void){ memset(V,0,sizeof V); ctl=0; }

static int alloc_voice(void){
    int best=0; float lo=1e9f;
    for(int i=0;i<VMAX;++i){ if(!V[i].active) return i; if(V[i].env<lo){lo=V[i].env;best=i;} }
    return best;
}

void guembri_note(float freq_hz, float amp){
    if (freq_hz < 20.0f) return;
    int i=alloc_voice(); gvoice_t *v=&V[i];
    v->freq=freq_hz; v->amp=dsp_clampf(amp,0.0f,1.0f);
    v->inc1=freq_hz/SR;
    v->inc2=freq_hz*1.003f/SR;             /* slight detune body */
    v->subInc=freq_hz*0.5f/SR;
    v->ph1=0.02f; v->ph2=0.5f; v->subPh=0.0f;
    v->rng=0x6C078965u ^ (uint32_t)(freq_hz*53.0f);
    v->body_base=freq_hz*2.2f;
    dsp_svf_reset(&v->body);   dsp_svf_set(&v->body, freq_hz*9.0f, 3.2f);   /* opens bright */
    dsp_svf_reset(&v->buzzbp); dsp_svf_set(&v->buzzbp, 2000.0f, 3.0f);
    v->env=1.0f;
    v->envCoef=dsp_smooth_coef(0.9f);       /* ~1.4 s dry decay via 1-pole below */
    /* exponential pluck decay: aim ~1.3 s to -60 dB */
    v->envCoef=1.0f - 1.0f/(1.3f*SR)*6.9f;  /* ln(1000)≈6.9 */
    if (v->envCoef < 0.9f) v->envCoef = 0.9f;
    v->fenv=1.0f;
    v->fenvCoef=1.0f - 1.0f/(0.35f*SR)*6.9f;/* filter closes in ~0.35 s */
    if (v->fenvCoef < 0.9f) v->fenvCoef = 0.9f;
    v->buzz_left=(int)(0.18f*SR);
    float pan=(i==0)?-0.15f:(i==1)?0.15f:0.0f;
    v->panL=0.5f*(1.0f-pan); v->panR=0.5f*(1.0f+pan);
    v->active=1;
}

int guembri_active_count(void){ int c=0; for(int i=0;i<VMAX;++i) c+=V[i].active?1:0; return c; }

void guembri_render_mix(float *dry_L,float *dry_R,float *send_L,float *send_R,int frames,float send_amount){
    for(int n=0;n<frames;++n){
        float L=0.0f,R=0.0f; int do_ctl=(ctl==0);
        for(int i=0;i<VMAX;++i){
            gvoice_t *v=&V[i];
            if(!v->active) continue;

            if(do_ctl){
                float cut=v->body_base*(1.0f+6.0f*v->fenv);   /* bright→dark */
                dsp_svf_set(&v->body, dsp_clampf(cut,90.0f,SR*0.45f), 3.2f);
            }
            /* string */
            float s=dsp_poly_saw(v->ph1,v->inc1)*0.55f + dsp_poly_saw(v->ph2,v->inc2)*0.35f;
            float sub=dsp_sin(v->subPh)*0.5f;
            v->ph1+=v->inc1; if(v->ph1>=1.0f)v->ph1-=1.0f;
            v->ph2+=v->inc2; if(v->ph2>=1.0f)v->ph2-=1.0f;
            v->subPh+=v->subInc; if(v->subPh>=1.0f)v->subPh-=1.0f;

            float body=dsp_svf_lp(&v->body, s+sub);

            /* sintir bridge buzz — attack only */
            if(v->buzz_left>0){
                --v->buzz_left;
                float a=(float)v->buzz_left/(0.18f*SR);
                body += dsp_svf_bp(&v->buzzbp, wnoise(&v->rng)) * a*a * 0.35f;
            }

            /* envelopes (per-sample 1-pole exp decays) */
            v->env  *= v->envCoef;
            v->fenv *= v->fenvCoef;
            if(v->env < 1.0e-4f){ v->active=0; }

            float out=body*v->env*v->amp*0.6f;
            L+=out*v->panL; R+=out*v->panR;
        }
        if(++ctl>=CTL) ctl=0;
        dry_L[n]+=L; dry_R[n]+=R;
        send_L[n]+=L*send_amount; send_R[n]+=R*send_amount;
    }
}
