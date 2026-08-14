/*
 * design_bench — drive the radial navigation system on a real ST7789, live.
 *
 * Bench tool for the Pico-2 breadboard, NOT a product build. Its job is to
 * answer what no desktop preview can: at 39.1 x 21.2 mm of glass, does the
 * wheel actually read while turning, do the icons survive at 22 px, and does
 * the snap feel like a detent or like a lag.
 *
 * ARCHITECTURE. There is no framebuffer and no background asset. Each row is
 * composited into a 640-byte line buffer by tools/ui_wheel.c and pushed
 * straight to SPI, so the whole UI costs two line buffers instead of a 106 KB
 * colour framebuffer — the H743 sits at 87 % RAM_D1 and 96 % RAM_D2 against
 * 11 % flash, so that split is not optional. The wheel is drawn from signed
 * distance fields, which also means it carries no artwork: the black ground
 * costs nothing and the geometry is code.
 *
 * Compositing measures 0.22 ms/frame on the host; the panel needs 29 ms to
 * take a full frame at 32 MHz, so the transfer, not the drawing, sets the
 * frame rate.
 *
 * CONTROLS (single encoder + one button, same wiring as display_hw_test)
 *   rotate          MAIN/GROUP: rotate the structure under the 12 o'clock
 *                   selection point.  VALUE: move the value.
 *   push            descend: group -> parameter -> value
 *   SHIFT + push    climb back out
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

    gpio_set_function(PIN_BL, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_BL);
    pwm_set_wrap(slice, 255);
    pwm_set_gpio_level(PIN_BL, 220);
    pwm_set_enabled(slice, true);

    oled_init();
    ui_wheel_init(&ui);
    ui_wheel_settle(&ui);
    present();

    bool last_clk = gpio_get(PIN_ENC_CLK);
    bool last_sw  = true;
    absolute_time_t sw_guard = get_absolute_time();
    absolute_time_t last_frame = get_absolute_time();

    for (;;) {
        bool dirty = false;
        bool shift = !gpio_get(PIN_SHIFT);

        bool clk = gpio_get(PIN_ENC_CLK);
        if (last_clk && !clk) {                         /* falling edge */
            int dir = gpio_get(PIN_ENC_DT) ? +1 : -1;
            ui_wheel_turn(&ui, dir, shift);
            dirty = true;
        }
        last_clk = clk;

        bool sw = gpio_get(PIN_ENC_SW);
        if (last_sw && !sw &&
            absolute_time_diff_us(sw_guard, get_absolute_time()) > 0) {
            ui_wheel_press(&ui, shift);
            sw_guard = make_timeout_time_ms(180);
            dirty = true;
        }
        last_sw = sw;

        /* Animate the snap. The wheel keeps drawing frames while it is still
         * moving, and a new detent during the ease simply retargets it — the
         * rotation is never queued or replayed. */
        absolute_time_t now = get_absolute_time();
        int dt = (int)(absolute_time_diff_us(last_frame, now) / 1000);
        if (dt < 1) dt = 1;
        last_frame = now;
        if (ui_wheel_tick(&ui, dt)) dirty = true;

        if (dirty) present();
        else       sleep_ms(2);
    }
}
