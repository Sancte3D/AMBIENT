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
 * THE FRAME BUDGET IS THE BINDING CONSTRAINT, and getting it wrong is what
 * made the first version feel jerky. A frame is 108,800 bytes; at 24 MHz that
 * is 36.3 ms of wire time, and the compositor wanted its own time on top of
 * that — roughly 22 fps, at which a 90 ms ease has TWO frames to run in. Two
 * frames is not a motion. Fixed here in two places: the transfer is now
 * DMA-driven so drawing overlaps it, and ui_motion.h resolves durations
 * against the MEASURED frame time rather than assuming a 60 fps screen. The
 * loop feeds the measurement back every frame and prints it once a second, so
 * the number is observed instead of guessed at.
 *
 * INPUT IS INTERRUPT-DRIVEN. A detent that arrives during a blit would
 * otherwise be lost, which on a fast turn drops roughly every other step. The
 * GPIO IRQ decodes while the panel is being written, the loop drains it, and
 * the model moves before the next frame is drawn.
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
#include "ui_motion.h"
#include "ui_encoder.h"
#include "oled.h"

#include "pico/stdlib.h"
#include "hardware/sync.h"
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
extern void lcd_stream_row_dma(const uint8_t *row, size_t n);
extern void lcd_stream_wait(void);
extern void lcd_stream_end(void);

static wheel_state_t ui;

/* Compose row N+1 while row N is on the wire.
 *
 * The frame is 108,800 bytes; at 24 MHz that is 36.3 ms of transfer with the
 * CPU otherwise idle, and the compositor wants its own time on top. Done
 * serially the frame costs transfer PLUS drawing, which is what put the bench
 * near 22 fps — and at 22 fps a short ease is two frames, i.e. a stutter with
 * an intermediate position rather than a motion. Overlapped, the frame costs
 * whichever of the two is larger. */
static void present(void)
{
    static uint16_t line[OLED_WIDTH];
    static uint8_t  buf[2][OLED_WIDTH * 2];
    int cur = 0;

    lcd_set_window_full();
    lcd_stream_begin();

    ui_wheel_compose_row(&ui, 0, line);
    for (int x = 0; x < OLED_WIDTH; ++x) {          /* ST7789 wants MSB first */
        buf[cur][x * 2]     = (uint8_t)(line[x] >> 8);
        buf[cur][x * 2 + 1] = (uint8_t)line[x];
    }

    for (int y = 0; y < OLED_HEIGHT; ++y) {
        lcd_stream_row_dma(buf[cur], OLED_WIDTH * 2);

        if (y + 1 < OLED_HEIGHT) {                  /* draw ahead, in the gap */
            int nxt = cur ^ 1;
            ui_wheel_compose_row(&ui, y + 1, line);
            for (int x = 0; x < OLED_WIDTH; ++x) {
                buf[nxt][x * 2]     = (uint8_t)(line[x] >> 8);
                buf[nxt][x * 2 + 1] = (uint8_t)line[x];
            }
        }

        lcd_stream_wait();
        cur ^= 1;
    }
    lcd_stream_end();
}

/* ---- interrupt-driven input ---------------------------------------------
 * The encoder is decoded as QUADRATURE, not by counting edges on one pin.
 * Counting falling edges on A and sampling B inside the interrupt is what made
 * one physical click move two or three steps: the contacts bounce, so a single
 * detent fires the interrupt several times, and B is read at the exact moment
 * it is least settled. The state machine in ui_encoder.c has neither problem —
 * a bounce walks forward and straight back, and the two cancel. */
static ui_enc_t          enc;
static volatile bool     sw_held;
static volatile uint32_t sw_down_us;
static volatile bool     sw_short_evt;
static volatile bool     sw_long_fired;

static void on_gpio(uint gpio, uint32_t events)
{
    static uint32_t last_sw_us;
    uint32_t now = time_us_32();

    if (gpio == PIN_ENC_CLK || gpio == PIN_ENC_DT) {
        (void)events;
        ui_enc_update(&enc, gpio_get(PIN_ENC_CLK), gpio_get(PIN_ENC_DT));
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

    /* Both edges of BOTH encoder pins: a quadrature decoder needs to see every
     * transition, not just one pin's falling edge. */
    ui_enc_init(&enc, gpio_get(PIN_ENC_CLK), gpio_get(PIN_ENC_DT));
    gpio_set_irq_enabled_with_callback(PIN_ENC_CLK,
                                       GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                                       true, &on_gpio);
    gpio_set_irq_enabled(PIN_ENC_DT, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                         true);
    gpio_set_irq_enabled(PIN_ENC_SW, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                         true);

    gpio_set_function(PIN_BL, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_BL);
    pwm_set_wrap(slice, 255);
    pwm_set_gpio_level(PIN_BL, 220);
    pwm_set_enabled(slice, true);

    oled_init();
    ui_wheel_init(&ui);
    /* This bench has ONE encoder; the product has four, and a scene's three
     * properties live on three of them simultaneously. Saying so here turns on
     * the underline that marks which property the single encoder is holding —
     * an affordance the product does not need and does not draw. */
    ui.one_encoder = 1;
    ui_wheel_settle(&ui);
    present();

    absolute_time_t last_frame = get_absolute_time();

    for (;;) {
        bool dirty = false;
        bool shift = !gpio_get(PIN_SHIFT);

        /* Drain everything the decoder collected, including whatever arrived
         * during the last blit. Each detent is applied to the model in full;
         * only the drawing lags. */
        uint32_t irq = save_and_disable_interrupts();
        int32_t  d   = ui_enc_take(&enc);
        restore_interrupts(irq);
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

        if (dirty) {
            absolute_time_t t0 = get_absolute_time();
            present();
            /* Feed the REAL frame time back into the motion system, so the
             * durations stretch to whatever this panel can actually deliver
             * instead of assuming a 60 fps screen. */
            ui_motion_set_frame_ms(
                (float)absolute_time_diff_us(t0, get_absolute_time()) / 1000.0f);
        } else {
            sleep_ms(2);
        }

        /* Report the measured frame budget once a second — guessing at it is
         * what produced a 90 ms ease that only had two frames to run in. */
        static absolute_time_t last_report;
        if (absolute_time_diff_us(last_report, get_absolute_time()) > 1000000) {
            last_report = get_absolute_time();
            printf("frame %.1f ms (%.0f fps) | micro %d  move %d  level %d ms\n",
                   (double)ui_motion_frame_ms(),
                   1000.0 / (double)ui_motion_frame_ms(),
                   ui_motion_duration(UI_SPEED_MICRO),
                   ui_motion_duration(UI_SPEED_MOVE),
                   ui_motion_duration(UI_SPEED_LEVEL));
        }
    }
}
