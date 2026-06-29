#ifndef FAM_DSP_LADDER_H
#define FAM_DSP_LADDER_H

/*
 * dsp_ladder — 4-pole Huovilainen "New Moog" transistor-ladder filter.
 *
 * C port of DaisySP's LadderFilter, itself ported from the Teensy Audio Library
 * ladder filter. Retained under the MIT license (see dsp_ladder.c for the full
 * original copyright/permission notice — required by the license).
 *
 * This is the real thing a plain SVF can't be: nonlinear (tanh) 4-pole ladder
 * with drive, passband-gain compensation and stable self-oscillation — i.e.
 * the squelchy, singing acid/Moog character. 4× oversampled internally.
 *
 * Use at control rate for set_freq/res (cheap) but process() is per-sample.
 */

typedef struct {
    float sample_rate, sr_int_recip;
    float alpha;
    float z0[4], z1[4];
    float K, Qadjust, pbg;
    float drive, drive_scaled, oldinput;
} dsp_ladder_t;

void  dsp_ladder_init(dsp_ladder_t *f, float sample_rate);
void  dsp_ladder_set_freq(dsp_ladder_t *f, float hz);        /* 5 .. ~0.425*sr */
void  dsp_ladder_set_res(dsp_ladder_t *f, float res);        /* 0 .. 1.8 (self-osc high) */
void  dsp_ladder_set_drive(dsp_ladder_t *f, float drv);      /* 0 .. 4.0 */
float dsp_ladder_process(dsp_ladder_t *f, float in);

#endif /* FAM_DSP_LADDER_H */
