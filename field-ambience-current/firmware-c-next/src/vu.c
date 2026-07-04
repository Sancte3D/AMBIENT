/*
 * vu.c — 8-segment output level meter (r18.85). See include/vu.h.
 *
 * All state is a handful of floats; vu_update() is a few dozen FLOPs at
 * 60 Hz — negligible. No allocation, no libc beyond math.h.
 */

#include "vu.h"
#include <math.h>

/* Segment thresholds in dBFS (see header). */
static const float SEG_DB[VU_CH_COUNT] = {
    -36.0f, -30.0f, -24.0f, -18.0f, -12.0f, -6.0f,   /* level segments */
    -3.0f, -0.5f                                     /* peak segments  */
};

static float    s_level_db;        /* current bar level (ballistic)     */
static int      s_peak_seg;        /* held peak segment, -1 = none      */
static uint32_t s_peak_since_ms;   /* when the held peak was (re)armed  */
static uint32_t s_last_ms;         /* previous vu_update timestamp      */
static int      s_have_last;       /* first-call flag                   */

void vu_init(void) {
    s_level_db      = VU_FLOOR_DB;
    s_peak_seg      = -1;
    s_peak_since_ms = 0;
    s_last_ms       = 0;
    s_have_last     = 0;
}

/* Highest segment index whose threshold is at or below `db`, or -1. */
static int seg_for_db(float db) {
    int seg = -1;
    for (int i = 0; i < VU_CH_COUNT; ++i)
        if (db >= SEG_DB[i]) seg = i;
    return seg;
}

void vu_update(float peak_lin, uint32_t now_ms) {
    /* --- release: wall-clock dB/s glide down --- */
    if (s_have_last) {
        uint32_t dt = now_ms - s_last_ms;
        if (dt > 250) dt = 250;                     /* clamp a stalled tick */
        s_level_db -= VU_DECAY_DB_PER_S * (float)dt * 0.001f;
        if (s_level_db < VU_FLOOR_DB) s_level_db = VU_FLOOR_DB;
    }
    s_last_ms   = now_ms;
    s_have_last = 1;

    /* --- attack: instant on a rising input --- */
    if (peak_lin > 0.0f) {
        float in_db = 20.0f * log10f(peak_lin);
        if (in_db > 0.0f) in_db = 0.0f;             /* limiter caps at FS   */
        if (in_db > s_level_db) s_level_db = in_db;
    }

    /* --- peak hold: track the highest lit segment --- */
    int cur_seg = seg_for_db(s_level_db);
    if (cur_seg > s_peak_seg) {
        s_peak_seg      = cur_seg;
        s_peak_since_ms = now_ms;
    } else if (s_peak_seg >= 0 &&
               (uint32_t)(now_ms - s_peak_since_ms) >= VU_PEAK_HOLD_MS) {
        /* hold expired: the peak dot falls onto the live bar */
        s_peak_seg = cur_seg;
        s_peak_since_ms = now_ms;
    }
}

void vu_render(uint16_t out[VU_CH_COUNT]) {
    for (int i = 0; i < VU_CH_COUNT; ++i) {
        float lo = SEG_DB[i];
        float hi = (i + 1 < VU_CH_COUNT) ? SEG_DB[i + 1] : 0.0f;
        uint16_t pwm;
        if (s_level_db >= hi) {
            pwm = VU_DUTY_WHITE;                    /* fully below the bar end */
        } else if (s_level_db > lo) {
            /* boundary segment: interpolate for a smooth bar end */
            float frac = (s_level_db - lo) / (hi - lo);
            pwm = (uint16_t)((float)VU_DUTY_WHITE * frac);
        } else {
            pwm = 0;
        }
        if (i == s_peak_seg && pwm < VU_DUTY_WHITE)
            pwm = VU_DUTY_WHITE;                    /* floating peak dot */
        out[i] = pwm;
    }
}

float vu_level_db(void)     { return s_level_db; }
int   vu_peak_segment(void) { return s_peak_seg; }
