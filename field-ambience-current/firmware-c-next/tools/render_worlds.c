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

/* ---- universal wind/breath: REAL resonant bandpass on pink-ish noise,
 * slow sweep + amplitude gusts. The previous version was a wide-band 1-pole
 * cascade and sounded like pure white noise — this uses dsp_svf_bp at Q≈2.5
 * for a narrow band, slow LFO that walks the centre between 350..900 Hz over
 * ~14 s, plus random gust envelopes every 4..8 s for the "atmen" feel. */
static uint32_t wnd_rng_L = 0xACE12345u, wnd_rng_R = 0x7B19F88Au;
static dsp_svf_t wnd_bpL_f, wnd_bpR_f;
static float wnd_lfo = 0;
static float wnd_pink_L_b0=0, wnd_pink_L_b1=0, wnd_pink_L_b2=0;
static float wnd_pink_R_b0=0, wnd_pink_R_b1=0, wnd_pink_R_b2=0;
static float wnd_gust_env = 0.5f;
static int   wnd_gust_until = 0;
static int   wnd_initialised = 0;
static inline float wnd_white(uint32_t *r){
    *r = (*r) * 1664525u + 1013904223u;
    return ((int32_t)*r) * (1.0f / 2147483648.0f);
}
/* Cheap 3-pole pink-noise filter (Paul Kellet approximation). Wind sources
 * have far more low-mid energy than white noise — this is what makes it
 * sound like air moving instead of static. */
static inline float wnd_pink(uint32_t *rng, float *b0, float *b1, float *b2){
    float w = wnd_white(rng);
    *b0 = 0.99765f * (*b0) + w * 0.0990460f;
    *b1 = 0.96300f * (*b1) + w * 0.2965164f;
    *b2 = 0.57000f * (*b2) + w * 1.0526913f;
    return (*b0 + *b1 + *b2 + w * 0.1848f) * 0.18f;
}
static inline void wind_block(float *L, float *R, int n, float amount){
    if (amount <= 0.0f) return;
    if (!wnd_initialised){
        dsp_svf_reset(&wnd_bpL_f); dsp_svf_reset(&wnd_bpR_f);
        wnd_gust_until = (int)SR * 5;
        wnd_initialised = 1;
    }
    for (int i = 0; i < n; ++i){
        /* 14 s sweep: centre 350..900 Hz */
        wnd_lfo += (1.0f / 14.0f) / (float)SR;
        if (wnd_lfo >= 1.0f) wnd_lfo -= 1.0f;
        float s = sinf(wnd_lfo * 6.2831853f);
        float centre = 625.0f + s * 275.0f;
        dsp_svf_set(&wnd_bpL_f, centre,        2.5f);
        dsp_svf_set(&wnd_bpR_f, centre * 1.07f, 2.5f);   /* slight L/R offset */

        /* random gusts every 4..8 s */
        if (--wnd_gust_until <= 0){
            wnd_gust_env   = 0.85f + (wnd_white(&wnd_rng_L) * 0.5f + 0.5f) * 0.5f;
            wnd_gust_until = (int)SR * (4 + (int)((wnd_white(&wnd_rng_R) * 0.5f + 0.5f) * 4));
        }
        wnd_gust_env *= 0.99996f;                       /* slow attack/decay */
        float gust = 0.40f + wnd_gust_env * 0.6f;

        float pL = wnd_pink(&wnd_rng_L, &wnd_pink_L_b0, &wnd_pink_L_b1, &wnd_pink_L_b2);
        float pR = wnd_pink(&wnd_rng_R, &wnd_pink_R_b0, &wnd_pink_R_b1, &wnd_pink_R_b2);
        float bpL = dsp_svf_bp(&wnd_bpL_f, pL);
        float bpR = dsp_svf_bp(&wnd_bpR_f, pR);

        L[i] += bpL * amount * gust;
        R[i] += bpR * amount * gust;
    }
}

/* ---- per-world atmospheric layers ---------------------------------- */

/* RAIN (TOKYO CITY): high-pass white noise (the "sshhh") + sparse low-Q
 * resonant impulses (the drops on pavement). */
static uint32_t rn_rng = 0x5417F00Du;
static float rn_lpL=0, rn_lpR=0;
static float rn_drop_env=0, rn_drop_freq=2400;
static int   rn_until_drop=0;
static inline float rn_white(void){
    rn_rng = rn_rng * 1664525u + 1013904223u;
    return ((int32_t)rn_rng) * (1.0f / 2147483648.0f);
}
static inline void rain_block(float *L, float *R, int n, float amount){
    if (amount <= 0.0f) return;
    for (int i = 0; i < n; ++i){
        /* shhh: high-passed noise = white - slow LP (HP corner ~2 kHz) */
        float wL = rn_white(), wR = rn_white();
        rn_lpL += 0.20f * (wL - rn_lpL);
        rn_lpR += 0.20f * (wR - rn_lpR);
        float hpL = wL - rn_lpL * 1.6f;
        float hpR = wR - rn_lpR * 1.6f;
        /* sparse drops: trigger an exponential 2-3 kHz hit every ~3500-9000 samples */
        if (--rn_until_drop <= 0){
            rn_drop_env  = 0.35f;
            rn_drop_freq = 2000.0f + (rn_white() * 0.5f + 0.5f) * 1200.0f;
            rn_until_drop = 3500 + (int)((rn_white() * 0.5f + 0.5f) * 5500.0f);
        }
        float drop = 0.0f;
        if (rn_drop_env > 0.0001f){
            static float drop_phase = 0;
            drop_phase += rn_drop_freq / (float)SR;
            if (drop_phase >= 1.0f) drop_phase -= 1.0f;
            drop = sinf(drop_phase * 6.2831853f) * rn_drop_env;
            rn_drop_env *= 0.998f;
        }
        L[i] += (hpL * 0.55f + drop) * amount;
        R[i] += (hpR * 0.55f - drop * 0.7f) * amount;  /* slight stereo offset */
    }
}

/* WAVES (CRYSTAL COAST): brown noise with very slow amplitude swell.
 * "wash in / wash out" of the surf. */
static uint32_t wv_rng = 0xBADCAFE1u;
static float wv_brnL=0, wv_brnR=0, wv_lfo=0;
static inline float wv_white(void){
    wv_rng = wv_rng * 1664525u + 1013904223u;
    return ((int32_t)wv_rng) * (1.0f / 2147483648.0f);
}
static inline void waves_block(float *L, float *R, int n, float amount){
    if (amount <= 0.0f) return;
    for (int i = 0; i < n; ++i){
        /* brown via integration */
        wv_brnL = wv_brnL * 0.996f + wv_white() * 0.04f;
        wv_brnR = wv_brnR * 0.996f + wv_white() * 0.04f;
        /* 0.10 Hz swell — "wave every 10 s" */
        wv_lfo += 0.10f / (float)SR;
        if (wv_lfo >= 1.0f) wv_lfo -= 1.0f;
        float swell = 0.35f + 0.55f * (0.5f + 0.5f * sinf(wv_lfo * 6.2831853f));
        L[i] += wv_brnL * amount * swell;   /* the *3.0 here was the loud culprit */
        R[i] += wv_brnR * amount * swell;
    }
}

/* WIND + DISTANT TRAFFIC (MIDNIGHT DRIVE): broader wind plus occasional
 * filtered low-mid sweep (the "whoosh" of an oncoming/passing car). */
static uint32_t tr_rng = 0x912EAFEEu;
static float tr_lpL=0, tr_lpR=0;
static float tr_sweep_env=0, tr_sweep_freq=200;
static int   tr_until_sweep=0;
static inline float tr_white(void){
    tr_rng = tr_rng * 1664525u + 1013904223u;
    return ((int32_t)tr_rng) * (1.0f / 2147483648.0f);
}
static inline void traffic_block(float *L, float *R, int n, float amount){
    if (amount <= 0.0f) return;
    for (int i = 0; i < n; ++i){
        /* mid-band wind, broader than the universal one */
        float wL = tr_white(), wR = tr_white();
        tr_lpL += 0.10f * (wL - tr_lpL);
        tr_lpR += 0.10f * (wR - tr_lpR);
        /* sweep: filtered noise burst whose centre drops over ~1 s */
        if (--tr_until_sweep <= 0){
            tr_sweep_env  = 0.55f;
            tr_sweep_freq = 800.0f;
            tr_until_sweep = (int)SR * (3 + (int)((tr_white() * 0.5f + 0.5f) * 6));   /* every 3-9 s */
        }
        float sweep = 0.0f;
        if (tr_sweep_env > 0.0005f){
            static float ph = 0;
            tr_sweep_freq *= 0.99996f;
            if (tr_sweep_freq < 80.0f) tr_sweep_freq = 80.0f;
            ph += tr_sweep_freq / (float)SR; if (ph >= 1.0f) ph -= 1.0f;
            float src = wL * 0.6f + sinf(ph * 6.2831853f) * 0.4f;
            sweep = src * tr_sweep_env;
            tr_sweep_env *= 0.99988f;
        }
        L[i] += (tr_lpL * 0.7f + sweep * 0.45f) * amount;
        R[i] += (tr_lpR * 0.7f - sweep * 0.45f) * amount;
    }
}

/* VINYL + SMOKY BAR (AFTER HOURS): hi-passed noise crackle + sparse
 * sharper crackle pops + very slow low rumble (distant city through walls). */
static uint32_t vy_rng = 0xC0DEFEEDu;
static float vy_lpL=0, vy_lpR=0, vy_rumL=0, vy_rumR=0;
static int   vy_until_pop = 0;
static float vy_pop_env = 0;
static inline float vy_white(void){
    vy_rng = vy_rng * 1664525u + 1013904223u;
    return ((int32_t)vy_rng) * (1.0f / 2147483648.0f);
}
static inline void vinyl_block(float *L, float *R, int n, float amount){
    if (amount <= 0.0f) return;
    for (int i = 0; i < n; ++i){
        float wL = vy_white(), wR = vy_white();
        /* hi-pass crackle: subtract slow LP */
        vy_lpL += 0.30f * (wL - vy_lpL);
        vy_lpR += 0.30f * (wR - vy_lpR);
        float crL = wL - vy_lpL;
        float crR = wR - vy_lpR;
        /* sparse sharp pops */
        if (--vy_until_pop <= 0){
            vy_pop_env = 0.5f + (vy_white() * 0.5f + 0.5f) * 0.4f;
            vy_until_pop = 800 + (int)((vy_white() * 0.5f + 0.5f) * 3000.0f);
        }
        float pop = 0.0f;
        if (vy_pop_env > 0.001f){
            pop = vy_white() * vy_pop_env;
            vy_pop_env *= 0.975f;
        }
        /* slow far-city rumble: brown noise heavily LP'd */
        vy_rumL = vy_rumL * 0.9985f + vy_white() * 0.0008f;
        vy_rumR = vy_rumR * 0.9985f + vy_white() * 0.0008f;
        L[i] += (crL * 0.35f + pop * 0.55f + vy_rumL * 1.5f) * amount;
        R[i] += (crR * 0.35f - pop * 0.55f + vy_rumR * 1.5f) * amount;
    }
}

/* ---- world definitions --------------------------------------------- */
typedef struct { float t; int midi; float vel; } evt_t;

typedef enum { AMB_NONE = 0, AMB_RAIN, AMB_WAVES, AMB_TRAFFIC, AMB_VINYL } amb_kind_t;

typedef struct {
    const char *name;
    /* pad timbre */
    float pad_vmix;          /* 0 = warm/pure saw, 0.6 = strings, 1.2 = brass */
    float pad_bright_hz;     /* cutoff offset */
    /* ambient layers — texture.c body rumble OFF (was the "brumm"); replaced
     * by a universal wind/breath at wind_amt + a world-specific layer below. */
    float wind_amt;          /* universal wind/breath, runs in every world */
    amb_kind_t world_amb;    /* which world-specific layer */
    float world_amb_amt;
    /* tape hiss + room + master */
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

/* Ambience amounts re-calibrated from the SOLO measurement so every layer
 * sits subtle (~-44 dBFS RMS solo) instead of fighting the pad. The previous
 * waves *3.0 + uncalibrated amounts made waves/rain the "loud noise that
 * drowns everything"; wind was -60 dBFS (inaudible). Targets:
 *   wind  ~-48 (subtle universal bed, now actually present)
 *   rain  ~-44   waves ~-44   traffic ~-44   vinyl ~-42 (bar a touch louder)
 *   hiss  ~-47 */
static const world_demo_t WORLDS[N_WORLDS] = {
    /* name             vmix  bright wind  amb         amb_amt hiss   rsiz  rdmp rdrv  wet   sat   gain  d_root d_5th  d_rvel d_5vel evts        n  hold */
    { "TOKYO CITY",     0.00f,   0.0f, 0.090f, AMB_RAIN,    0.024f, 0.005f, 0.60f, 0.40f, 0.08f, 0.42f, 1.10f, 0.95f,  33,  40,   0.13f, 0.10f, TOKYO_EVTS, sizeof TOKYO_EVTS/sizeof TOKYO_EVTS[0], 5.5f },
    { "CRYSTAL COAST",  0.45f, 200.0f, 0.075f, AMB_WAVES,   0.035f, 0.000f, 0.42f, 0.32f, 0.05f, 0.36f, 1.05f, 1.00f,  38,  -1,   0.10f, 0.00f, COAST_EVTS, sizeof COAST_EVTS/sizeof COAST_EVTS[0], 4.0f },
    { "MIDNIGHT DRIVE", 0.20f,   0.0f, 0.110f, AMB_TRAFFIC, 0.066f, 0.008f, 0.55f, 0.50f, 0.15f, 0.40f, 1.15f, 0.95f,  30,  37,   0.13f, 0.10f, DRIVE_EVTS, sizeof DRIVE_EVTS/sizeof DRIVE_EVTS[0], 5.0f },
    { "AFTER HOURS",    0.00f,-100.0f, 0.060f, AMB_VINYL,   0.051f, 0.008f, 0.75f, 0.55f, 0.18f, 0.50f, 1.20f, 0.92f,  24,  31,   0.13f, 0.10f, HOURS_EVTS, sizeof HOURS_EVTS/sizeof HOURS_EVTS[0], 7.5f },
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
    texture_init(); texture_set_amount(0.0f);     /* OFF — was the brumm source */
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
        /* universal wind/breath + world-specific ambience layer, both summed
         * into the dry bus and then also sent (a bit) to the reverb */
        wind_block(dryL, dryR, n, w->wind_amt);
        switch (w->world_amb){
        case AMB_RAIN:    rain_block   (dryL, dryR, n, w->world_amb_amt); break;
        case AMB_WAVES:   waves_block  (dryL, dryR, n, w->world_amb_amt); break;
        case AMB_TRAFFIC: traffic_block(dryL, dryR, n, w->world_amb_amt); break;
        case AMB_VINYL:   vinyl_block  (dryL, dryR, n, w->world_amb_amt); break;
        case AMB_NONE:    default: break;
        }
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

/* ---- diagnostic: render each layer SOLO 5 s and report level ---------- */
static void measure_one(const char *name, void (*fn)(float*,float*,int,float), float amt){
    float L[BLOCK], R[BLOCK];
    int peak = 0; double sumsq = 0; long sumN = 0;
    int total = SR * 5;
    for (int frame = 0; frame < total; frame += BLOCK){
        int n = (total - frame) < BLOCK ? (total - frame) : BLOCK;
        memset(L, 0, sizeof(float)*n); memset(R, 0, sizeof(float)*n);
        fn(L, R, n, amt);
        for (int i = 0; i < n; ++i){
            int li = (int)(L[i]*32767.0f);
            if (li > 32767) li = 32767; if (li < -32768) li = -32768;
            int a = li < 0 ? -li : li; if (a > peak) peak = a;
            double s = L[i]; sumsq += s*s; ++sumN;
        }
    }
    double rms = sqrt(sumsq/(double)sumN);
    printf("  %-10s amt=%.3f  peak %5.1f dBFS   RMS %5.1f dBFS\n",
           name, amt, 20.0*log10((double)(peak?peak:1)/32767.0), 20.0*log10(rms+1e-12));
}

int main(int argc, char **argv){
    const char *prefix = (argc > 1) ? argv[1] : "/tmp/worlds_";
    dsp_init();

    if (argc > 2 && strcmp(argv[2], "measure") == 0){
        printf("=== per-layer SOLO levels (5 s each, at the per-world amount) ===\n");
        measure_one("wind",    wind_block,    0.110f);   /* DRIVE (max) */
        measure_one("rain",    rain_block,    0.024f);   /* TOKYO */
        measure_one("waves",   waves_block,   0.055f);   /* COAST */
        measure_one("traffic", traffic_block, 0.066f);   /* DRIVE */
        measure_one("vinyl",   vinyl_block,   0.051f);   /* HOURS */
        measure_one("hiss",    hiss_block,    0.008f);   /* HOURS */
        printf("--- same generators at amt=1.0 (raw level, amount removed) ---\n");
        measure_one("wind",    wind_block,    1.0f);
        measure_one("rain",    rain_block,    1.0f);
        measure_one("waves",   waves_block,   1.0f);
        measure_one("traffic", traffic_block, 1.0f);
        measure_one("vinyl",   vinyl_block,   1.0f);
        measure_one("hiss",    hiss_block,    1.0f);
        return 0;
    }

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
