/*
 * render_drone_drift_ab.c — proof-of-concept for Tier A #3 from
 * docs/audio/ENGINE_UPGRADE_ANALYSIS.md: give the drone a slow pitch drift
 * + breath tremolo so it sounds alive instead of "frozen oscillator".
 *
 * The current drone.c plays a sustained pad voice at a fixed pitch — no
 * micro-detune walk, no breath. That makes the bottom of the mix sit very
 * still. Ambient drones across the canon (Eno's Music for Airports, Stars
 * of the Lid, JPverb residue tails) all share one trick: the fundamental
 * never holds still. ±2 cent random walk on a ~30 s timescale + ~0.04 Hz
 * tremolo on the amplitude is what gives the drone "breath".
 *
 * This tool fakes the upgrade without touching drone.c: it drives the pad
 * directly at the drone's frequency with two parallel voices whose pitches
 * are slowly walked, and renders 30 s WITHOUT the drift then 30 s WITH it,
 * over the LIQUID reverb. So we can hear the actual difference before
 * deciding whether to commit the change into drone.c proper.
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_drone_drift_ab.c \
 *      src/dsp.c src/pad.c src/texture.c src/reverb.c -lm \
 *      -o /tmp/render_drone_drift_ab
 *   /tmp/render_drone_drift_ab /tmp/drone_drift_ab.wav
 */

#include "pad.h"
#include "texture.h"
#include "reverb.h"
#include "dsp.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SR        44100
#define BLOCK     256
#define SECT_SECS 30

/* ---- master colour shared by both sections so the only variable is drift ---- */
static uint32_t hr = 0xC0FFEE11u;
static inline float hn(void){ hr = hr*1664525u + 1013904223u; return ((int32_t)hr)*(1.0f/2147483648.0f); }
static inline void hiss_block(float *L,float *R,int n,float amp){ for(int i=0;i<n;++i){L[i]+=hn()*amp; R[i]+=hn()*amp;} }

/* ---- WAV header ---- */
static void wav_header(FILE *f,uint32_t nf){
    uint32_t data=nf*4u,v; uint16_t h;
    fwrite("RIFF",1,4,f); v=36+data;fwrite(&v,4,1,f); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); v=16;fwrite(&v,4,1,f); h=1;fwrite(&h,2,1,f); h=2;fwrite(&h,2,1,f);
    v=SR;fwrite(&v,4,1,f); v=SR*4u;fwrite(&v,4,1,f); h=4;fwrite(&h,2,1,f); h=16;fwrite(&h,2,1,f);
    fwrite("data",1,4,f); fwrite(&data,4,1,f);
}

/* ---- "drone-drift" parameters (the proposed Tier A #3 values) ---- */
#define DRIFT_CENTS    2.0f          /* ± peak pitch-walk in cents */
#define DRIFT_TAU_S    18.0f         /* random-walk time constant ~ slow */
#define BREATH_HZ      0.04f         /* tremolo rate */
#define BREATH_DB      0.5f          /* ± peak tremolo, dB */

static float drift_state_L = 0.0f, drift_state_R = 0.0f;
static float breath_phase  = 0.0f;
static uint32_t drift_rng  = 0xBADBADu;
static inline float drift_white(void){
    drift_rng = drift_rng*1664525u + 1013904223u;
    return ((int32_t)drift_rng) * (1.0f/2147483648.0f);
}

/* one-pole low-pass random walk: walks freely but never runs away */
static inline void drift_tick(float *state, float dt){
    float target = drift_white() * DRIFT_CENTS;        /* white in [-cents, +cents] */
    float a      = dt / (DRIFT_TAU_S + dt);            /* slow approach */
    *state += a * (target - *state);
}

static void render_half(int with_drift, FILE *f, int *peak, double *sumsq, long *sumN){
    pad_init();
    texture_init(); texture_set_amount(0.12f);
    reverb_init(); reverb_set(0.60f, 0.40f); reverb_set_drive(0.08f);

    /* drone: A1 root + E2 fifth, two pad voices held the whole section */
    const float base_root_hz = dsp_midi_to_hz(33.0f);   /* A1 */
    const float base_5th_hz  = dsp_midi_to_hz(40.0f);   /* E2 */
    pad_note_on(10, base_root_hz, 0.13f);
    pad_note_on(11, base_5th_hz,  0.10f);

    drift_state_L = drift_state_R = 0.0f;
    breath_phase  = 0.0f;
    /* control-rate tick = block; treat each block as one time step */
    const float dt_block = (float)BLOCK / (float)SR;

    float dryL[BLOCK],dryR[BLOCK],sndL[BLOCK],sndR[BLOCK],wL[BLOCK],wR[BLOCK];
    int16_t out16[BLOCK*2];
    uint32_t total = (uint32_t)SECT_SECS*SR, frame=0;
    const float WET=0.42f, HISS=0.005f, SAT=1.10f;

    while (frame < total){
        if (with_drift){
            drift_tick(&drift_state_L, dt_block);
            drift_tick(&drift_state_R, dt_block);
            breath_phase += BREATH_HZ * dt_block;
            if (breath_phase >= 1.0f) breath_phase -= 1.0f;
            /* re-tune the two held voices: cents -> ratio = 2^(c/1200) */
            float root = base_root_hz * powf(2.0f, drift_state_L / 1200.0f);
            float fifth= base_5th_hz  * powf(2.0f, drift_state_R / 1200.0f);
            /* breath tremolo on amp (±BREATH_DB → linear gain) */
            float trem_db = BREATH_DB * sinf(breath_phase * 6.2831853f);
            float gain    = powf(10.0f, trem_db / 20.0f);
            /* pad_note_on with the same source IDs is a re-trigger that glides
             * cleanly (keep_phase path inside pad.c) — exactly the "drift"
             * behaviour the upgrade would make permanent in drone.c. */
            pad_note_on(10, root,  0.13f * gain);
            pad_note_on(11, fifth, 0.10f * gain);
        }

        int n = (int)((total-frame) < BLOCK ? (total-frame) : BLOCK);
        memset(dryL,0,sizeof(float)*n); memset(dryR,0,sizeof(float)*n);
        memset(sndL,0,sizeof(float)*n); memset(sndR,0,sizeof(float)*n);
        pad_render_mix(dryL, dryR, sndL, sndR, n, 0.75f);
        texture_render_mix(dryL, dryR, sndL, sndR, n, 0.55f);
        hiss_block(dryL, dryR, n, HISS);
        reverb_render(sndL, sndR, wL, wR, n);

        for (int i=0;i<n;++i){
            float L = (dryL[i] + wL[i]*WET) * 0.95f;
            float Rr= (dryR[i] + wR[i]*WET) * 0.95f;
            L = tanhf(L*SAT)*0.78f; Rr = tanhf(Rr*SAT)*0.78f;
            int li=(int)(L*32767.0f), ri=(int)(Rr*32767.0f);
            if(li>32767)li=32767; if(li<-32768)li=-32768;
            if(ri>32767)ri=32767; if(ri<-32768)ri=-32768;
            out16[i*2]=(int16_t)li; out16[i*2+1]=(int16_t)ri;
            int a=li<0?-li:li; if(a>*peak)*peak=a; double s=li/32768.0; *sumsq+=s*s; ++*sumN;
        }
        fwrite(out16, sizeof(int16_t), (size_t)n*2, f);
        frame += (uint32_t)n;
    }
    pad_all_off();
}

int main(int argc, char **argv){
    const char *out = (argc>1) ? argv[1] : "/tmp/drone_drift_ab.wav";
    dsp_init();
    FILE *f = fopen(out,"wb"); if(!f){fprintf(stderr,"open %s\n",out); return 1;}
    wav_header(f, (uint32_t)SECT_SECS*SR*2);
    int peak = 0; double sumsq = 0; long sumN = 0;
    render_half(0, f, &peak, &sumsq, &sumN);   /* A: drone static (current)  */
    render_half(1, f, &peak, &sumsq, &sumN);   /* B: drone with drift+breath */
    fclose(f);
    printf("drone drift A/B : %s\n  peak %d (%.1f dBFS), RMS %.4f (%.1f dBFS)\n",
           out, peak, 20.0*log10((double)(peak?peak:1)/32767.0),
           sqrt(sumsq/(double)sumN), 20.0*log10(sqrt(sumsq/(double)sumN)+1e-12));
    return 0;
}
