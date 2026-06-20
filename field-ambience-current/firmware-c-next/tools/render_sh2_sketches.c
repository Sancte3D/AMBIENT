/*
 * render_sh2_sketches.c — 3 x 30 s sketches of PS2-era Silent-Hill-2-style
 * ambient: "Theme of Laura" (melancholic), "Save Room / Forest" (unheimlich),
 * "Black Fairy" (dirty-loud).
 *
 * Honest skizze: built from existing pad / texture / reverb modules plus three
 * tiny extras inline — tape hiss, Karplus-Strong metal clang, simple bit-
 * crushed drums — to cover what's missing for the SH2 idiom. No firmware
 * change; this only auditions the direction before any engine work.
 *
 * Layout:
 *   0..30 s : LAURA — sparse mournful pad notes + tape hiss + dark big hall.
 *             No drums, no clangs, no distortion. Theme-of-Laura territory.
 *   30..60 s: SAVE  — sub-drone foundation + loud noise bed + occasional
 *             metal clangs + tritone dyad pad stabs. Unsettling, atmospheric.
 *   60..90 s: BLACK — bitcrushed drum pattern + distorted noise wash +
 *             saturated pad. Dirty / loud / lo-fi-aggressive.
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_sh2_sketches.c \
 *      src/dsp.c src/pad.c src/texture.c src/reverb.c -lm \
 *      -o /tmp/render_sh2_sketches
 *   /tmp/render_sh2_sketches /tmp/sh2_sketches.wav
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
#define TOTAL_SECS (3 * SECT_SECS)

/* ----------------------------- tape hiss ------------------------------ */
/* High-pass colored noise, very quiet, decorrelated L/R. PS2-DAC vibe. */
static uint32_t hiss_rng_L = 0xC0FFEE11u, hiss_rng_R = 0xDEADBEEFu;
static float hp_zL=0, hp_zR=0;
static inline float hiss_sample(uint32_t *r){
    *r = (*r)*1664525u + 1013904223u;
    return ((int32_t)*r) * (1.0f / 2147483648.0f);
}
static inline void hiss_block(float *L, float *R, int n, float amp){
    /* simple first-order HP at ~3 kHz to colour the noise tape-ish */
    const float a = 0.86f;
    for (int i=0;i<n;++i){
        float nL = hiss_sample(&hiss_rng_L), nR = hiss_sample(&hiss_rng_R);
        float hL = a*(hp_zL + nL - hp_zL);   /* basically nL for simplicity */
        float hR = a*(hp_zR + nR - hp_zR);
        hp_zL = nL; hp_zR = nR;
        L[i] += hL * amp;
        R[i] += hR * amp;
    }
}

/* --------------------------- metal clang ------------------------------ */
/* One-shot Karplus-Strong: noise burst → delay line with decaying feedback.
 * Drive on the output for the industrial bite. */
#define CLANG_LEN 800        /* 800 samples = ~55 Hz fundamental */
static float clang_buf[CLANG_LEN];
static int   clang_w = 0;
static float clang_amp = 0.0f;
static float clang_fb = 0.992f;
static int   clang_active_samples = 0;
static int   clang_pan = 0;    /* 0 = L, 1 = R */
static void clang_trigger(int pan){
    for (int i=0;i<CLANG_LEN;++i)
        clang_buf[i] = ((hiss_sample(&hiss_rng_L)) * 0.5f);
    clang_w = 0;
    clang_amp = 0.55f;
    clang_active_samples = (int)(SR * 4.0f);   /* up to 4 s ring */
    clang_pan = pan;
}
static inline float clang_step(void){
    if (clang_active_samples <= 0) return 0.0f;
    int r = (clang_w + 1) % CLANG_LEN;
    float avg = 0.5f*(clang_buf[clang_w] + clang_buf[r]);
    float s = clang_buf[clang_w];
    clang_buf[clang_w] = avg * clang_fb;
    clang_w = r;
    clang_amp *= 0.99996f;
    clang_active_samples--;
    /* tanh drive for industrial bite */
    return tanhf(s * clang_amp * 3.5f) * 0.45f;
}

/* ----------------------------- drums ---------------------------------- */
typedef struct { float env; float freq; float phase; int active_samples; int kind; } drum_t;
static drum_t drum;
static void drum_trigger(int kind){
    drum.kind = kind;
    drum.env = 1.0f;
    if (kind == 0){           /* kick */
        drum.freq = 120.0f;
        drum.active_samples = (int)(SR*0.45f);
    } else if (kind == 1){    /* snare */
        drum.active_samples = (int)(SR*0.25f);
    } else {                  /* hat */
        drum.active_samples = (int)(SR*0.08f);
    }
    drum.phase = 0.0f;
}
static inline float drum_step(void){
    if (drum.active_samples <= 0) return 0.0f;
    drum.active_samples--;
    float s = 0.0f;
    if (drum.kind == 0){
        drum.freq *= 0.99985f;
        if (drum.freq < 45.0f) drum.freq = 45.0f;
        drum.phase += drum.freq / SR;
        if (drum.phase >= 1.0f) drum.phase -= 1.0f;
        s = sinf(drum.phase * 6.2831853f);
        drum.env *= 0.99985f;
    } else if (drum.kind == 1){
        float n = hiss_sample(&hiss_rng_L);
        s = n * 0.7f + sinf(drum.phase * 6.2831853f) * 0.3f;
        drum.phase += 185.0f / SR; if (drum.phase >= 1.0f) drum.phase -= 1.0f;
        drum.env *= 0.9993f;
    } else {
        float n = hiss_sample(&hiss_rng_R);
        /* crude HPF: difference of noise samples */
        static float prev = 0;
        s = (n - prev) * 0.6f;
        prev = n;
        drum.env *= 0.9985f;
    }
    return s * drum.env;
}

/* ------------------------- master FX: bitcrush + drive --------------- */
static int   bc_hold_ctr = 0;
static float bc_hL = 0, bc_hR = 0;
static inline void bitcrush_drive(float *L, float *R, int n,
                                  int sr_div, int bits, float drive_pre){
    int   levels = (1 << bits);
    float scale  = (float)(levels/2 - 1);
    for (int i=0;i<n;++i){
        if (bc_hold_ctr <= 0){
            bc_hL = L[i]; bc_hR = R[i];
            bc_hold_ctr = sr_div;
        }
        bc_hold_ctr--;
        float qL = floorf(bc_hL * scale + 0.5f) / scale;
        float qR = floorf(bc_hR * scale + 0.5f) / scale;
        L[i] = tanhf(qL * drive_pre) * 0.85f;
        R[i] = tanhf(qR * drive_pre) * 0.85f;
    }
}

/* ----------------------------- WAV out ------------------------------- */
static void put_u32(FILE *f,uint32_t v){fputc(v&0xff,f);fputc((v>>8)&0xff,f);fputc((v>>16)&0xff,f);fputc((v>>24)&0xff,f);}
static void put_u16(FILE *f,uint16_t v){fputc(v&0xff,f);fputc((v>>8)&0xff,f);}
static void wav_header(FILE *f,uint32_t nf){
    uint32_t data=nf*4u;
    fwrite("RIFF",1,4,f); put_u32(f,36+data); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); put_u32(f,16); put_u16(f,1); put_u16(f,2);
    put_u32(f,SR); put_u32(f,SR*4u); put_u16(f,4); put_u16(f,16);
    fwrite("data",1,4,f); put_u32(f,data);
}

/* schedule helpers — small fixed queue for pad note-offs */
typedef struct { float t; uint8_t src; } pend_t;
static pend_t pend[64]; static int npend = 0;
static void sched_off(float t, uint8_t s){ if(npend<64){pend[npend].t=t;pend[npend].src=s;npend++;} }

int main(int argc, char **argv){
    const char *out=(argc>1)?argv[1]:"/tmp/sh2_sketches.wav";
    dsp_init();
    pad_init(); pad_set_profile(0);     /* Bed — darkest, slowest profile */
    texture_init(); texture_set_amount(0.0f);
    reverb_init();
    memset(&drum,0,sizeof drum);

    FILE *f=fopen(out,"wb"); if(!f){fprintf(stderr,"open %s\n",out);return 1;}
    wav_header(f,(uint32_t)TOTAL_SECS*SR);

    float dryL[BLOCK],dryR[BLOCK],sndL[BLOCK],sndR[BLOCK],wL[BLOCK],wR[BLOCK];
    int16_t out16[BLOCK*2];

    uint32_t total=(uint32_t)TOTAL_SECS*SR, frame=0;
    int section = -1;      /* 0/1/2 */
    int peak=0; double sumsq=0; long sumN=0;

    /* sparse event lists per section. Time is relative to section start. */
    struct laura_ev { float t; int midi; };
    static const struct laura_ev LAURA[] = {
        {  0.5f, 50 }, {  6.0f, 53 }, { 11.0f, 48 },
        { 16.5f, 55 }, { 22.0f, 50 }, { 27.0f, 48 },
    };
    /* SAVE section: tritone dyads + clangs */
    struct save_ev { float t; int midi1, midi2; int clang; };
    static const struct save_ev SAVE[] = {
        {  1.0f, 38, 44, 0 },                /* D2 + Ab2 tritone */
        {  9.0f,  0,  0, 1 },                /* clang */
        { 13.0f, 39, 45, 0 },                /* Eb2 + A2 */
        { 19.0f,  0,  0, 1 },
        { 23.0f, 38, 44, 0 },
    };
    /* BLACK section: 16th-grid drum pattern at 88 BPM, sparse pad stabs */
    /* 88 BPM → 60/88 = 0.6818 s per beat, 16th = 0.1705 s */
    const float STEP = 60.0f/88.0f/4.0f;

    /* per-section state we need to reset on section change */
    float hiss_amp=0, clang_send=0, pad_send=0, tex_amount=0, wet_amp=0, drum_send=0;
    int   bc_sr_div=1, bc_bits=16; float bc_drive=1.0f;
    float master_gain=1.0f;
    size_t laura_i=0, save_i=0;
    int last_step = -1;
    /* BLACK pattern: 16 steps, x = kick, . = none. K=kick S=snare H=hat */
    /*                  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 */
    static const char *BLACK_KICK = "K.....K....K....";
    static const char *BLACK_SNR  = "....S.......S...";
    static const char *BLACK_HAT  = "..H...H...H...H.";

    while (frame < total){
        float gt = (float)frame/SR;                      /* global time */
        int sec_now = (int)(gt / (float)SECT_SECS);
        float st = gt - (float)(sec_now * SECT_SECS);    /* time-in-section */

        if (sec_now != section){
            section = sec_now;
            pad_all_off(); npend = 0;
            laura_i = save_i = 0; last_step = -1;
            switch (section){
            case 0:   /* LAURA */
                texture_set_amount(0.30f); tex_amount = 0.30f;
                reverb_set(0.85f, 0.55f); reverb_set_drive(0.20f);
                hiss_amp = 0.020f;
                pad_send = 0.85f; clang_send = 0.0f; drum_send = 0.0f;
                wet_amp = 0.55f;
                bc_sr_div = 1; bc_bits = 16; bc_drive = 1.05f;   /* almost no FX */
                master_gain = 1.10f;
                break;
            case 1:   /* SAVE */
                texture_set_amount(0.70f); tex_amount = 0.70f;
                reverb_set(0.95f, 0.65f); reverb_set_drive(0.30f);
                hiss_amp = 0.040f;
                pad_send = 0.90f; clang_send = 0.70f; drum_send = 0.0f;
                wet_amp = 0.65f;
                bc_sr_div = 1; bc_bits = 14; bc_drive = 1.15f;
                master_gain = 1.05f;
                /* immediate sub-drone */
                pad_note_on(10, dsp_midi_to_hz(26.0f), 0.10f);   /* D1 */
                pad_note_on(11, dsp_midi_to_hz(33.0f), 0.07f);   /* A1 */
                sched_off(28.0f, 10); sched_off(28.0f, 11);
                break;
            case 2:   /* BLACK */
                texture_set_amount(0.55f); tex_amount = 0.55f;
                reverb_set(0.78f, 0.50f); reverb_set_drive(0.40f);
                hiss_amp = 0.030f;
                pad_send = 0.60f; clang_send = 0.0f; drum_send = 0.45f;
                wet_amp = 0.45f;
                bc_sr_div = 3; bc_bits = 6; bc_drive = 1.55f;   /* DIRTY */
                master_gain = 1.15f;
                /* sustained dirty pad: tritone Eb + A around mid */
                pad_note_on(10, dsp_midi_to_hz(39.0f), 0.10f);   /* Eb2 */
                pad_note_on(11, dsp_midi_to_hz(45.0f), 0.08f);   /* A2  */
                sched_off(28.5f, 10); sched_off(28.5f, 11);
                break;
            }
        }

        /* per-section event scheduling */
        if (section == 0){
            while (laura_i < sizeof LAURA/sizeof LAURA[0] && LAURA[laura_i].t <= st){
                int midi = LAURA[laura_i].midi;
                uint8_t s = (uint8_t)(laura_i % 6);
                pad_note_on(s, dsp_midi_to_hz((float)midi), 0.12f);
                sched_off(gt + 4.5f, s);
                ++laura_i;
            }
        } else if (section == 1){
            while (save_i < sizeof SAVE/sizeof SAVE[0] && SAVE[save_i].t <= st){
                const struct save_ev *e = &SAVE[save_i];
                if (e->clang){
                    clang_trigger((int)(save_i % 2));
                } else {
                    uint8_t s1 = (uint8_t)(save_i*2 % 4);
                    uint8_t s2 = (uint8_t)(save_i*2 % 4 + 1);
                    pad_note_on(s1, dsp_midi_to_hz((float)e->midi1), 0.09f);
                    pad_note_on(s2, dsp_midi_to_hz((float)e->midi2), 0.07f);
                    sched_off(gt + 8.0f, s1); sched_off(gt + 8.0f, s2);
                }
                ++save_i;
            }
        } else if (section == 2){
            int step = (int)(st / STEP);
            if (step != last_step && step < 64){
                last_step = step;
                int s16 = step % 16;
                if (BLACK_KICK[s16] != '.') drum_trigger(0);
                else if (BLACK_SNR[s16] != '.') drum_trigger(1);
                else if (BLACK_HAT[s16] != '.') drum_trigger(2);
            }
        }

        for (int i=0;i<npend;){ if (pend[i].t<=gt){ pad_note_off(pend[i].src); pend[i]=pend[--npend]; } else ++i; }

        int n = (int)((total-frame)<BLOCK ? (total-frame) : BLOCK);
        memset(dryL,0,sizeof(float)*n); memset(dryR,0,sizeof(float)*n);
        memset(sndL,0,sizeof(float)*n); memset(sndR,0,sizeof(float)*n);

        pad_render_mix(dryL, dryR, sndL, sndR, n, pad_send);
        texture_render_mix(dryL, dryR, sndL, sndR, n, pad_send * 0.8f);
        hiss_block(dryL, dryR, n, hiss_amp);

        /* one-shot clangs/drums sample-by-sample into dry and reverb send */
        for (int i=0;i<n;++i){
            float c = clang_step();
            if (clang_pan == 0){ dryL[i] += c; sndL[i] += c * clang_send; }
            else                { dryR[i] += c; sndR[i] += c * clang_send; }
            float d = drum_step();
            dryL[i] += d * 0.85f; dryR[i] += d * 0.85f;
            sndL[i] += d * drum_send; sndR[i] += d * drum_send;
        }

        reverb_render(sndL, sndR, wL, wR, n);
        for (int i=0;i<n;++i){
            dryL[i] = (dryL[i] + wL[i]*wet_amp) * master_gain;
            dryR[i] = (dryR[i] + wR[i]*wet_amp) * master_gain;
        }
        bitcrush_drive(dryL, dryR, n, bc_sr_div, bc_bits, bc_drive);

        for (int i=0;i<n;++i){
            int li = (int)(dryL[i] * 32767.0f); int ri = (int)(dryR[i] * 32767.0f);
            if (li > 32767) li = 32767; if (li < -32768) li = -32768;
            if (ri > 32767) ri = 32767; if (ri < -32768) ri = -32768;
            out16[i*2] = (int16_t)li; out16[i*2+1] = (int16_t)ri;
            int a = li<0?-li:li; if (a>peak) peak = a;
            double s = li/32768.0; sumsq += s*s; ++sumN;
        }
        fwrite(out16, sizeof(int16_t), (size_t)n*2, f);
        frame += (uint32_t)n;
    }
    fclose(f);
    printf("SH2 sketches : %s\n  peak %d (%.1f dBFS), RMS %.4f (%.1f dBFS)\n",
           out, peak, 20.0*log10((double)(peak?peak:1)/32767.0),
           sqrt(sumsq/(double)sumN), 20.0*log10(sqrt(sumsq/(double)sumN)+1e-12));
    return 0;
}
