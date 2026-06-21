/*
 * arp.c — pentatonic bell arpeggiator. See arp.h.
 */

#include "v2/arp.h"
#include "v2/harmony_field.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR 44100.0f

static uint32_t xs32(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *s = x ? x : 0xB16B00B5u;
    return *s;
}
static float urand_a(uint32_t *s) { return (float)xs32(s) * (1.0f / 4294967296.0f); }

/* Rising arpeggio pattern over pentatonic step indices, spanning ~2 octaves.
 * (0,1,2,3,4 = scale degrees; +5/+10 = octave up.) A gentle up shape with a
 * couple of returns so it sings rather than climbs monotonically. */
static const int PATTERN[] = { 0, 2, 4, 5, 7, 9, 7, 5, 4, 2, 5, 9, 12, 9, 7, 4 };
#define PATTERN_LEN (int)(sizeof PATTERN / sizeof PATTERN[0])

void arp_init(arp_t *a, uint32_t seed) {
    memset(a, 0, sizeof *a);
    a->rng = seed ? seed : 0x1234ABCDu;
    a->rate_hz = 1.6f;
    a->amount = 0.0f;
    a->division = 2;
    a->dir = 1;
    a->pattern_pos = 0;
}

void arp_set_rate(arp_t *a, float hz) {
    if (hz < 0.0f) hz = 0.0f;
    if (hz > 12.0f) hz = 12.0f;
    a->rate_hz = hz;
}

void arp_set_division(arp_t *a, int steps) {
    if (steps < 1) steps = 1;
    if (steps > 16) steps = 16;
    a->division = steps;
}
void arp_set_amount(arp_t *a, float amt) {
    if (amt < 0.0f) amt = 0.0f;
    if (amt > 1.0f) amt = 1.0f;
    a->amount = amt;
}

/* Allocate a note slot (steal the quietest if full). */
static arp_note_t *alloc_note(arp_t *a) {
    int best = 0;
    float lowest = 1e9f;
    for (int i = 0; i < ARP_POLY; ++i) {
        if (!a->notes[i].active) return &a->notes[i];
        float e = a->notes[i].env * a->notes[i].amp;
        if (e < lowest) { lowest = e; best = i; }
    }
    return &a->notes[best];
}

static void trigger(arp_t *a, int pattern_index) {
    int n = hf_scale_len();
    int step = PATTERN[pattern_index % PATTERN_LEN];
    /* step is a pentatonic position (degree + octave*n encoded as raw step). */
    int pos = step;                 /* PATTERN entries already are scale steps */
    /* place arp an octave above the root register for a bell shimmer. */
    int midi = hf_scale_pos_to_midi(pos + n);   /* +1 octave */
    float freq = 440.0f * powf(2.0f, (midi - 69) / 12.0f);

    arp_note_t *nt = alloc_note(a);
    nt->freq = freq;
    nt->phase1 = urand_a(&a->rng);
    nt->phase2 = urand_a(&a->rng);
    nt->phase3 = urand_a(&a->rng);
    nt->env = 1.0f;
    nt->rise = 0.0f;
    /* Velocity varies gently for a human feel. */
    nt->amp = 0.55f + 0.45f * urand_a(&a->rng);
    /* Pan follows pitch a little (higher notes drift right). */
    nt->pan = -0.4f + 0.8f * ((float)(pattern_index % PATTERN_LEN) / (float)PATTERN_LEN);
    nt->active = 1;
}

void arp_tick(arp_t *a, float dt_s) {
    if (a->amount <= 0.0001f || a->rate_hz <= 0.0001f) return;
    a->step_phase += a->rate_hz * dt_s;
    while (a->step_phase >= 1.0f) {
        a->step_phase -= 1.0f;
        /* Occasionally skip a step for breathing room. */
        if (urand_a(&a->rng) < 0.82f) {
            trigger(a, a->pattern_pos);
        }
        a->pattern_pos = (a->pattern_pos + 1) % PATTERN_LEN;
    }
}

void arp_on_step(arp_t *a, int global_step) {
    if (a->amount <= 0.0001f) return;
    if (a->division < 1) a->division = 1;
    if ((global_step % a->division) != 0) return;
    /* Slight gap for breathing room — denser than free-run since it's gridded. */
    if (urand_a(&a->rng) < 0.88f) trigger(a, a->pattern_pos);
    a->pattern_pos = (a->pattern_pos + 1) % PATTERN_LEN;
}

void arp_render_add(arp_t *a, float *L, float *R, float *sL, float *sR,
                    int frames, float color, float send_scale)
{
    if (a->amount <= 0.0001f) return;
    if (color < 0.0f) color = 0.0f;
    if (color > 1.0f) color = 1.0f;

    /* Decay time grows with lower color (darker = longer felt tail). */
    float decay_s = 1.6f + (1.0f - color) * 1.8f;   /* 1.6 .. 3.4 s */
    float env_coef = expf(-1.0f / (decay_s * SR));

    /* Attack softness: short rise so it's a bell, not a click. */
    const float atk_coef = 1.0f - expf(-1.0f / (0.006f * SR));

    float master = a->amount * 0.35f;

    for (int i = 0; i < ARP_POLY; ++i) {
        arp_note_t *nt = &a->notes[i];
        if (!nt->active) continue;

        float gL = 0.5f * (1.0f - nt->pan);
        float gR = 0.5f * (1.0f + nt->pan);
        float dt1 = nt->freq / SR;
        float dt2 = nt->freq * 2.0f / SR;
        float dt3 = nt->freq * 3.01f / SR;   /* faint inharmonic 12th */

        for (int s = 0; s < frames; ++s) {
            nt->phase1 += dt1; if (nt->phase1 >= 1.0f) nt->phase1 -= 1.0f;
            nt->phase2 += dt2; if (nt->phase2 >= 1.0f) nt->phase2 -= 1.0f;
            nt->phase3 += dt3; if (nt->phase3 >= 1.0f) nt->phase3 -= 1.0f;

            float o1 = dsp_sin(nt->phase1);
            float o2 = dsp_sin(nt->phase2) * (0.35f + 0.25f * color);
            float o3 = dsp_sin(nt->phase3) * 0.12f * color;
            float tone = (o1 + o2 + o3) * 0.7f;

            /* Smooth attack toward 1.0 then exponential decay (rise persists
             * across blocks via the note struct → no per-block re-attack). */
            nt->rise += atk_coef * (1.0f - nt->rise);
            nt->env *= env_coef;
            float e = nt->env * nt->rise;

            float v = tone * e * nt->amp * master;
            L[s]  += v * gL;
            R[s]  += v * gR;
            sL[s] += v * gL * send_scale;
            sR[s] += v * gR * send_scale;
        }

        if (nt->env < 0.0008f) nt->active = 0;
    }
}
