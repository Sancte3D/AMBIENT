/*
 * choir.c — damp organ/choir voice (Moss Fields). See choir.h.
 *
 * Signal per voice (two detuned "singers" A/B):
 *   partials : 4 LUT-sine harmonics, drawbar-ish soft levels (organ body)
 *   breath   : white noise → bandpass, low level, follows the envelope (the
 *              choir "hh"); fades as the note settles
 *   formant  : one fixed SVF bandpass ≈ 600 Hz for a vowel colour
 *   damp     : one SVF lowpass ≈ 1.5 kHz — the felt/fog absorption (Moss)
 *   vibrato  : slow, shallow, delayed (a choir wobble, not a fast trill)
 *
 * Control-rate work every CTL samples; per-sample stays 8 LUT sines + noise +
 * one BP + one LP + adds. No per-sample transcendental (dsp_sin is a LUT).
 */
#include "choir.h"
#include "shape.h"
#include "dsp.h"
#include <string.h>

#define SR    ((float)DSP_SAMPLE_RATE_HZ)
#define CTL   32
#define VMAX  3
#define NPART 4

typedef enum { V_IDLE = 0, V_ATTACK, V_HOLD, V_RELEASE } vstage_t;

/* soft, choir-ish partial levels (fundamental strong, quick rolloff) */
static const float PART_LVL[NPART] = { 1.0f, 0.42f, 0.16f, 0.08f };

typedef struct {
    vstage_t stage;
    int source;
    float expression, vibFade;
    float    freq, amp;
    float    phA[NPART], phB[NPART];      /* two detuned singers          */
    float    incA[NPART], incB[NPART];
    dsp_svf_t breathbp, form, damp;
    uint32_t rng;

    float    env, envInc, relCoef;
    int      hold_left;
    float    breath;                       /* breath level 1→low            */

    float    vibPh, vibInc;
    int      vibDelay;
    float    panL, panR;
} cvoice_t;

static cvoice_t V[VMAX], pending[VMAX];
/* Prepare off the audio path; at capacity fade the old voice for 8 ms,
 * then start the prepared attack. Exactly VMAX voices render at any time. */
#define HANDOVER_SAMPLES 353
static volatile int queued[VMAX];
static int fade_left[VMAX];
static int      ctl;

static inline float wnoise(uint32_t *r){ *r=(*r)*1664525u+1013904223u; return (float)((int32_t)*r)*(1.0f/2147483648.0f); }

void choir_init(void){
    memset(pending,0,sizeof pending); memset((void*)queued,0,sizeof queued);
    memset(fade_left,0,sizeof fade_left); memset(V,0,sizeof V); ctl=0; }

static int alloc_voice(int source) {
    int best=0; float lowest=1e9f;
    for(int i=0;i<VMAX;++i) {
        if(queued[i] && pending[i].source==source && source>=0) return i;
        if(V[i].stage==V_IDLE && !queued[i]) return i;
        /* Released voices yield first, then the quietest held voice. */
        float score=V[i].env + (V[i].stage==V_RELEASE ? 0.0f:2.0f);
        if(score<lowest) { lowest=score; best=i; }
    }
    return best;
}

static void prepare_note(cvoice_t *v, int i, int source, float freq_hz, float amp) {
    memset(v,0,sizeof *v);
    v->source=source; v->expression=dsp_clampf(amp/0.62f,0.0f,1.0f); v->vibFade=0.0f;
    v->freq=freq_hz; v->amp=dsp_clampf(amp,0.0f,1.0f);
    for(int k=0;k<NPART;++k){
        float f=freq_hz*(float)(k+1);
        v->incA[k]=f/SR;
        v->incB[k]=f*1.0055f/SR;           /* +9.5 cents singer B (choir spread) */
        if (v->stage==V_IDLE){ v->phA[k]=0.13f*(float)k; v->phB[k]=0.61f*(float)k; }
    }
    v->rng=0x2545F491u ^ (uint32_t)(freq_hz*71.0f);
    dsp_svf_reset(&v->breathbp); dsp_svf_set(&v->breathbp, freq_hz*3.0f+900.0f, 1.0f);
    dsp_svf_reset(&v->form);     dsp_svf_set(&v->form, 600.0f, 1.6f);
    dsp_svf_reset(&v->damp);     dsp_svf_set(&v->damp, 1050.0f+450.0f*v->expression, 0.8f);
    v->env=0.0001f;
    v->envInc=v->amp/(0.40f*shape_attack_scale()*SR);   /* r19.61: 400 ms x SHAPE */
    v->relCoef=dsp_smooth_coef(1.1f*shape_release_scale()); /* r19.61 */
    v->hold_left=(int)(3.0f*SR);
    v->breath=1.0f;
    v->vibPh=0.0f; v->vibInc=4.6f/SR; v->vibDelay=(int)(0.5f*SR);
    float pan=(i==0)?-0.3f:(i==1)?0.3f:0.0f;
    v->panL=0.5f*(1.0f-pan); v->panR=0.5f*(1.0f+pan);
    v->stage=V_ATTACK;
}

static void start_note(int source, float freq_hz, float amp) {
    if (!isfinite(freq_hz) || !isfinite(amp) || freq_hz<20.0f || freq_hz>8000.0f || amp<=0.0f) return;
    if(source>=0) for(int j=0;j<VMAX;++j)
        if(V[j].stage!=V_IDLE && V[j].source==source) V[j].stage=V_RELEASE;
    int i=alloc_voice(source);
    queued[i]=0;
    prepare_note(&pending[i],i,source,freq_hz,amp);
    if(V[i].stage!=V_IDLE && fade_left[i]==0) fade_left[i]=HANDOVER_SAMPLES;
    __asm__ volatile("" ::: "memory");
    queued[i]=1;
}

void choir_note(float freq_hz, float amp) { start_note(-1,freq_hz,amp); }
void choir_note_on(int source,float freq_hz,float amp) {
    if(source>=0 && source<16) start_note(source,freq_hz,amp);
}
void choir_note_off(int source) {
    for(int i=0;i<VMAX;++i) if(queued[i] && pending[i].source==source) queued[i]=0;
    for(int i=0;i<VMAX;++i) if(V[i].stage!=V_IDLE && V[i].source==source) {
        V[i].source=-1; V[i].stage=V_RELEASE;
    }
}
void choir_all_off(void) {
    for(int i=0;i<VMAX;++i) queued[i]=0;
    for(int i=0;i<VMAX;++i) if(V[i].stage!=V_IDLE) { V[i].source=-1; V[i].stage=V_RELEASE; }
}

int choir_active_count(void){ int c=0; for(int i=0;i<VMAX;++i) if(V[i].stage!=V_IDLE || queued[i]) ++c; return c; }

void choir_render_mix(float *dry_L,float *dry_R,float *send_L,float *send_R,int frames,float send_amount){
    for(int n=0;n<frames;++n){
        float L=0.0f,R=0.0f; int do_ctl=(ctl==0);
        for(int i=0;i<VMAX;++i){
            cvoice_t *v=&V[i];
            if(queued[i] && v->stage==V_IDLE) {
                *v=pending[i]; queued[i]=0; fade_left[i]=0;
            }
            if(v->stage==V_IDLE) continue;

            if(do_ctl){
                /* breath fades from full at the onset to a whisper on the hold */
                float bt=(v->stage==V_ATTACK)?1.0f:0.18f;
                v->breath += (bt - v->breath)*0.03f;
            }
            /* amp envelope */
            switch(v->stage){
                case V_ATTACK: v->env+=v->envInc; if(v->env>=v->amp){v->env=v->amp;v->stage=V_HOLD;} break;
                case V_HOLD:   if(v->source < 0 && --v->hold_left<=0) v->stage=V_RELEASE; break;
                case V_RELEASE:v->env-=v->relCoef*v->env; if(v->env<=1.0e-5f){v->env=0.0f;v->stage=V_IDLE;} break;
                default: break;
            }
            if(v->stage==V_IDLE) continue;

            /* delayed shallow vibrato */
            v->vibFade += 0.00009f*((v->vibDelay<=0 ? 1.0f : 0.0f)-v->vibFade);
            float vibAmt=v->vibFade;
            if(v->vibDelay>0){--v->vibDelay; vibAmt=0.0f;}
            float vib=dsp_sin(v->vibPh)*0.0025f*vibAmt;      /* ±~4 cents */
            v->vibPh+=v->vibInc; if(v->vibPh>=1.0f) v->vibPh-=1.0f;

            /* additive sine stack, two detuned singers */
            float s=0.0f;
            for(int k=0;k<NPART;++k){
                s += dsp_sin(v->phA[k])*PART_LVL[k];
                s += dsp_sin(v->phB[k])*PART_LVL[k];
                v->phA[k]+=v->incA[k]*(1.0f+vib); if(v->phA[k]>=1.0f) v->phA[k]-=1.0f;
                v->phB[k]+=v->incB[k]*(1.0f+vib); if(v->phB[k]>=1.0f) v->phB[k]-=1.0f;
            }
            s *= 0.24f;                                       /* stack makeup */

            /* breath grain */
            float bn=dsp_svf_bp(&v->breathbp, wnoise(&v->rng))*(0.05f+0.14f*v->breath);
            /* formant vowel + damp (felt/fog absorption) */
            float body=s + dsp_svf_bp(&v->form, s)*0.3f + bn;
            body=dsp_svf_lp(&v->damp, body);

            float out=body*v->env*0.5f;
            if(fade_left[i]>0) {
                out *= (float)fade_left[i]/HANDOVER_SAMPLES;
                if(--fade_left[i]==0) v->stage=V_IDLE;
            }
            L+=out*v->panL; R+=out*v->panR;
        }
        if(++ctl>=CTL) ctl=0;
        dry_L[n]+=L; dry_R[n]+=R;
        send_L[n]+=L*send_amount; send_R[n]+=R*send_amount;
    }
}
