/*
 * Host test for the LED render module (leds.c) — verifies channel mapping
 * + fade-engine semantics against the controls/modifier state.
 */
#include <stdio.h>
#include <stdint.h>
#include "leds.h"
#include "controls.h"
#include "engine.h"
#include "brain.h"
#include "dsp.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

/* Advance the fade engine until channels settle (or `max_ms` elapse). */
static void settle(uint32_t *now, uint16_t out[LED_CH_COUNT], int max_ms) {
    for (int i = 0; i < max_ms; i += 8) {
        leds_render(*now, 8, out);
        *now += 8;
    }
}

int main(void) {
    printf("== leds (state → PCA9685 channels) ==\n");
    dsp_init(); brain_init(); engine_init(); controls_init(); leds_init();

    uint16_t out[LED_CH_COUNT];
    uint32_t now = 1000;

    /* ---- 1. All channels start at 0 ---- */
    leds_render(now, 8, out);
    for (int ch = 0; ch < LED_CH_COUNT; ++ch) CHECK(out[ch] == 0, "ch %d boots dark", ch);

    /* ---- 2. SHIFT latch lights ch 0 (green target) ---- */
    controls_modifier(MOD_SHIFT, true);
    settle(&now, out, 200);
    CHECK(out[0] == LED_DUTY_GREEN, "SHIFT lit on ch 0 (%d)", out[0]);
    CHECK(out[1] == 0, "HOLD still off");

    /* ---- 3. HOLD latch lights ch 1 (yellow), SHIFT stays ---- */
    controls_modifier(MOD_HOLD, true);
    settle(&now, out, 200);
    CHECK(out[0] == LED_DUTY_GREEN,  "SHIFT still lit");
    CHECK(out[1] == LED_DUTY_YELLOW, "HOLD lit on ch 1");

    /* ---- 4. Cell 2 base hold lights ch 9 (yellow), shift bit lights ch 10 ---- */
    /* Clear shift first (it latched on in test 2). */
    controls_modifier(MOD_SHIFT, true);             /* shift now OFF */
    controls_cell_press(2, 0.15f);                  /* base on */
    controls_modifier(MOD_SHIFT, true);             /* shift ON */
    controls_cell_press(2, 0.15f);                  /* shift bit on too */
    settle(&now, out, 200);
    CHECK(out[5 + 2*2]     == LED_DUTY_YELLOW, "Cell 2 base → ch 9 yellow (%d)", out[5+2*2]);
    CHECK(out[5 + 2*2 + 1] == LED_DUTY_GREEN,  "Cell 2 shift → ch 10 green (%d)", out[5+2*2+1]);

    /* ---- 5. Octave stack: both LEDs lit simultaneously (ADR-0008 r2) ---- */
    CHECK(out[5+4] > 0 && out[5+5] > 0, "Cell 2 both LEDs on at once (stack)");

    /* ---- 6. Clear flash on ch 4, decays after LED_CLEAR_FLASH_MS ---- */
    leds_clear_flash(now);
    leds_render(now, 8, out); now += 8;
    leds_render(now, 8, out); now += 8;
    CHECK(out[4] > 0, "Clear flash on ch 4");
    now += LED_CLEAR_FLASH_MS + 200;                /* past flash + decay window */
    settle(&now, out, 200);
    CHECK(out[4] == 0, "Clear flash decays (now %d)", out[4]);

    /* ---- 7. Backlight set independently → ch 15 ---- */
    leds_set_backlight(2000);
    settle(&now, out, 200);
    CHECK(out[15] == 2000, "backlight → ch 15 (%d)", out[15]);

    /* ---- 8. Fade-OUT: clear latches → channel ramps back to 0 ---- */
    controls_modifier(MOD_CLEAR, true);             /* wipes all holds */
    settle(&now, out, 400);                         /* > LED_FADE_MS */
    CHECK(out[5+4] == 0 && out[5+5] == 0, "Cell 2 LEDs fade to 0 after Clear");

    /* ---- 9. Mid-fade: 60 ms into a 120 ms fade → roughly half value ---- */
    leds_init();
    controls_init();
    controls_modifier(MOD_DRONE, true);             /* turn on */
    now = 50000;
    leds_render(now, 60, out);
    int mid = out[2];                                /* ch 2 = DRONE */
    CHECK(mid > 0 && mid < LED_DUTY_WHITE,
          "Drone mid-fade is in-between (%d, target %d)", mid, LED_DUTY_WHITE);

    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
