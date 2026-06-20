/*
 * render_ps2_world.c — one 60 s PS2-era ambient "world": sparse modal piano-
 * like cell-taps over a sustained sub-drone, brown-noise/breath texture as
 * the room, constant tape hiss as the medium, large dark hall as the space,
 * and a SUBTLE bitcrush+drive master colour for the PS2-DAC vibe.
 *
 * Built from the LAURA composition + BLACK's master colouration from the
 * SH2 sketches, but tuned for "welt-bauend und angenehm" instead of either
 * pure-melancholic or aggressive-dirty. No drums, no industrial clangs in
 * the foreground — the only events are the player's notes and one very
 * distant tick around the mid-point (field-recording element, far in the
 * reverb send).
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_ps2_world.c \
 *      src/dsp.c src/pad.c src/texture.c src/reverb.c -lm \
 *      -o /tmp/render_ps2_world
 *   /tmp/render_ps2_world /tmp/ps2_world.wav
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
#define TOTAL_SECS 60

/* tape hiss — decorrelated colored noise, very low constant level */
static uint32_t hr_L = 0xC0FFEE11u, hr_R = 0xDEADBEEFu;
static inline float hn(uint32_t *r){
    *r = (*r)*1664525u + 1013904223u;
    return ((int32_t)*r) * (1.0f / 2147483648.0f);
}
static inline void hiss_block(float *L, float *R, int n, float amp){
    for (int i=0;i<n;++i){ L[i] += hn(&hr_L) * amp; R[i] += hn(&hr_R) * amp; }
}

/* one-shot Karplus-Strong tick — for the single "distant sound in the world" */
#define KS_LEN 1600
static float ks_buf[KS_LEN]; static int ks_w=0, ks_active=0; static float ks_amp=0;
static void ks_trigger(void){
    for (int i=0;i<KS_LEN;++i) ks_buf[i] = hn(&hr_L) * 0.3f;
    ks_w = 0; ks_amp = 0.18f; ks_active = (int)(SR*3.0f);
}
static inline float ks_step(void){
    if (ks_active <= 0) return 0.0f;
    int r = (ks_w+1) % KS_LEN;
    float s = ks_buf[ks_w];
    ks_buf[ks_w] = 0.5f*(ks_buf[ks_w] + ks_buf[r]) * 0.995f;
    ks_w = r; ks_amp *= 0.99996f; --ks_active;
    return tanhf(s * ks_amp * 2.5f) * 0.35f;
}

/* master bitcrush + drive — subtle PS2 DAC colour */
static int   bc_ctr = 0;
static float bc_hL = 0, bc_hR = 0;
static inline void crush(float *L, float *R, int n, int sr_div, int bits, float drv){
    int   levels = (1 << bits);
    float scale  = (float)(levels/2 - 1);
    for (int i=0;i<n;++i){
        if (bc_ctr <= 0){ bc_hL = L[i]; bc_hR = R[i]; bc_ctr = sr_div; }
        --bc_ctr;
        float qL = floorf(bc_hL * scale + 0.5f) / scale;
        float qR = floorf(bc_hR * scale + 0.5f) / scale;
        L[i] = tanhf(qL * drv) * 0.88f;
        R[i] = tanhf(qR * drv) * 0.88f;
    }
}

static void put_u32(FILE *f,uint32_t v){fputc(v&0xff,f);fputc((v>>8)&0xff,f);fputc((v>>16)&0xff,f);fputc((v>>24)&0xff,f);}
static void put_u16(FILE *f,uint16_t v){fputc(v&0xff,f);fputc((v>>8)&0xff,f);}
static void wav_header(FILE *f,uint32_t nf){
    uint32_t data=nf*4u;
    fwrite("RIFF",1,4,f); put_u32(f,36+data); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); put_u32(f,16); put_u16(f,1); put_u16(f,2);
    put_u32(f,SR); put_u32(f,SR*4u); put_u16(f,4); put_u16(f,16);
    fwrite("data",1,4,f); put_u32(f,data);
}

/* D-aeolian (D F G A Bb C D). Sparse melancholic line, descending arc
 * with a return — classic PS2-ambient piano-style. ~9 events in 60 s. */
typedef struct { float t; int midi; float vel; } evt_t;
static const evt_t EV[] = {
    {  1.0f, 62, 0.13f },   /* D4 */
    {  6.5f, 65, 0.11f },   /* F4 */
    { 12.0f, 60, 0.12f },   /* C4 */
    { 18.5f, 57, 0.10f },   /* A3 */
    { 25.0f, 62, 0.11f },   /* D4 */
    { 32.0f, 65, 0.10f },   /* F4 */
    { 38.5f, 60, 0.11f },   /* C4 */
    { 46.0f, 57, 0.09f },   /* A3 */
    { 53.0f, 50, 0.08f },   /* D3 — lowest, resolution */
};
#define N_EV (sizeof EV / sizeof EV[0])

typedef struct { float t; uint8_t src; } pend_t;
static pend_t pend[32]; static int npend = 0;
static void sched_off(float t, uint8_t s){ if (npend<32){ pend[npend].t=t; pend[npend].src=s; npend++; } }

int main(int argc, char **argv){
    const char *out = (argc>1) ? argv[1] : "/tmp/ps2_world.wav";
    dsp_init();
    pad_init(); pad_set_profile(0);                    /* Bed — darkest */
    texture_init(); texture_set_amount(0.45f);         /* room body, audible */
    reverb_init();
    /* Reverb tuned for "weltschaffende Halle": large, dark, slightly driven
     * (warmth on the tail), high damp so the cathedral-sparkle is killed. */
    reverb_set(0.88f, 0.58f); reverb_set_drive(0.18f);
    const float WET = 0.50f;
    const float HISS = 0.022f;

    /* Sustained sub-drone — the "world" under everything */
    pad_note_on(10, dsp_midi_to_hz(26.0f), 0.10f);     /* D1 */
    pad_note_on(11, dsp_midi_to_hz(33.0f), 0.08f);     /* A1 */
    sched_off(57.5f, 10); sched_off(57.5f, 11);

    FILE *f = fopen(out, "wb"); if (!f){ fprintf(stderr,"open %s\n",out); return 1; }
    wav_header(f, (uint32_t)TOTAL_SECS*SR);

    float dryL[BLOCK], dryR[BLOCK], sndL[BLOCK], sndR[BLOCK], wL[BLOCK], wR[BLOCK];
    int16_t out16[BLOCK*2];
    uint32_t total = (uint32_t)TOTAL_SECS*SR, frame = 0;
    size_t ei = 0;
    int distant_fired = 0;
    int peak = 0; double sumsq = 0; long sumN = 0;

    while (frame < total){
        float t = (float)frame / SR;

        /* sparse cell-taps */
        while (ei < N_EV && EV[ei].t <= t){
            uint8_t s = (uint8_t)(ei % 6);
            pad_note_on(s, dsp_midi_to_hz((float)EV[ei].midi), EV[ei].vel);
            sched_off(t + 7.0f, s);   /* long tail per note */
            ++ei;
        }
        /* one distant "world tick" around mid-point */
        if (!distant_fired && t >= 30.0f){ ks_trigger(); distant_fired = 1; }

        for (int i=0;i<npend;){ if (pend[i].t<=t){ pad_note_off(pend[i].src); pend[i]=pend[--npend]; } else ++i; }

        int n = (int)((total-frame)<BLOCK ? (total-frame) : BLOCK);
        memset(dryL,0,sizeof(float)*n); memset(dryR,0,sizeof(float)*n);
        memset(sndL,0,sizeof(float)*n); memset(sndR,0,sizeof(float)*n);

        pad_render_mix(dryL, dryR, sndL, sndR, n, 0.85f);          /* high send for "in the room" */
        texture_render_mix(dryL, dryR, sndL, sndR, n, 0.70f);
        hiss_block(dryL, dryR, n, HISS);
        /* distant tick: very little dry, lots of reverb send */
        for (int i=0;i<n;++i){
            float k = ks_step();
            dryL[i] += k * 0.15f; dryR[i] += k * 0.15f;
            sndL[i] += k * 0.85f; sndR[i] += k * 0.85f;
        }

        reverb_render(sndL, sndR, wL, wR, n);
        for (int i=0;i<n;++i){
            dryL[i] = (dryL[i] + wL[i]*WET) * 1.10f;
            dryR[i] = (dryR[i] + wR[i]*WET) * 1.10f;
        }
        /* SUBTLE PS2-DAC colour: 11-bit, SR/2, light drive — keeps warmth
         * without going into the BLACK-section aggressiveness. */
        crush(dryL, dryR, n, 2, 11, 1.10f);

        for (int i=0;i<n;++i){
            int li = (int)(dryL[i]*32767.0f), ri = (int)(dryR[i]*32767.0f);
            if (li>32767) li=32767; if (li<-32768) li=-32768;
            if (ri>32767) ri=32767; if (ri<-32768) ri=-32768;
            out16[i*2]=(int16_t)li; out16[i*2+1]=(int16_t)ri;
            int a = li<0?-li:li; if (a>peak) peak=a;
            double s = li/32768.0; sumsq += s*s; ++sumN;
        }
        fwrite(out16, sizeof(int16_t), (size_t)n*2, f);
        frame += (uint32_t)n;
    }
    fclose(f);
    printf("PS2 world : %s\n  peak %d (%.1f dBFS), RMS %.4f (%.1f dBFS)\n",
           out, peak, 20.0*log10((double)(peak?peak:1)/32767.0),
           sqrt(sumsq/(double)sumN), 20.0*log10(sqrt(sumsq/(double)sumN)+1e-12));
    return 0;
}
