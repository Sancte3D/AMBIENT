/*
 * tuning.c — equal / just intonation layer. See tuning.h.
 */

#include "tuning.h"
#include "dsp.h"
#include <math.h>

/* 5-limit just-intonation ratios for the 12 chromatic steps above the
 * tonic. The pentatonic-safe intervals this instrument actually uses are
 * the pure ones: 9/8 (2nd), 6/5 & 5/4 (3rds), 4/3 (4th), 3/2 (5th),
 * 5/3 (6th). The rest are filled with standard 5-limit choices so any
 * note stays sane if the harmony wanders. */
static const float JI[12] = {
    1.0f,          /* 0  unison   1/1  */
    16.0f/15.0f,   /* 1  min2          */
    9.0f/8.0f,     /* 2  maj2          */
    6.0f/5.0f,     /* 3  min3          */
    5.0f/4.0f,     /* 4  maj3          */
    4.0f/3.0f,     /* 5  P4            */
    45.0f/32.0f,   /* 6  tritone       */
    3.0f/2.0f,     /* 7  P5            */
    8.0f/5.0f,     /* 8  min6          */
    5.0f/3.0f,     /* 9  maj6          */
    9.0f/5.0f,     /* 10 min7          */
    15.0f/8.0f,    /* 11 maj7          */
};

static int s_just  = 0;
static int s_tonic = 60;     /* C4 default */

void tuning_set_mode(int just)      { s_just = just ? 1 : 0; }
int  tuning_mode(void)              { return s_just; }
void tuning_set_key(int tonic_midi) { s_tonic = tonic_midi; }

float tuning_hz(float midi) {
    if (!s_just) return dsp_midi_to_hz(midi);   /* bit-exact ET */

    /* Interval from the tonic, split into octave + chromatic step. The
     * tonic itself keeps its ET frequency, so the instrument stays at
     * standard concert pitch; everything else is a pure ratio from it. */
    float d      = midi - (float)s_tonic;
    float fstep  = floorf(d);
    float frac   = d - fstep;                   /* keep any micro-tuning */
    int   di     = (int)fstep;
    int   oct    = di / 12;
    int   step   = di % 12;
    if (step < 0) { step += 12; --oct; }        /* floor-mod             */

    float tonic_hz = dsp_midi_to_hz((float)s_tonic);
    float ratio    = JI[step] * powf(2.0f, (float)oct);
    /* apply any fractional cents on top (pitch jitter etc.) in ET terms */
    return tonic_hz * ratio * powf(2.0f, frac / 12.0f);
}
