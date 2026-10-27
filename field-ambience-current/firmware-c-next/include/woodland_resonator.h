#ifndef FAM_WOODLAND_RESONATOR_H
#define FAM_WOODLAND_RESONATOR_H
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "dsp.h"

/* Model 1: an initially displaced, freely decaying harmonic modal body.
 * Independent float design inspired by DRUID's modal principle. No Tonitor
 * code, coefficient tables, inharmonic stiffness or sympathetic pitches.
 * Each damped rotation is a resonator, excited once at note preparation.
 * No continuing oscillator drive, delay, random audio or re-excitation.
 * Main prepares; Audio alone advances the published states. The enclosing
 * voice supplies its existing attack/release/color/ownership and tail rules.
 */
#ifdef FAM_SOUND_FINISH_CANDIDATE
#define WOODLAND_RESONATOR_MODEL 2
#else
#define WOODLAND_RESONATOR_MODEL 1
#endif
#define WOODLAND_RESONATOR_MODES 8
typedef struct {
    float re[WOODLAND_RESONATOR_MODES],im[WOODLAND_RESONATOR_MODES];
    float a[WOODLAND_RESONATOR_MODES],b[WOODLAND_RESONATOR_MODES];
    uint8_t modes;
} woodland_resonator_t;

static inline void woodland_resonator_prepare(woodland_resonator_t *r,
                                             float hz,float amp,float position,
                                             float release_scale) {
    memset(r,0,sizeof *r);
    if(!isfinite(hz) || hz<=0 || hz>=.45f*DSP_SAMPLE_RATE_HZ ||
       !isfinite(amp) || amp<=0 || !isfinite(release_scale) || release_scale<=0 ||
       !isfinite(position)) return;
    amp=dsp_clampf(amp,0,1);position=dsp_clampf(position,.20f,.28f);
    float sum=0;
    for(int m=0;m<WOODLAND_RESONATOR_MODES;++m) {
        float harmonic=(float)(m+1),f=hz*harmonic;
        if(f>=.45f*DSP_SAMPLE_RATE_HZ) break; /* omit; never retune */
        /* Rounded displacement, followed by progressively darker ringing.
         * Fundamental nominal T60=36s at neutral, captured Release 9..144s.
         * Complex states avoid low-frequency cancellation in a direct-form
         * two-pole filter. Coefficients use double at preparation only. */
        double w=6.2831853071795864769*(double)f/DSP_SAMPLE_RATE_HZ;
#ifdef FAM_SOUND_FINISH_CANDIDATE
        /* A voiced wooden body, rather than a triangle losing every upper
         * mode at once. The fourth/sixth form a warm struck shoulder; upper
         * loss is still faster than the unchanged 36s fundamental. */
        static const float seconds[8]={36,30,18,26,16,12,10,8};
        static const float weight[8]={.40f,.025f,.08f,.28f,.07f,.14f,.015f,.06f};
        double t60=(double)seconds[m]*release_scale;
#else
        double t60=36.0*(double)release_scale/(1.0+.28*m*m);
#endif
        double radius=exp(-6.907755278982137/(t60*DSP_SAMPLE_RATE_HZ));
        r->a[m]=(float)(radius*cos(w));r->b[m]=(float)(radius*sin(w));
#ifdef FAM_SOUND_FINISH_CANDIDATE
        /* Small position-dependent articulation; it cannot erase the body.
         * Excite once, never sustain/restrike the modes during render. */
        r->re[m]=weight[m]*(1+.15f*sinf(3.141592654f*position*harmonic));
#else
        r->re[m]=fabsf(sinf(3.141592654f*position*harmonic))/(harmonic*harmonic);
#endif
        sum+=r->re[m];++r->modes;
    }
    /* Triangle inequality bounds the initial sum by the admitted amplitude.
     * Output peak is measured through the actual envelope/color/Room path. */
    float gain=sum>0 ? amp/sum : 0;
    for(int m=0;m<r->modes;++m) r->re[m]*=gain;
}
static inline float woodland_resonator_step(woodland_resonator_t *r) {
    float out=0;
    for(int m=0;m<r->modes;++m) {
        float re=r->re[m],im=r->im[m];
        r->re[m]=r->a[m]*re-r->b[m]*im;
        r->im[m]=r->b[m]*re+r->a[m]*im;
        out+=r->im[m];
    }
    return out;
}
#endif
