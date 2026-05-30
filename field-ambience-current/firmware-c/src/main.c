/*
 * Field Ambience — native C firmware.
 *
 * Step 4: 4× EC11 encoders alive (rotation + push).
 *
 *   - Step 1: USB CDC heartbeat + STATUS LED on GP26 @ 1 Hz
 *   - Step 2: SSD1322 OLED, banner
 *   - Step 3: I²C + MCP23017 + 10 switches + jack-detect
 *   - Step 4 (here): 4× EC11 (DRIVE, BRIGHT, DISPLAY, VOLUME) on GP10-21,
 *     quadrature-decoded at 1 kHz from a hardware repeating timer, with
 *     debounced push switches. Live values render to the OLED; USB CDC
 *     logs each event.
 *
 * No audio yet. Step 5 = I²S DMA + first sine.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "version.h"
#include "oled.h"
#include "mcp23017.h"
#include "encoders.h"

#define PIN_STATUS_LED  26

static void draw_banner_static(void) {
    oled_text( 72,  0, "FIELD AMBIENCE", 0x0F);
    oled_text( 88,  8, "V0.9 STEP 4",    0x0A);
}

/* --- Step 3 button display (preserved, compact form on bottom rows) --- */
static void draw_buttons_state(uint16_t v) {
    /* Labels (Step 3 style, compacted onto y=48). */
    oled_text(0, 48, "C", 0x06);
    const int cell_x[5] = { 16, 32, 48, 64, 80 };
    for (int i = 0; i < 5; ++i) {
        bool pressed = !(v & (1u << (MCP_BIT_CELL1 + i)));
        char ch[2] = { pressed ? 'O' : '.', 0 };
        oled_text(cell_x[i], 48, " ", 0);
        oled_text(cell_x[i], 48, ch, pressed ? 0x0F : 0x05);
    }
    oled_text(96, 48, "M", 0x06);
    const int mod_x[5] = { 112, 128, 144, 160, 176 };
    for (int i = 0; i < 5; ++i) {
        bool pressed = !(v & (1u << (MCP_BIT_MOD_SHIFT + i)));
        char ch[2] = { pressed ? 'O' : '.', 0 };
        oled_text(mod_x[i], 48, " ", 0);
        oled_text(mod_x[i], 48, ch, pressed ? 0x0F : 0x05);
    }
    bool jack_in = (v & (1u << MCP_BIT_JACK)) != 0;
    oled_text(200, 48, " ", 0);
    oled_text(200, 48, "J", jack_in ? 0x0F : 0x05);
}

/* --- Step 4 encoder display --- */

/* Render a signed position into a fixed-width 5-char field: e.g. "+0123".
 * Range clamped to ±9999; values outside print as "+9999" / "-9999". */
static void format_signed(int32_t v, char out[6]) {
    bool neg = (v < 0);
    long n = neg ? -(long)v : (long)v;
    if (n > 9999) n = 9999;
    out[0] = neg ? '-' : '+';
    out[1] = (char)('0' + (n / 1000) % 10);
    out[2] = (char)('0' + (n / 100)  % 10);
    out[3] = (char)('0' + (n / 10)   % 10);
    out[4] = (char)('0' + (n)        % 10);
    out[5] = 0;
}

static void draw_encoders_state(void) {
    const char *labels[ENC_COUNT] = { "DRIV", "BRIT", "DISP", "VOL " };
    const int   col_x[ENC_COUNT]  = { 0, 64, 128, 192 };
    char buf[6];

    /* Row 24: labels + push-state dot */
    for (int i = 0; i < ENC_COUNT; ++i) {
        oled_text(col_x[i], 24, "        ", 0);   /* clear column band */
        oled_text(col_x[i], 24, labels[i], 0x07);
        bool sw = enc_pushed((uint8_t)(i + 1));
        oled_text(col_x[i] + 40, 24, sw ? "O" : ".", sw ? 0x0F : 0x05);
    }
    /* Row 32: signed position */
    for (int i = 0; i < ENC_COUNT; ++i) {
        oled_text(col_x[i], 32, "        ", 0);
        format_signed(enc_position((uint8_t)(i + 1)), buf);
        oled_text(col_x[i], 32, buf, 0x0F);
    }
}

int main(void) {
    stdio_init_all();

    gpio_init(PIN_STATUS_LED);
    gpio_set_dir(PIN_STATUS_LED, GPIO_OUT);

    oled_init();
    oled_fill(0);
    draw_banner_static();
    oled_show();

    bool mcp_ok = mcp_init();
    if (!mcp_ok) {
        oled_text(0, 48, "MCP23017 NOT FOUND", 0x0F);
        printf("ERROR: MCP23017 did not ACK at 0x%02X\n", MCP_I2C_ADDR);
    } else {
        draw_buttons_state(mcp_state());
    }

    enc_init();
    draw_encoders_state();
    oled_show();

    uint32_t hb_count = 0;
    absolute_time_t next_blink_at = make_timeout_time_ms(500);
    absolute_time_t next_hb_at    = make_timeout_time_ms(2000);
    bool led_on = false;

    while (true) {
        bool dirty = false;

        if (mcp_ok && mcp_irq_pending()) {
            uint16_t v = mcp_service();
            printf("MCP state 0x%04X\n", v);
            draw_buttons_state(v);
            dirty = true;
        }

        enc_event_t e;
        while (enc_pop_event(&e)) {
            if (e.kind == ENC_EVENT_ROTATE) {
                printf("ENC %u rotate %+d  pos=%ld\n",
                       e.id, e.value,
                       (long)enc_position(e.id));
            } else if (e.kind == ENC_EVENT_PUSH) {
                printf("ENC %u push   %s\n",
                       e.id, e.value ? "DOWN" : "UP");
            }
            dirty = true;
        }

        if (dirty) {
            draw_encoders_state();
            oled_show();
        }

        if (absolute_time_diff_us(get_absolute_time(), next_blink_at) <= 0) {
            led_on = !led_on;
            gpio_put(PIN_STATUS_LED, led_on);
            next_blink_at = make_timeout_time_ms(500);
        }
        if (absolute_time_diff_us(get_absolute_time(), next_hb_at) <= 0) {
            printf("%s — %s — heartbeat %lu\n",
                   FAM_FW_VERSION_STR, FAM_FW_BOARD_STR,
                   (unsigned long)hb_count++);
            next_hb_at = make_timeout_time_ms(2000);
        }

        sleep_ms(20);   /* keep encoder/MCP latency under a frame; 1 kHz sampling runs on the timer */
    }
}
