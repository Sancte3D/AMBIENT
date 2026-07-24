/*
 * guembri.c — plucked low-lute voice (Desert). See guembri.h.
 *
 * r19.57 rebuild. The first version measured 33 % of its energy BELOW 40 Hz
 * (inaudible cone-flap) while the bridge buzz — the instrument's whole identity
 * — sat at 0.8 %. Result: an undefined low thud with no recognisable note. The
 * fix follows how the real instrument actually makes its sound:
 *
 *   string : two lightly detuned band-limited saws. NO sub-octave — the
 *            fundamental is already low; a sub only adds subsonic mud.
 *   body   : resonant SVF lowpass, cutoff decays bright→warm but stays well
 *            ABOVE the fundamental so the harmonics (= the pitch you hear)
 *            survive; moderate Q so the sweep doesn't "wow".
 *   buzz   : the sintir's metal-ring rattle. COUPLED to the string: the
 *            rectified string signal drives a high resonant bandpass, so the
 *            rattle is excited by the note and rides its envelope instead of
 *            being an unrelated noise burst. This is the identity of the voice.
 *   attack : 4 ms ramp (no discontinuity), then exponential decay — dry pluck.
 *
 * Control-rate coeff updates every CTL samples; per-sample stays two saws +
 * one LP + one BP + adds. Alias-free, no per-sample transcendental.
 */
#include "guembri.h"
#include "shape.h"
#include "dsp.h"
#include <string.h>

#define SR   ((float)DSP_SAMPLE_RATE_HZ)
#define CTL  32
#define VMAX 3

typedef struct {
    int      active;
    float    freq, amp;
    float    ph1, ph2, inc1, inc2;
    dsp_svf_t body, buzzbp;
    float    env, envCoef;                 /* pluck amp decay           */
    float    fenv, fenvCoef;               /* brightness decay          */
    float    atk, atkInc;                  /* 4 ms attack ramp          */
    float    buzz_env, buzz_coef;          /* rattle intensity decay    */
    float    body_base;
    float    panL, panR;
} gvoice_t;

static gvoice_t V[VMAX];
static int      ctl;

/* per-sample exponential coefficient reaching -60 dB in t seconds */
static inline float decay_coef(float t_s) {
    float c = 1.0f - 6.9f / (t_s * SR);
    return c < 0.0f ? 0.0f : (c > 0.99999f ? 0.99999f : c);
}

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
    v->inc2=freq_hz*1.0035f/SR;            /* slight detune = body      */
    v->ph1=0.02f; v->ph2=0.5f;

    /* Body stays ABOVE the fundamental at all times: bright ~×11 on the
     * attack, settling to ~×3.5 — the harmonics that carry the PITCH survive
     * (the old ×2.2 floor killed them and left a thud). */
    v->body_base=freq_hz*3.5f;
    dsp_svf_reset(&v->body);   dsp_svf_set(&v->body, freq_hz*11.0f, 1.8f);
    /* rattle sits in the presence band where a metal ring actually rings */
    dsp_svf_reset(&v->buzzbp); dsp_svf_set(&v->buzzbp, 2600.0f, 6.0f);

    v->env=1.0f;      v->envCoef =decay_coef(1.6f*shape_release_scale()); /* r19.61 */
    v->fenv=1.0f;     v->fenvCoef=decay_coef(0.45f);  /* brightness closes */
    v->buzz_env=1.0f; v->buzz_coef=decay_coef(0.70f); /* rattle rings on   */
    v->atk=0.0f;      v->atkInc=1.0f/(0.004f*shape_attack_scale()*SR);    /* r19.61 */

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
                /* bright pluck → warm sustain, never below body_base */
                float cut=v->body_base*(1.0f+2.2f*v->fenv);
                dsp_svf_set(&v->body, dsp_clampf(cut,120.0f,SR*0.45f), 1.8f);
            }

            /* string */
            float s=dsp_poly_saw(v->ph1,v->inc1)*0.6f + dsp_poly_saw(v->ph2,v->inc2)*0.4f;
            v->ph1+=v->inc1; if(v->ph1>=1.0f)v->ph1-=1.0f;
            v->ph2+=v->inc2; if(v->ph2>=1.0f)v->ph2-=1.0f;

            float body=dsp_svf_lp(&v->body, s);

            /* BRIDGE BUZZ — the sintir identity. The rattle is EXCITED BY the
             * string: rectified string drive into a high resonant bandpass, so
             * it is locked to the note (pitch-coupled rasp) rather than an
             * unrelated hiss. Rides its own decay so it fades with the pluck. */
            float drive = s >= 0.0f ? s : -s;          /* rectified string     */
            float rattle = dsp_svf_bp(&v->buzzbp, drive - 0.5f);
            body += rattle * v->buzz_env * 0.55f;

            /* envelopes */
            if (v->atk < 1.0f) { v->atk += v->atkInc; if (v->atk > 1.0f) v->atk = 1.0f; }
            v->env      *= v->envCoef;
            v->fenv     *= v->fenvCoef;
            v->buzz_env *= v->buzz_coef;
            if(v->env < 1.0e-4f){ v->active=0; }

            float out=body*v->env*v->atk*v->amp*0.55f;
            L+=out*v->panL; R+=out*v->panR;
        }
        if(++ctl>=CTL) ctl=0;
        dry_L[n]+=L; dry_R[n]+=R;
        send_L[n]+=L*send_amount; send_R[n]+=R*send_amount;
    }
}
