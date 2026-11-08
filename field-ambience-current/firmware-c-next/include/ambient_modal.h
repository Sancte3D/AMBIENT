#ifndef FAM_AMBIENT_MODAL_H
#define FAM_AMBIENT_MODAL_H
/* Ambient voicing: struck instruments as modal bodies. Each mode is a damped
 * rotation, excited once when Main prepares the note: no oscillator drive,
 * delay line, noise or per-sample transcendental. Audio only advances the
 * states. Every mode starts at zero output (sine phase), so an onset never
 * steps; the initial mode sum is normalised to the admitted amplitude.
 *   Kalimba: a metal tine on a wooden box. Fundamental, a weak octave from
 *            the tine's nonlinearity, the first bending mode near 5.6x that
 *            gives the bright "ping", a short sparkle near 15x and a 12 ms
 *            thumb tick at any pitch.
 *   Bell:    a glockenspiel/celesta bar. The free-bar modes 2.756x, 5.404x
 *            and 8.933x give the bell; a soft octave adds celesta warmth; the
 *            fundamental is a slightly split pair that beats about once every
 *            two seconds, the shimmer of a real bell. */
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "dsp.h"

#define AMBIENT_MODAL_MODES 8
typedef struct {
    float re[AMBIENT_MODAL_MODES],im[AMBIENT_MODAL_MODES];
    float a[AMBIENT_MODAL_MODES],b[AMBIENT_MODAL_MODES];
    uint8_t modes;
} ambient_modal_t;

/* Add one mode (Main, at preparation): T60 in seconds, initial weight. Modes
 * at or above .45 SR are omitted, never retuned. */
static inline void ambient_modal_add(ambient_modal_t *r,float hz,float t60,float weight) {
    if(r->modes>=AMBIENT_MODAL_MODES || !(hz>0) || hz>=.45f*DSP_SAMPLE_RATE_HZ ||
       !(t60>0) || !(weight>0)) return;
    double w=6.2831853071795864769*(double)hz/DSP_SAMPLE_RATE_HZ;
    double radius=exp(-6.907755278982137/((double)t60*DSP_SAMPLE_RATE_HZ));
    int m=r->modes++;
    r->a[m]=(float)(radius*cos(w)); r->b[m]=(float)(radius*sin(w));
    r->re[m]=weight; r->im[m]=0;
}
static inline void ambient_modal_normalise(ambient_modal_t *r,float amp) {
    float sum=0; for(int m=0;m<r->modes;++m) sum+=r->re[m];
    float g=sum>0 ? dsp_clampf(amp,0,1)/sum : 0;
    for(int m=0;m<r->modes;++m) r->re[m]*=g;
}
static inline float ambient_modal_step(ambient_modal_t *r) {
    float out=0;
    for(int m=0;m<r->modes;++m) {
        float re=r->re[m],im=r->im[m];
        r->re[m]=r->a[m]*re-r->b[m]*im;
        r->im[m]=r->b[m]*re+r->a[m]*im;
        out+=r->im[m];
    }
    return out;
}
/* Remaining modal energy, for retiring a voice whose ring has died away. */
static inline float ambient_modal_energy(const ambient_modal_t *r) {
    float e=0; for(int m=0;m<r->modes;++m) e+=r->re[m]*r->re[m]+r->im[m]*r->im[m];
    return e;
}

static inline void ambient_kalimba_prepare(ambient_modal_t *r,float hz,float amp,float bright,float ring) {
    memset(r,0,sizeof *r);
    if(!isfinite(hz) || hz<=0 || !isfinite(amp) || amp<=0 || !isfinite(ring) || ring<=0) return;
    bright=dsp_clampf(bright,0,1);
    float t=dsp_clampf(4.5f*sqrtf(330.0f/hz),1.6f,7.0f)*ring; /* higher tines ring shorter */
    ambient_modal_add(r,hz,t,.70f);
    ambient_modal_add(r,2.0f*hz,.40f*t,.07f);
    ambient_modal_add(r,5.62f*hz,.55f,.20f+.18f*bright);
    ambient_modal_add(r,15.3f*hz,.09f,.03f+.06f*bright);
    ambient_modal_add(r,3150.0f,.012f,.025f+.030f*bright);
    ambient_modal_normalise(r,amp);
}
static inline void ambient_bell_prepare(ambient_modal_t *r,float hz,float amp,float bright,float ring) {
    memset(r,0,sizeof *r);
    if(!isfinite(hz) || hz<=0 || !isfinite(amp) || amp<=0 || !isfinite(ring) || ring<=0) return;
    bright=dsp_clampf(bright,0,1);
    float t=dsp_clampf(7.0f*sqrtf(330.0f/hz),3.0f,10.0f)*ring;
    ambient_modal_add(r,hz*.99935f,t,.38f);
    ambient_modal_add(r,hz*1.00065f,.92f*t,.38f);
    ambient_modal_add(r,2.0f*hz,.45f*t,.10f);
    ambient_modal_add(r,2.756f*hz,.32f*t,.18f+.10f*bright);
    ambient_modal_add(r,5.404f*hz,.12f*t,.06f+.06f*bright);
    ambient_modal_add(r,8.933f*hz,.05f*t,.02f+.03f*bright);
    ambient_modal_add(r,4700.0f,.010f,.015f+.020f*bright);
    ambient_modal_normalise(r,amp);
}
#endif
