/*
 * design_bench — drive the radial navigation system on a real ST7789, live.
 *
 * Bench tool for the Pico-2 breadboard, NOT a product build. Its job is to
 * answer what no desktop preview can: at 39.1 x 21.2 mm of glass, does the
 * wheel read while turning, do the icons survive at 22 px, and does the snap
 * feel like a detent or like a lag.
 *
 * ARCHITECTURE. There is no framebuffer and no background asset. Each row is
 * composited into a 640-byte line buffer by tools/ui_wheel.c and pushed
 * straight to SPI, so the whole UI costs two line buffers instead of a 106 KB
 * colour framebuffer — the H743 sits at 87 % RAM_D1 and 96 % RAM_D2 against
 * 11 % flash, so that split is not optional.
 *
 * INPUT IS INTERRUPT-DRIVEN, AND THAT IS THE POINT. A full frame takes 29 ms
 * on the wire at 32 MHz. Polling the encoder in the main loop means every
 * detent that arrives during a blit is simply lost, which on a fast turn drops
 * roughly every other step — the exact "latency fail" that makes an instrument
 * feel cheap. The GPIO IRQ records detents while the panel is being written,
 * the loop drains them, and the model moves before the next frame is drawn.
 *
 * Motion follows tools/ui_motion.h: the model is updated the instant the edge
 * arrives, only the presentation eases, and nothing is ever queued.
 *
 * CONTROLS (single encoder + one button, same wiring as display_hw_test)
 *   rotate          MAIN/GROUP: rotate the structure under the 12 o'clock
 *                   selection point.  VALUE: move the value.
 *   press           descend: group -> parameter -> value
 *   HOLD (350 ms)   climb back out of the level you are in
 *   SHIFT + rotate  coarse steps while editing a value
 */

#include "ui_wheel.h"
#include "oled.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include <stdio.h>

/* ---- pins (mirrors display_hw_test so the same breadboard works) --------- */
#ifndef PIN_ENC_CLK
#define PIN_ENC_CLK 2
#endif
#ifndef PIN_ENC_DT
#define PIN_ENC_DT  3
#endif
#ifndef PIN_ENC_SW
#define PIN_ENC_SW  4
#endif
#ifndef PIN_SHIFT
#define PIN_SHIFT   5
#endif
#ifndef PIN_BL
#define PIN_BL     22
#endif

#define HOLD_MS       350       /* press vs hold */
#define DEBOUNCE_US  1500

/* ---- panel -------------------------------------------------------------- */
extern void lcd_set_window_full(void);        /* from lcd_st7789_pico.c */
extern void lcd_stream_begin(void);
extern void lcd_stream_row(const uint8_t *row, size_t n);
extern void lcd_stream_end(void);

static wheel_state_t ui;

static void present(void)
{
    static uint16_t line[OLED_WIDTH];
    static uint8_t  out[OLED_WIDTH * 2];
    lcd_set_window_full();
    lcd_stream_begin();
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        ui_wheel_compose_row(&ui, y, line);
        for (int x = 0; x < OLED_WIDTH; ++x) {      /* ST7789 wants MSB first */
            out[x * 2]     = (uint8_t)(line[x] >> 8);
            out[x * 2 + 1] = (uint8_t)line[x];
        }
        lcd_stream_row(out, sizeof out);
    }
    lcd_stream_end();
}

/* ---- interrupt-driven input --------------------------------------------- */
static volatile int32_t  enc_delta;         /* detents not yet consumed */
static volatile bool     sw_held;
static volatile uint32_t sw_down_us;
static volatile bool     sw_short_evt;
static volatile bool     sw_long_fired;

static void on_gpio(uint gpio, uint32_t events)
{
    static uint32_t last_clk_us, last_sw_us;
    uint32_t now = time_us_32();

    if (gpio == PIN_ENC_CLK && (events & GPIO_IRQ_EDGE_FALL)) {
        if (now - last_clk_us < DEBOUNCE_US) return;
        last_clk_us = now;
        enc_delta += gpio_get(PIN_ENC_DT) ? +1 : -1;
        return;
    }

    if (gpio == PIN_ENC_SW) {
        if (now - last_sw_us < DEBOUNCE_US) return;
        last_sw_us = now;
        if (events & GPIO_IRQ_EDGE_FALL) {
            sw_held       = true;
            sw_down_us    = now;
            sw_long_fired = false;
        } else if (events & GPIO_IRQ_EDGE_RISE) {
            /* A hold has already acted on the way down; releasing must not
             * then also count as a press. */
            if (sw_held && !sw_long_fired) sw_short_evt = true;
            sw_held = false;
        }
    }
}

static void gpio_in_pullup(uint pin)
{
    gpio_init(pin); gpio_set_dir(pin, GPIO_IN); gpio_pull_up(pin);
}

int main(void)
{
    stdio_init_all();

    gpio_in_pullup(PIN_ENC_CLK);
    gpio_in_pullup(PIN_ENC_DT);
    gpio_in_pullup(PIN_ENC_SW);
    gpio_in_pullup(PIN_SHIFT);

    gpio_set_irq_enabled_with_callback(PIN_ENC_CLK, GPIO_IRQ_EDGE_FALL,
                                       true, &on_gpio);
    gpio_set_irq_enabled(PIN_ENC_SW, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                         true);

    gpio_set_function(PIN_BL, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_BL);
    pwm_set_wrap(slice, 255);
    pwm_set_gpio_level(PIN_BL, 220);
    pwm_set_enabled(slice, true);

    oled_init();
    ui_wheel_init(&ui);
    ui_wheel_settle(&ui);
    present();

    absolute_time_t last_frame = get_absolute_time();

    for (;;) {
        bool dirty = false;
        bool shift = !gpio_get(PIN_SHIFT);

        /* Drain everything the IRQ collected, including whatever arrived
         * during the last 29 ms blit. Each detent is applied to the model in
         * full; only the drawing lags. */
        int32_t d;
        do { d = enc_delta; } while (d && !__atomic_compare_exchange_n(
                 (int32_t *)&enc_delta, &d, 0, false,
                 __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
        for (int32_t i = 0; i < d; ++i)  { ui_wheel_turn(&ui, +1, shift); dirty = true; }
        for (int32_t i = 0; i > d; --i)  { ui_wheel_turn(&ui, -1, shift); dirty = true; }

        if (sw_short_evt) { sw_short_evt = false; ui_wheel_press(&ui, 0); dirty = true; }

        /* Hold acts on the way DOWN, at the threshold — waiting for the
         * release would make the exit feel later than the gesture. */
        if (sw_held && !sw_long_fired &&
            (time_us_32() - sw_down_us) > HOLD_MS * 1000u) {
            sw_long_fired = true;
            ui_wheel_press(&ui, 1);
            dirty = true;
        }

        absolute_time_t now = get_absolute_time();
        int dt = (int)(absolute_time_diff_us(last_frame, now) / 1000);
        if (dt < 1) dt = 1;
        last_frame = now;

        if (ui_wheel_tick(&ui, dt)) dirty = true;

        if (dirty) present();
        else       sleep_ms(2);
    }
}
