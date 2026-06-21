/*
 * render_dreamy_warm.c — 60 s "dreamy / warm-pop / PS2-racing-game-chillout"
 * audition. Runs on the V1 warm-chorus pad (src/pad.c — restored after the
 * r18.34/35/36 profile rewrites were reverted as the wrong direction).
 *
 * The shape proven in the audition that the user accepted ("DAS IST ES"):
 *   - A-major composition (bright, hopeful)
 *   - sustained sub-drone A1 + E2 underneath
 *   - 13 sparse cell-style taps tracing an A-add9 / F#m9 / D-add9 arc
 *   - texture.c at 0.12 (subtle room body, not loud noise)
 *   - tape hiss at 0.005 (almost imperceptible — colour only)
 *   - bright medium hall (size 0.60, damp 0.40, drive 0.08) — not doom-dark
 *   - warm tanh saturation only — NO bitcrush (lo-fi DAC didn't fit warm-pop)
 *
 * This is reference material, not a firmware change. It defines the target
 * sound the device should produce; the engine work to actually play this
 * shape from cells + macros is the next step.
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_dreamy_warm.c \
 *      src/dsp.c src/pad.c src/texture.c src/reverb.c -lm \
 *      -o /tmp/render_dreamy_warm
 *   /tmp/render_dreamy_warm /tmp/dreamy_warm.wav
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

/* tape hiss — decorrelated colored noise at -46 dBFS; colour, not body */
static uint32_t hr_L = 0xC0FFEE11u, hr_R = 0xDEADBEEFu;
static inline float hn(uint32_t *r){
    *r = (*r) * 1664525u + 1013904223u;
    return ((int32_t)*r) * (1.0f / 2147483648.0f);
}
static inline void hiss_block(float *L, float *R, int n, float amp){
    for (int i = 0; i < n; ++i){ L[i] += hn(&hr_L) * amp; R[i] += hn(&hr_R) * amp; }
}

/* warm tanh saturation — analog colour, no makeup gain */
static inline void warm_sat(float *L, float *R, int n, float drive){
    for (int i = 0; i < n; ++i){
        L[i] = tanhf(L[i] * drive) * 0.78f;
        R[i] = tanhf(R[i] * drive) * 0.78f;
    }
}

static void put_u32(FILE *f, uint32_t v){ fputc(v&0xff,f); fputc((v>>8)&0xff,f); fputc((v>>16)&0xff,f); fputc((v>>24)&0xff,f); }
static void put_u16(FILE *f, uint16_t v){ fputc(v&0xff,f); fputc((v>>8)&0xff,f); }
static void wav_header(FILE *f, uint32_t nf){
    uint32_t data = nf * 4u;
    fwrite("RIFF", 1, 4, f); put_u32(f, 36 + data); fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); put_u32(f, 16); put_u16(f, 1); put_u16(f, 2);
    put_u32(f, SR); put_u32(f, SR * 4u); put_u16(f, 4); put_u16(f, 16);
    fwrite("data", 1, 4, f); put_u32(f, data);
}

/* A-major (A B C# D E F# G#), sparse dreamy arc — A-add9 / F#m9 / D-add9
 * implied, the F#-minor pull is what makes "racing-game chilled" tracks hopeful
 * without being saccharine. ~13 events in 60 s, all velocities under 0.20. */
typedef struct { float t; int midi; float vel; } evt_t;
static const evt_t EV[] = {
    {  1.0f, 69, 0.20f },   /* A4  */
    {  5.0f, 73, 0.18f },   /* C#5 */
    {  9.0f, 76, 0.17f },   /* E5  */
    { 13.0f, 71, 0.18f },   /* B4  */
    { 18.0f, 78, 0.16f },   /* F#5 — wistful pull */
    { 22.0f, 74, 0.17f },   /* D5  */
    { 27.0f, 69, 0.19f },   /* A4  return home */
    { 32.0f, 81, 0.14f },   /* A5  octave glimmer */
    { 36.0f, 76, 0.16f },   /* E5  */
    { 41.0f, 73, 0.16f },   /* C#5 */
    { 46.0f, 78, 0.13f },   /* F#5 */
    { 51.0f, 71, 0.15f },   /* B4  */
    { 55.0f, 69, 0.14f },   /* A4  resolution */
};
#define N_EV (sizeof EV / sizeof EV[0])

typedef struct { float t; uint8_t src; } pend_t;
static pend_t pend[32]; static int npend = 0;
static void sched_off(float t, uint8_t s){ if (npend < 32){ pend[npend].t = t; pend[npend].src = s; npend++; } }

int main(int argc, char **argv){
    const char *out = (argc > 1) ? argv[1] : "/tmp/dreamy_warm.wav";
    dsp_init();
    pad_init();
    texture_init(); texture_set_amount(0.12f);            /* subtle room */
    reverb_init();
    reverb_set(0.60f, 0.40f); reverb_set_drive(0.08f);
    const float WET   = 0.42f;
    const float HISS  = 0.005f;
    const float SAT_D = 1.10f;

    /* Sub + 5th drone — A1 + E2, held the whole take */
    pad_note_on(10, dsp_midi_to_hz(33.0f), 0.13f);
    pad_note_on(11, dsp_midi_to_hz(40.0f), 0.10f);
    sched_off(57.5f, 10); sched_off(57.5f, 11);

    FILE *f = fopen(out, "wb");
    if (!f){ fprintf(stderr, "open %s\n", out); return 1; }
    wav_header(f, (uint32_t)TOTAL_SECS * SR);

    float dryL[BLOCK], dryR[BLOCK], sndL[BLOCK], sndR[BLOCK], wL[BLOCK], wR[BLOCK];
    int16_t out16[BLOCK * 2];
    uint32_t total = (uint32_t)TOTAL_SECS * SR, frame = 0;
    size_t ei = 0;
    int peak = 0; double sumsq = 0; long sumN = 0;

    while (frame < total){
        float t = (float)frame / SR;

        while (ei < N_EV && EV[ei].t <= t){
            uint8_t s = (uint8_t)(ei % 6);
            pad_note_on(s, dsp_midi_to_hz((float)EV[ei].midi), EV[ei].vel);
            sched_off(t + 5.5f, s);
            ++ei;
        }
        for (int i = 0; i < npend; ){
            if (pend[i].t <= t){ pad_note_off(pend[i].src); pend[i] = pend[--npend]; }
            else ++i;
        }

        int n = (int)((total - frame) < BLOCK ? (total - frame) : BLOCK);
        memset(dryL, 0, sizeof(float) * n); memset(dryR, 0, sizeof(float) * n);
        memset(sndL, 0, sizeof(float) * n); memset(sndR, 0, sizeof(float) * n);

        pad_render_mix(dryL, dryR, sndL, sndR, n, 0.75f);
        texture_render_mix(dryL, dryR, sndL, sndR, n, 0.55f);
        hiss_block(dryL, dryR, n, HISS);

        reverb_render(sndL, sndR, wL, wR, n);
        for (int i = 0; i < n; ++i){
            dryL[i] = (dryL[i] + wL[i] * WET) * 0.95f;
            dryR[i] = (dryR[i] + wR[i] * WET) * 0.95f;
        }
        warm_sat(dryL, dryR, n, SAT_D);

        for (int i = 0; i < n; ++i){
            int li = (int)(dryL[i] * 32767.0f);
            int ri = (int)(dryR[i] * 32767.0f);
            if (li >  32767) li =  32767; if (li < -32768) li = -32768;
            if (ri >  32767) ri =  32767; if (ri < -32768) ri = -32768;
            out16[i*2]     = (int16_t)li;
            out16[i*2 + 1] = (int16_t)ri;
            int a = li < 0 ? -li : li; if (a > peak) peak = a;
            double s = li / 32768.0; sumsq += s * s; ++sumN;
        }
        fwrite(out16, sizeof(int16_t), (size_t)n * 2, f);
        frame += (uint32_t)n;
    }
    fclose(f);
    printf("dreamy warm : %s\n  peak %d (%.1f dBFS), RMS %.4f (%.1f dBFS)\n",
           out, peak, 20.0 * log10((double)(peak ? peak : 1) / 32767.0),
           sqrt(sumsq / (double)sumN), 20.0 * log10(sqrt(sumsq / (double)sumN) + 1e-12));
    return 0;
}
