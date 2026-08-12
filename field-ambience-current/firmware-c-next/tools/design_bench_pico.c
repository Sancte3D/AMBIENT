/*
 * design_bench — drive the candidate AMBIENT layouts on a real ST7789, live.
 *
 * Bench tool for the Pico-2 breadboard, NOT a product build. Its job is to
 * answer the one question no desktop preview can: at 39.1 x 21.2 mm of glass,
 * with the backlight down and the panel off-axis, which of the three
 * information densities can actually be read while playing.
 *
 * ARCHITECTURE — this is the split the product will use too:
 *
 *   background   the approved plate, baked once offline into RGB565 and living
 *                in flash as `plate_plain` (108,800 byte): gradient, glass
 *                card, white edge, glow. Never copied to RAM. The UI is NOT
 *                baked into it — tools/make_plain_plate.py removed the old
 *                list so any layout can draw over it.
 *   foreground   everything that carries information, drawn per row on the way
 *                to the panel by tools/ui_layouts.c.
 *
 * There is no framebuffer. Each row is composited into a 640-byte line buffer
 * straight from flash and pushed to SPI, so the whole UI costs two line
 * buffers instead of a 106 KB colour framebuffer. That matters: on the H743
 * the RAM budget is already at 87 % (D1) and 96 % (D2), while flash sits at
 * 11 %.
 *
 * CONTROLS (single encoder + one button, same wiring as display_hw_test)
 *   rotate          browse: move through the 16 parameters
 *                   edit:   change the selected value
 *   push            toggle browse <-> edit (same as src/menu.c)
 *   SHIFT + push    next layout: A FOCUS -> B CONTEXT -> C PAGES
 *   SHIFT + rotate  coarse steps while editing
 */

#include "ui_layouts.h"
#include "plate_plain.h"
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

static ui_state_t ui;

static void present(void)
{
    static uint16_t line[OLED_WIDTH];
    static uint8_t  out[OLED_WIDTH * 2];
    lcd_set_window_full();
    lcd_stream_begin();
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        ui_compose_row(&ui, y, line);
        for (int x = 0; x < OLED_WIDTH; ++x) {      /* ST7789 wants MSB first */
            out[x * 2]     = (uint8_t)(line[x] >> 8);
            out[x * 2 + 1] = (uint8_t)line[x];
        }
        lcd_stream_row(out, sizeof out);
    }
    lcd_stream_end();
}

/* ---- input -------------------------------------------------------------- */
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
    ui_init(&ui, UI_FOCUS);
    present();

    bool last_clk = gpio_get(PIN_ENC_CLK);
    bool last_sw  = true;
    absolute_time_t sw_guard = get_absolute_time();

    for (;;) {
        bool dirty = false;
        bool shift = !gpio_get(PIN_SHIFT);

        bool clk = gpio_get(PIN_ENC_CLK);
        if (last_clk && !clk) {                         /* falling edge */
            int dir = gpio_get(PIN_ENC_DT) ? +1 : -1;
            if (ui.edit) ui_edit(&ui, dir, shift);
            else         ui_move(&ui, dir);
            dirty = true;
        }
        last_clk = clk;

        bool sw = gpio_get(PIN_ENC_SW);
        if (last_sw && !sw &&
            absolute_time_diff_us(sw_guard, get_absolute_time()) > 0) {
            if (shift) {
                ui.layout = (ui_layout_t)((ui.layout + 1) % UI_LAYOUT_COUNT);
                ui.edit   = 0;
                printf("layout %d\n", (int)ui.layout);
            } else {
                ui.edit = !ui.edit;
            }
            sw_guard = make_timeout_time_ms(180);
            dirty = true;
        }
        last_sw = sw;

        if (dirty) present();
        sleep_ms(2);
    }
}
