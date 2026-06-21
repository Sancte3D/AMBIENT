#ifndef FAM_V2_DIFFUSER_H
#define FAM_V2_DIFFUSER_H

/*
 * diffuser.c — short Allpass-Kette vor dem Reverb (ADR-0014 §9).
 *
 *   7 / 11 / 17 / 23 ms cascade per channel.
 *   Feedback 0.35 .. 0.55  (Blur macro 0..1 maps in).
 *
 * Wandelt harte Akkordkanten in eine Wolke, ohne Reverb-Größe ändern zu
 * müssen. Stereo-Versatz auf der R-Kette (+3 / +5 / +7 / +11 Samples
 * Decorrelation) wie Schroeder.
 */

#include <stdint.h>

#define DF_TAPS 4

typedef struct {
    /* 23 ms @ 44.1k = 1014 samples → 1024 ring (power-of-2 mask). */
    float bufL[DF_TAPS][1024];
    float bufR[DF_TAPS][1024];
    int   writeL[DF_TAPS];
    int   writeR[DF_TAPS];
    int   delayL[DF_TAPS];
    int   delayR[DF_TAPS];
    float fb;
} diffuser_t;

void diffuser_init(diffuser_t *d);
void diffuser_set_amount(diffuser_t *d, float blur_0_1);
void diffuser_process(diffuser_t *d, float *L, float *R, int frames);

#endif
