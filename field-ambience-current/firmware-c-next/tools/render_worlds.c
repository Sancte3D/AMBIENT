/*
 * render_worlds.c — 4 x 45 s audition of the MVP world set:
 *   TOKYO CITY, CRYSTAL COAST, MIDNIGHT DRIVE, AFTER HOURS
 *
 * Each world is one combined preset of pad voice-mix + brightness, texture
 * amount, tape hiss amount, reverb size/damp/drive/wet, master tanh drive,
 * drone root + optional 5th, plus a world-specific composition (key/mode +
 * sparse note arc that fits the vibe).
 *
 * Drums intentionally off in this audition — the user wants drums as a menu
 * option with a per-world appropriateness system; that's the next layer.
 *
 * Runs on the V1 warm-chorus pad restored by r18.37. No engine code yet —
 * this is the SOUND TARGET each world must hit when the real engine refactor
 * builds the world-preset structure.
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_worlds.c \
 *      src/dsp.c src/pad.c src/texture.c src/reverb.c -lm \
 *      -o /tmp/render_worlds
 *   /tmp/render_worlds /tmp/worlds_  (prefix; writes combined + per-world WAVs)
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

#define SR           44100
#define BLOCK        256
#define WORLD_SECS   45
#define N_WORLDS     4

/* ---- tape hiss + saturation (same building blocks as dreamy_warm) ---- */
static uint32_t hr_L = 0xC0FFEE11u, hr_R = 0xDEADBEEFu;
static inline float hn(uint32_t *r){
    *r = (*r) * 1664525u + 1013904223u;
    return ((int32_t)*r) * (1.0f / 2147483648.0f);
}
static inline void hiss_block(float *L, float *R, int n, float amp){
    for (int i = 0; i < n; ++i){ L[i] += hn(&hr_L) * amp; R[i] += hn(&hr_R) * amp; }
}
static inline void warm_sat(float *L, float *R, int n, float drive){
    for (int i = 0; i < n; ++i){
        L[i] = tanhf(L[i] * drive) * 0.78f;
        R[i] = tanhf(R[i] * drive) * 0.78f;
    }
}

/* ---- world definitions --------------------------------------------- */
typedef struct { float t; int midi; float vel; } evt_t;

typedef struct {
    const char *name;
    /* pad timbre */
    float pad_vmix;          /* 0 = warm/pure saw, 0.6 = strings, 1.2 = brass */
    float pad_bright_hz;     /* cutoff offset */
    /* texture + hiss + room + master */
    float texture_amt;
    float hiss_amt;
    float rev_size, rev_damp, rev_drive;
    float wet_amp;
    float sat_drive;
    float master_gain;
    /* drone (set to -1 to disable a side) */
    int   drone_root_midi;
    int   drone_5th_midi;
    float drone_root_vel, drone_5th_vel;
    /* composition */
    const evt_t *evts;
    int         n_evts;
    float       note_hold_s;
} world_demo_t;

/* TOKYO CITY — Tokyo night, neon on wet street, melancholic-bright.
 *   A-major with the F#-minor wistful pull. The "DAS IST ES" reference. */
static const evt_t TOKYO_EVTS[] = {
    {  1.0f, 69, 0.20f }, {  5.0f, 73, 0.18f }, {  9.0f, 76, 0.17f },
    { 13.0f, 71, 0.18f }, { 18.0f, 78, 0.16f }, { 22.0f, 74, 0.17f },
    { 27.0f, 69, 0.19f }, { 32.0f, 81, 0.14f }, { 36.0f, 76, 0.16f },
    { 40.0f, 69, 0.14f },
};

/* CRYSTAL COAST — Mediterranean sunset, chrome high-rises, Frutiger Aero
 *   2003 optimism. D-major, brighter pad, less hall, open horizon. */
static const evt_t COAST_EVTS[] = {
    {  1.0f, 74, 0.20f }, {  5.0f, 78, 0.18f }, {  9.0f, 81, 0.17f },
    { 13.0f, 86, 0.16f }, { 18.0f, 83, 0.17f }, { 22.0f, 78, 0.18f },
    { 27.0f, 74, 0.19f }, { 32.0f, 86, 0.15f }, { 36.0f, 81, 0.17f },
    { 40.0f, 74, 0.16f },
};

/* MIDNIGHT DRIVE — Initial D / Tokyo Xtreme Racer highway, tunnel lights
 *   sweeping past. F#-minor (dorian-flavoured), tape-warm, descending arc. */
static const evt_t DRIVE_EVTS[] = {
    {  1.0f, 78, 0.18f }, {  4.5f, 73, 0.17f }, {  8.0f, 76, 0.16f },
    { 12.0f, 71, 0.17f }, { 16.0f, 78, 0.18f }, { 20.0f, 73, 0.16f },
    { 25.0f, 69, 0.17f }, { 30.0f, 73, 0.16f }, { 34.0f, 76, 0.15f },
    { 38.0f, 71, 0.15f }, { 42.0f, 66, 0.14f },
};

/* AFTER HOURS — Cowboy Bebop jazz bar at 3 AM, lonely-but-okay. C-minor,
 *   thicker tape hiss (vinyl/club), deep drone, sparse jazz-y notes. */
static const evt_t HOURS_EVTS[] = {
    {  1.5f, 75, 0.18f },                                     /* Eb5 */
    {  7.0f, 79, 0.16f },                                     /* G5  */
    { 13.0f, 82, 0.15f },                                     /* Bb5 */
    { 19.5f, 75, 0.17f },                                     /* Eb5 */
    { 26.0f, 72, 0.18f },                                     /* C5  */
    { 33.0f, 70, 0.16f },                                     /* Bb4 */
    { 40.0f, 63, 0.15f },                                     /* Eb4 */
};

static const world_demo_t WORLDS[N_WORLDS] = {
    /* name             vmix  bright tex   hiss   rsiz  rdmp rdrv  wet   sat   gain   d_root d_5th  d_rvel d_5vel evts        n  hold */
    { "TOKYO CITY",     0.00f,   0.0f, 0.12f, 0.005f, 0.60f, 0.40f, 0.08f, 0.42f, 1.10f, 0.95f,  33,  40,   0.13f, 0.10f, TOKYO_EVTS, sizeof TOKYO_EVTS/sizeof TOKYO_EVTS[0], 5.5f },
    { "CRYSTAL COAST",  0.45f, 200.0f, 0.08f, 0.000f, 0.42f, 0.32f, 0.05f, 0.36f, 1.05f, 1.00f,  38,  -1,   0.10f, 0.00f, COAST_EVTS, sizeof COAST_EVTS/sizeof COAST_EVTS[0], 4.0f },
    { "MIDNIGHT DRIVE", 0.20f,   0.0f, 0.15f, 0.008f, 0.55f, 0.50f, 0.15f, 0.40f, 1.15f, 0.95f,  30,  37,   0.13f, 0.10f, DRIVE_EVTS, sizeof DRIVE_EVTS/sizeof DRIVE_EVTS[0], 5.0f },
    { "AFTER HOURS",    0.00f,-100.0f, 0.18f, 0.012f, 0.75f, 0.55f, 0.18f, 0.50f, 1.20f, 0.92f,  24,  31,   0.13f, 0.10f, HOURS_EVTS, sizeof HOURS_EVTS/sizeof HOURS_EVTS[0], 7.5f },
};

/* ---- WAV ----------------------------------------------------------- */
static void put_u32(FILE *f, uint32_t v){ fputc(v&0xff,f); fputc((v>>8)&0xff,f); fputc((v>>16)&0xff,f); fputc((v>>24)&0xff,f); }
static void put_u16(FILE *f, uint16_t v){ fputc(v&0xff,f); fputc((v>>8)&0xff,f); }
static void wav_header(FILE *f, uint32_t nf){
    uint32_t data = nf * 4u;
    fwrite("RIFF", 1, 4, f); put_u32(f, 36 + data); fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); put_u32(f, 16); put_u16(f, 1); put_u16(f, 2);
    put_u32(f, SR); put_u32(f, SR * 4u); put_u16(f, 4); put_u16(f, 16);
    fwrite("data", 1, 4, f); put_u32(f, data);
}

/* ---- scheduler ----------------------------------------------------- */
typedef struct { float t; uint8_t src; } pend_t;
static pend_t pend[32]; static int npend = 0;
static void sched_off(float t, uint8_t s){ if (npend < 32){ pend[npend].t = t; pend[npend].src = s; npend++; } }

/* Render one world to its own WAV and (if combined != NULL) also append to it. */
static int render_world(const world_demo_t *w, const char *out_path, FILE *combined){
    pad_init();
    pad_set_voice_mix(w->pad_vmix);
    pad_set_brightness(w->pad_bright_hz);
    texture_init(); texture_set_amount(w->texture_amt);
    reverb_init();
    reverb_set(w->rev_size, w->rev_damp); reverb_set_drive(w->rev_drive);
    npend = 0;

    /* Drone */
    pad_note_on(10, dsp_midi_to_hz((float)w->drone_root_midi), w->drone_root_vel);
    sched_off((float)(WORLD_SECS - 3), 10);
    if (w->drone_5th_midi > 0){
        pad_note_on(11, dsp_midi_to_hz((float)w->drone_5th_midi), w->drone_5th_vel);
        sched_off((float)(WORLD_SECS - 3), 11);
    }

    FILE *f = fopen(out_path, "wb");
    if (!f){ fprintf(stderr, "open %s\n", out_path); return -1; }
    wav_header(f, (uint32_t)WORLD_SECS * SR);

    float dryL[BLOCK], dryR[BLOCK], sndL[BLOCK], sndR[BLOCK], wL[BLOCK], wR[BLOCK];
    int16_t out16[BLOCK * 2];
    uint32_t total = (uint32_t)WORLD_SECS * SR, frame = 0;
    int ei = 0;
    int peak = 0; double sumsq = 0; long sumN = 0;

    while (frame < total){
        float t = (float)frame / SR;

        while (ei < w->n_evts && w->evts[ei].t <= t){
            uint8_t s = (uint8_t)(ei % 6);
            pad_note_on(s, dsp_midi_to_hz((float)w->evts[ei].midi), w->evts[ei].vel);
            sched_off(t + w->note_hold_s, s);
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
        hiss_block(dryL, dryR, n, w->hiss_amt);
        reverb_render(sndL, sndR, wL, wR, n);
        for (int i = 0; i < n; ++i){
            dryL[i] = (dryL[i] + wL[i] * w->wet_amp) * w->master_gain;
            dryR[i] = (dryR[i] + wR[i] * w->wet_amp) * w->master_gain;
        }
        warm_sat(dryL, dryR, n, w->sat_drive);

        for (int i = 0; i < n; ++i){
            int li = (int)(dryL[i] * 32767.0f); int ri = (int)(dryR[i] * 32767.0f);
            if (li >  32767) li =  32767; if (li < -32768) li = -32768;
            if (ri >  32767) ri =  32767; if (ri < -32768) ri = -32768;
            out16[i*2] = (int16_t)li; out16[i*2+1] = (int16_t)ri;
            int a = li < 0 ? -li : li; if (a > peak) peak = a;
            double s = li / 32768.0; sumsq += s * s; ++sumN;
        }
        fwrite(out16, sizeof(int16_t), (size_t)n * 2, f);
        if (combined) fwrite(out16, sizeof(int16_t), (size_t)n * 2, combined);
        frame += (uint32_t)n;
    }
    fclose(f);
    pad_all_off();
    double rms = sqrt(sumsq / (double)sumN);
    printf("  %-16s peak %d (%5.1f dBFS), RMS %.4f (%.1f dBFS)\n",
           w->name, peak, 20.0*log10((double)(peak?peak:1)/32767.0),
           rms, 20.0*log10(rms+1e-12));
    return 0;
}

int main(int argc, char **argv){
    const char *prefix = (argc > 1) ? argv[1] : "/tmp/worlds_";
    dsp_init();

    char combined_path[512];
    snprintf(combined_path, sizeof combined_path, "%sall.wav", prefix);
    FILE *cf = fopen(combined_path, "wb");
    if (!cf){ fprintf(stderr, "open %s\n", combined_path); return 1; }
    wav_header(cf, (uint32_t)WORLD_SECS * SR * N_WORLDS);

    printf("Rendering %d worlds x %d s -> %s + per-world files\n",
           N_WORLDS, WORLD_SECS, combined_path);

    for (int i = 0; i < N_WORLDS; ++i){
        char path[512];
        snprintf(path, sizeof path, "%s%d_%s.wav", prefix, i, WORLDS[i].name);
        for (char *p = path; *p; ++p) if (*p == ' ') *p = '_';
        if (render_world(&WORLDS[i], path, cf) != 0){ fclose(cf); return 1; }
    }
    fclose(cf);
    printf("Combined: %s (%d s total)\n", combined_path, WORLD_SECS * N_WORLDS);
    return 0;
}
