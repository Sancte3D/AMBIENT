#ifndef FAM_V2_MOD_DELAY_H
#define FAM_V2_MOD_DELAY_H

/*
 * mod_delay.c — Stereo-Delay mit moduliertem Tap (ADR-0014 §9 Mod Delay).
 *
 *   Left  base: 280..420 ms
 *   Right base: 370..610 ms
 *   Feedback : 0.15..0.35
 *   Wow LFO  : 0.05..0.15 Hz, depth ±10..30 samples
 *
 * Wozu: erzeugt Bewegung ohne Beat. Sehr leise, Wet-Anteil < 30%. Sitzt
 * NACH dem Diffuser und VOR dem Reverb in der Engine-V2-Kette.
 */

#include <stdint.h>

#define MD_BUF 32768                /* ~743 ms @ 44.1k, power-of-two */

typedef struct {
    float bufL[MD_BUF];
    float bufR[MD_BUF];
    int   write;
    float lfo_L_phase, lfo_R_phase;
    float base_L_samples, base_R_samples;
    float depth_samples;
    float fb;
    float wet;                     /* 0..1, dry kept at 1.0 (parallel mix) */
} mod_delay_t;

void mod_delay_init(mod_delay_t *md);
void mod_delay_set_amount(mod_delay_t *md, float blur_0_1);
void mod_delay_process(mod_delay_t *md, float *L, float *R, int frames);

#endif
