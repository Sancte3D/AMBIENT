/*
 * beat.c — Crystal-Castles-style synth drum machine. See beat.h.
 */

#include "v2/beat.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR 44100.0f

/* 16-step patterns: bit s (0..15) set = hit on that 16th. One row each for
 * kick / snare / closed-hat / open-hat. Designed melancholic-driving. */
typedef struct { uint16_t k, s, hc, ho; } pattern_t;

static const pattern_t PATTERNS[BEAT_PATTERN_COUNT] = {
    /* FOUR — four-on-floor, snare on 2&4, offbeat open hats. */
    { .k = 0x1111, .s = 0x0404, .hc = 0xAAAA, .ho = 0x4444 },
    /* CC — broken Crystal-Castles groove: syncopated kick, clap on 2&4,
     * busy-but-gappy hats. (bit0 = step0 = beat 1.) */
    { .k = 0x0913, .s = 0x0440, .hc = 0xBBBA, .ho = 0x1010 },
    /* HALF — half-time heavy: kick on 1, big snare on 3, sparse hats. */
    { .k = 0x0021, .s = 0x0100, .hc = 0x8888, .ho = 0x0800 },
    /* DRIVE — driving: kick 1&3 + pickups, 16th hats, snare 2&4. */
    { .k = 0x1115, .s = 0x0404, .hc = 0xFFFF, .ho = 0x0000 },
};

static uint32_t xs(uint32_t *s) {
    uint32_t x = *s; x ^= x<<13; x ^= x>>17; x ^= x<<5;
    *s = x ? x : 0xA5A5F00Du; return *s;
}
static float noise(uint32_t *s) { return (float)xs(s) * (2.0f/4294967296.0f) - 1.0f; }

void beat_init(beat_t *b, uint32_t seed) {
    memset(b, 0, sizeof *b);
    b->rng = seed ? seed : 0xBEA70001u;
    b->pattern = BEAT_PATTERN_CC;
    b->amount = 0.0f;
    b->h_decay = 0.0006f;
    dsp_svf_reset(&b->s_bp);
    dsp_svf_set(&b->s_bp, 1800.0f, 1.2f);     /* snare body bandpass */
    dsp_svf_reset(&b->h_hp);
    dsp_svf_set(&b->h_hp, 7200.0f, 0.7f);     /* hat highpass */
}

void beat_set_pattern(beat_t *b, int p) {
    if (p < 0) p = 0;
    if (p >= BEAT_PATTERN_COUNT) p = BEAT_PATTERN_COUNT - 1;
    b->pattern = p;
}
void beat_set_amount(beat_t *b, float a) {
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    b->amount = a;
}

void beat_on_step(beat_t *b, int step) {
    if (b->amount <= 0.0001f) return;
    const pattern_t *p = &PATTERNS[b->pattern];
    int s = ((step % 16) + 16) % 16;
    uint16_t mask = (uint16_t)(1u << s);

    if (p->k & mask)  { b->k_amp = 1.0f; b->k_pitch = 1.0f; b->k_phase = 0.0f; }
    if (p->s & mask)  { b->s_amp = 1.0f; }
    if (p->ho & mask) { b->h_amp = 1.0f; b->h_decay = 0.0007f; }   /* open: ~195ms */
    else if (p->hc & mask) { b->h_amp = 1.0f; b->h_decay = 0.004f; } /* closed: ~40ms */
}

void beat_render_add(beat_t *b, float *L, float *R, float *sL, float *sR,
                     int frames, float send_scale)
{
    if (b->amount <= 0.0001f) return;

    const float k_amp_dec   = expf(-1.0f / (0.18f  * SR));   /* kick body ~180ms */
    const float k_pitch_dec = expf(-1.0f / (0.045f * SR));   /* pitch drop ~45ms */
    const float s_amp_dec   = expf(-1.0f / (0.13f  * SR));   /* snare ~130ms */

    float master = b->amount * 0.9f;

    for (int i = 0; i < frames; ++i) {
        /* ---- KICK ---- */
        float kick = 0.0f;
        if (b->k_amp > 0.0008f) {
            float f = 48.0f + 78.0f * b->k_pitch;       /* 126 → 48 Hz */
            b->k_phase += f / SR; if (b->k_phase >= 1.0f) b->k_phase -= 1.0f;
            float click = (b->k_amp > 0.85f) ? noise(&b->rng) * 0.25f * (b->k_amp - 0.85f) / 0.15f : 0.0f;
            kick = (dsp_sin(b->k_phase) + click) * b->k_amp;
            b->k_amp   *= k_amp_dec;
            b->k_pitch *= k_pitch_dec;
        }

        /* ---- SNARE/CLAP ---- */
        float snare = 0.0f;
        if (b->s_amp > 0.0008f) {
            float n = noise(&b->rng);
            float bp = dsp_svf_bp(&b->s_bp, n);
            b->s_tone_phase += 185.0f / SR; if (b->s_tone_phase >= 1.0f) b->s_tone_phase -= 1.0f;
            float body = dsp_sin(b->s_tone_phase) * 0.35f;
            snare = (bp * 0.9f + body) * b->s_amp;
            b->s_amp *= s_amp_dec;
        }

        /* ---- HAT ---- */
        float hat = 0.0f;
        if (b->h_amp > 0.0008f) {
            float n = noise(&b->rng);
            float hp = dsp_svf_hp(&b->h_hp, n);
            hat = hp * b->h_amp;
            b->h_amp *= (1.0f - b->h_decay);    /* h_decay = per-sample decay rate */
        }

        float dry = (kick * 0.95f + snare * 0.55f) * master;
        float hatL = hat * 0.40f * master;
        /* hats slightly right, snare centred, kick centred. */
        L[i]  += dry + hatL * 0.85f;
        R[i]  += dry + hatL * 1.15f;
        /* send: snare + hat tails into reverb, kick stays dry. */
        float snd = (snare * 0.55f + hat * 0.40f) * master * send_scale;
        sL[i] += snd;
        sR[i] += snd;
    }
}
