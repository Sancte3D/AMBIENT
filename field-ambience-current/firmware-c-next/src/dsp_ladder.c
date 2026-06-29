/*
 * dsp_ladder.c — 4-pole Huovilainen "New Moog" ladder filter (C port).
 *
 * Ported to C from DaisySP's LadderFilter (Source/Filters/ladder.{h,cpp}),
 * which is itself ported from the Teensy Audio Library ladder filter. Algorithm
 * and coefficients unchanged; only translated C++ → C for this codebase.
 *
 * Original copyright / license (retained as required by the MIT license):
 *
 *   Ported from Audio Library for Teensy, Ladder Filter
 *   Copyright (c) 2021, Richard van Hoesel
 *   Copyright (c) 2024, Infrasonic Audio LLC
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice, development funding notice, and this permission
 *   notice shall be included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND ...
 *
 *   Huovilainen New Moog (HNM) model as per CMJ jun 2006,
 *   Richard van Hoesel, v.1.03 / Infrasonic-Daisy v1.7.
 *   please retain this header if you use this code.
 */
#include "dsp_ladder.h"
#include "dsp.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define LAD_OS       4              /* 4× oversampling */
#define LAD_OS_RECIP 0.25f
#define LAD_MAX_RES  1.8f

static inline float fast_tanh(float x) {
    if (x >  3.0f) return  1.0f;
    if (x < -3.0f) return -1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

static void compute_coeffs(dsp_ladder_t *f, float freq) {
    freq = dsp_clampf(freq, 5.0f, f->sample_rate * 0.425f);
    float wc  = freq * 2.0f * (float)M_PI * f->sr_int_recip;
    float wc2 = wc * wc;
    f->alpha   = 0.9892f * wc - 0.4324f * wc2 + 0.1381f * wc * wc2
               - 0.0202f * wc2 * wc2;
    f->Qadjust = 1.006f + 0.0536f * wc - 0.095f * wc2 - 0.05f * wc2 * wc2;
}

void dsp_ladder_set_drive(dsp_ladder_t *f, float drv) {
    f->drive = drv < 0.0f ? 0.0f : drv;
    if (f->drive > 1.0f) {
        if (f->drive > 4.0f) f->drive = 4.0f;
        f->drive_scaled = 1.0f + (f->drive - 1.0f) * (1.0f - f->pbg);
    } else {
        f->drive_scaled = f->drive;
    }
}

void dsp_ladder_set_freq(dsp_ladder_t *f, float hz) { compute_coeffs(f, hz); }

void dsp_ladder_set_res(dsp_ladder_t *f, float res) {
    f->K = 4.0f * dsp_clampf(res, 0.0f, LAD_MAX_RES);   /* res 0..1.8 → K 0..7.2 */
}

void dsp_ladder_init(dsp_ladder_t *f, float sample_rate) {
    for (int i = 0; i < 4; ++i) { f->z0[i] = 0.0f; f->z1[i] = 0.0f; }
    f->sample_rate  = sample_rate;
    f->sr_int_recip = 1.0f / (sample_rate * LAD_OS);
    f->alpha = 1.0f; f->K = 1.0f; f->Qadjust = 1.0f;
    f->oldinput = 0.0f;
    f->pbg = 0.5f;
    dsp_ladder_set_drive(f, 0.5f);
    dsp_ladder_set_freq(f, 5000.0f);
    dsp_ladder_set_res(f, 0.2f);
}

static inline float lpf(dsp_ladder_t *f, float s, int i) {
    float ft = s * 0.76923077f + 0.23076923f * f->z0[i] - f->z1[i];
    ft       = ft * f->alpha + f->z1[i];
    f->z1[i] = ft;
    f->z0[i] = s;
    return ft;
}

float dsp_ladder_process(dsp_ladder_t *f, float in) {
    float input  = in * f->drive_scaled;
    float total  = 0.0f;
    float interp = 0.0f;
    for (int os = 0; os < LAD_OS; ++os) {
        float in_interp = interp * f->oldinput + (1.0f - interp) * input;
        float u = in_interp - (f->z1[3] - f->pbg * in_interp) * f->K * f->Qadjust;
        u = fast_tanh(u);
        float s1 = lpf(f, u,  0);
        float s2 = lpf(f, s1, 1);
        float s3 = lpf(f, s2, 2);
        float s4 = lpf(f, s3, 3);
        total  += s4 * LAD_OS_RECIP;          /* LP24 (classic Moog 4-pole) */
        interp += LAD_OS_RECIP;
    }
    f->oldinput = input;
    return total;
}
