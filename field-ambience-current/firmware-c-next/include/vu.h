#ifndef FAM_VU_H
#define FAM_VU_H

/*
 * vu.c — 8-segment output level meter (r18.85, hardware r18.66: U10
 * PCA9685 @ 0x41, 8 white LEDs in a row, OP-1-Field style).
 *
 * Hardware-independent, host-tested. The main loop ticks this at the same
 * ~60 Hz cadence as leds_render():
 *
 *     vu_update(engine_render_peak(), now_ms);
 *     uint16_t pwm[VU_CH_COUNT];
 *     vu_render(pwm);
 *     for (ch = 0; ch < VU_CH_COUNT; ++ch) pca2_set_pwm(ch, 0, pwm[ch]);
 *
 * Ballistics (classic peak-programme feel, tuned for an ambient device —
 * slow enough to breathe, fast enough to still look "live"):
 *   - attack: instant (a rising peak snaps the bar up in one tick)
 *   - release: VU_DECAY_DB_PER_S dB/s glide down
 *   - peak hold: the highest lit segment stays lit VU_PEAK_HOLD_MS, then
 *     falls with the bar (OP-1-Field-style floating peak dot)
 *
 * Segment thresholds (dBFS of the post-limiter engine output; the limiter
 * knee sits at 0.75 FS ≈ -2.5 dBFS, so the top segment ≈ "you're touching
 * the limiter"):
 *   seg 1..6 (level): -36, -30, -24, -18, -12, -6
 *   seg 7..8 (peak):  -3, -0.5
 *
 * Segments below the current level are fully lit; the boundary segment is
 * PWM-interpolated (smooth bar end, no hard stepping); the held peak
 * segment is lit at full white duty.
 */

#include <stdint.h>

#define VU_CH_COUNT        8
#define VU_PWM_MAX         4095            /* PCA9685 12-bit */
#define VU_DUTY_WHITE      (VU_PWM_MAX * 80 / 100)  /* match LED_DUTY_WHITE */
#define VU_DECAY_DB_PER_S  30.0f
#define VU_PEAK_HOLD_MS    900
#define VU_FLOOR_DB        (-42.0f)        /* below this the bar is dark */

/* Reset all meter state (bar at floor, no held peak). */
void vu_init(void);

/* Feed the most recent block peak (0..1 linear, e.g. engine_render_peak())
 * and advance the ballistics to `now_ms`. Call at ~60 Hz; irregular tick
 * spacing is fine (decay is wall-clock-based). */
void vu_update(float peak_lin, uint32_t now_ms);

/* Render the 8 segment PWM values (0..VU_PWM_MAX) for U10 channels 0..7.
 * Pure function of the state left by vu_update(). */
void vu_render(uint16_t out[VU_CH_COUNT]);

/* Test/introspection helpers. */
float vu_level_db(void);      /* current bar level in dBFS (>= VU_FLOOR_DB) */
int   vu_peak_segment(void);  /* held peak segment index 0..7, or -1 */

#endif
