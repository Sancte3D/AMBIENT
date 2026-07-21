/*
 * leds.c — controls/modifier state → 16-channel PCA9685 PWM (see leds.h).
 *
 * Per-channel fade engine: each channel has a current 12-bit value that
 * ramps toward its target at LED_FADE_MS over the full 0↔TARGET span.
 * The target is recomputed every leds_render() from live state.
 */

#include "leds.h"
#include "controls.h"
#include "scenes.h"   /* r19.22: Scenes-Modus uebersteuert die Cell-LEDs */
#include "gesture.h"  /* r19.25: HOLD-LED zeigt den Gesten-Loop-Zustand */
#include <string.h>

/* Channel layout (matches generate_kicad_project.py:4459) */
#define CH_SHIFT     0
#define CH_HOLD      1
#define CH_DRONE     2
#define CH_GENERATE  3
#define CH_CLEAR     4
#define CH_CELL_Y(c) (5 + (c) * 2)        /* yellow LED of cell c (0..4) */
#define CH_CELL_G(c) (5 + (c) * 2 + 1)    /* green */
#define CH_BACKLIGHT 15

static uint16_t s_cur[LED_CH_COUNT];
static uint16_t s_backlight = 0;
static uint32_t s_clear_until = 0;

void leds_init(void) {
    memset(s_cur, 0, sizeof s_cur);
    s_backlight = 0;
    s_clear_until = 0;
}

void leds_clear_flash(uint32_t now_ms) {
    s_clear_until = now_ms + LED_CLEAR_FLASH_MS;
}

void leds_set_backlight(uint16_t pwm) {
    s_backlight = (pwm > LED_PWM_MAX) ? LED_PWM_MAX : pwm;
}

/* Ramp `cur` toward `target` by `step` (signed-clamped). */
static uint16_t ramp(uint16_t cur, uint16_t target, uint16_t step) {
    if (cur < target) {
        uint16_t need = (uint16_t)(target - cur);
        return (need <= step) ? target : (uint16_t)(cur + step);
    } else if (cur > target) {
        uint16_t need = (uint16_t)(cur - target);
        return (need <= step) ? target : (uint16_t)(cur - step);
    }
    return cur;
}

void leds_render(uint32_t now_ms, uint16_t dt_ms, uint16_t out[LED_CH_COUNT]) {
    /* Compute fade step for this tick. Full 0..LED_PWM_MAX ramp in LED_FADE_MS. */
    uint32_t step = ((uint32_t)LED_PWM_MAX * (uint32_t)dt_ms) / LED_FADE_MS;
    if (step == 0) step = 1;
    uint16_t step16 = (step > 0xFFFFu) ? 0xFFFFu : (uint16_t)step;

    /* ---- Targets from live state ----
     * Modifier LED colours per user spec (r18.64): Shift=yellow, Hold=green,
     * Generate/Drone/Clear=white. (This swaps Shift/Hold vs the earlier
     * convention that matched the modifier colour to its cell-hold LED;
     * the user spec takes precedence.) Clear is momentary (s_clear_until):
     * it flashes on click and goes dark; the others show persistent status. */
    uint16_t target[LED_CH_COUNT] = {0};
    target[CH_SHIFT]    = controls_modifier_active(MOD_SHIFT)    ? LED_DUTY_YELLOW : 0;
    /* r19.25: SHIFT+HOLD steuert den Gesten-Loop → die HOLD-LED zeigt ihn:
     * REC pulsiert (Dreieck ~800 ms), PLAY leuchtet ruhig, sonst normale
     * HOLD-Latch-Anzeige. */
    {
        gesture_state_t g = gesture_state();
        if (g == GESTURE_REC) {
            uint32_t ph = now_ms % 800u;
            target[CH_HOLD] = (uint16_t)(ph < 400u
                ? (uint32_t)LED_DUTY_GREEN * ph / 400u
                : (uint32_t)LED_DUTY_GREEN * (800u - ph) / 400u);
        } else if (g == GESTURE_PLAY) {
            target[CH_HOLD] = LED_DUTY_GREEN;
        } else {
            target[CH_HOLD] = controls_modifier_active(MOD_HOLD) ? LED_DUTY_GREEN : 0;
        }
    }
    target[CH_DRONE]    = controls_modifier_active(MOD_DRONE)    ? LED_DUTY_WHITE  : 0;
    target[CH_GENERATE] = controls_modifier_active(MOD_GENERATE) ? LED_DUTY_WHITE  : 0;
    target[CH_CLEAR]    = (now_ms < s_clear_until)               ? LED_DUTY_WHITE  : 0;

    if (scenes_ui_active()) {
        /* r19.22 Scenes-Modus: gelb = Slot belegt, gruen = aktive Scene —
         * die Latch-Anzeige pausiert, solange der Browser offen ist. */
        for (uint8_t c = 0; c < 5; ++c) {
            bool used   = scenes_used(c);
            bool active = used && scenes_active() == (int)c;
            target[CH_CELL_Y(c)] = (used && !active) ? LED_DUTY_YELLOW : 0;
            target[CH_CELL_G(c)] = active            ? LED_DUTY_GREEN  : 0;
        }
    } else {
        for (uint8_t c = 0; c < 5; ++c) {
            target[CH_CELL_Y(c)] = controls_hold_base(c)  ? LED_DUTY_YELLOW : 0;
            target[CH_CELL_G(c)] = controls_hold_shift(c) ? LED_DUTY_GREEN  : 0;
        }
    }
    target[CH_BACKLIGHT] = s_backlight;

    /* ---- Ramp each channel toward its target ---- */
    for (uint8_t ch = 0; ch < LED_CH_COUNT; ++ch) {
        s_cur[ch] = ramp(s_cur[ch], target[ch], step16);
        out[ch]   = s_cur[ch];
    }
}

uint16_t leds_get_channel(uint8_t ch) {
    return ch < LED_CH_COUNT ? s_cur[ch] : 0;
}
