#ifndef FAM_LEDS_H
#define FAM_LEDS_H

/*
 * leds.c — controls/modifier state → PCA9685 16-channel PWM values.
 *
 * Hardware-independent. The STM32 main loop ticks this at ~60 Hz; on each
 * tick leds_render() reads the live controls/modifier state, runs the
 * brightness/fade math, and writes a snapshot of 16 PWM values
 * (0…LED_PWM_MAX) into the output buffer. The HAL (mcp23017_h743.c) then
 * pushes that buffer to the PCA9685 over I²C in one burst.
 *
 * PCA9685 channel map (SPEC §7.2, ADR-0008 r2 — verified against
 * generate_kicad_project.py:4459):
 *
 *   ch 0  → LED6  SHIFT-Modifier   (green)   gated by controls_modifier_active(MOD_SHIFT)
 *   ch 1  → LED7  HOLD-Modifier    (yellow)  gated by controls_modifier_active(MOD_HOLD)
 *   ch 2  → LED8  DRONE-Modifier   (white)   gated by controls_modifier_active(MOD_DRONE)
 *   ch 3  → LED9  GENERATE-Modifier(white)   gated by controls_modifier_active(MOD_GENERATE)
 *   ch 4  → LED10 CLEAR-Confirm    (white)   flashes 250 ms on Clear press
 *   ch 5  → LED11Y CELL1 Hold@Base (yellow)  controls_hold_base(0)
 *   ch 6  → LED11G CELL1 Hold@Shift(green)   controls_hold_shift(0)
 *   ch 7  → LED12Y CELL2 Hold@Base (yellow)  controls_hold_base(1)
 *   ch 8  → LED12G CELL2 Hold@Shift(green)   controls_hold_shift(1)
 *   ch 9  → LED13Y CELL3 Hold@Base (yellow)
 *   ch 10 → LED13G CELL3 Hold@Shift(green)
 *   ch 11 → LED14Y CELL4 Hold@Base (yellow)
 *   ch 12 → LED14G CELL4 Hold@Shift(green)
 *   ch 13 → LED15Y CELL5 Hold@Base (yellow)
 *   ch 14 → LED15G CELL5 Hold@Shift(green)
 *   ch 15 → LCD_BLK_PWM (gates Q2 2N7002 → LCD backlight)
 *
 * Fade behaviour: a state change (e.g. Hold-bit on) ramps the channel's PWM
 * from 0 → target over LED_FADE_MS, and 0-out ramps target → 0 over the
 * same time. Yellow/green per-colour brightness compensates for the Vf
 * difference (green Vf 3.1 V → less current at 5 V → bump duty).
 */

#include <stdint.h>
#include <stdbool.h>

#define LED_CH_COUNT     16
#define LED_PWM_MAX      4095     /* PCA9685 is 12-bit */
#define LED_FADE_MS      120      /* on/off fade duration */
#define LED_CLEAR_FLASH_MS 250    /* CLEAR LED flash-on-press duration */

/* Per-colour duty targets (perceptual matching at 5 V / 390 Ω):
 *   yellow Vf 2.4 V → I = 6.7 mA → 70 % duty
 *   green  Vf 3.1 V → I = 4.9 mA → 100 % duty (already dim)
 *   white  Vf 3.0 V → I = 5.1 mA → 80 % duty */
#define LED_DUTY_YELLOW  (LED_PWM_MAX * 70 / 100)
#define LED_DUTY_GREEN   (LED_PWM_MAX * 100 / 100)
#define LED_DUTY_WHITE   (LED_PWM_MAX * 80 / 100)

/* Reset. Call once after engine/controls init. */
void leds_init(void);

/* Trigger the Clear-confirm flash on channel 4 (LED10). Call from the
 * MOD_CLEAR press edge. */
void leds_clear_flash(uint32_t now_ms);

/* Set the LCD backlight target (0..LED_PWM_MAX). Independent of fade —
 * the backlight has its own smoother because it's user-driven via the
 * Shift+Display encoder. */
void leds_set_backlight(uint16_t pwm);

/* Render: read the live controls/modifier state, advance the fade math by
 * `dt_ms` ticks, and write the 16 PWM values into `out`. Caller pushes
 * `out` to the PCA9685. */
void leds_render(uint32_t now_ms, uint16_t dt_ms, uint16_t out[LED_CH_COUNT]);

/* Read-back for tests/UI. */
uint16_t leds_get_channel(uint8_t ch);

#endif
