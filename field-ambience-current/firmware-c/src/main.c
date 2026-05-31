/*
 * Field Ambience — native C firmware.
 *
 * Step 3: I²C + MCP23017 + 10 switches + jack-detect alive.
 *
 *   - Step 1: USB CDC heartbeat + STATUS LED on GP26 @ 1 Hz.
 *   - Step 2: SSD1322 OLED, banner.
 *   - Step 3 (here): I²C1 @ 400 kHz, MCP23017 at 0x20, GPA0-4 = CELL1-5,
 *     GPB0-4 = SHIFT/HOLD/DRONE/GENERATE/CLEAR, GPA6 = JACK_DETECT,
 *     GPA5 = PCM_XSMT. INTA wired to GP22 falling-edge.
 *
 * On every IRQ:
 *   - read both ports (auto-clears INT)
 *   - log a one-line USB CDC trace with which inputs are now low (pressed)
 *   - redraw the live-state row on the OLED (dim 'O' = released, bright 'O'
 *     = pressed); jack-detect rendered as a 'J' that brightens when a plug
 *     is in.
 *
 * No audio still, no menu logic yet. Step 4 = encoders, Step 5 = I²S.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "version.h"
#include "oled.h"
#include "mcp23017.h"

#define PIN_STATUS_LED  26

static void draw_banner_static(void) {
    /* Top half of the display = static banner (drawn once). */
    oled_text( 72,  0, "FIELD AMBIENCE", 0x0F);
    oled_text( 88,  8, "V0.9 STEP 3",    0x0A);
    /* Row labels for the live state below. */
    oled_text(  0, 24, "CELL  1  2  3  4  5    JACK", 0x06);
    oled_text(  0, 48, "MOD   S  H  D  G  C",         0x06);
}

/* Render '.' (dim) when bit is 1 (released / open), 'O' (bright) when bit
 * is 0 (pressed / closed). Inverted for jack since plug-in pulls HIGH. */
static void draw_live_state(uint16_t v) {
    /* Wipe the two live rows without touching the labels above. */
    for (int y = 32; y < 40; ++y)
        for (int x = 0; x < OLED_WIDTH; ++x)
            ; /* no-op; we just overwrite per-glyph */

    /* The simplest reliable clear: blit a space glyph (zero pixels) over
     * each cell position before drawing the new one. */
    const int cell_x[5] = { 48, 72, 96, 120, 144 };
    for (int i = 0; i < 5; ++i) {
        bool pressed = !(v & (1u << (MCP_BIT_CELL1 + i)));
        char ch[2] = { pressed ? 'O' : '.', 0 };
        oled_text(cell_x[i], 32, " ", 0);          /* erase prior */
        oled_text(cell_x[i], 32, ch,  pressed ? 0x0F : 0x05);
    }

    /* Jack-detect: in this MCP wiring with a normally-closed PJ-320D detect
     * switch + GPPU pull-up, no plug = LOW (switch shorts to GND), plug-in
     * = HIGH (switch opens, pull-up wins). Inverted vs the cells. */
    bool jack_in = (v & (1u << MCP_BIT_JACK)) != 0;
    oled_text(208, 32, " ", 0);
    oled_text(208, 32, "J", jack_in ? 0x0F : 0x05);

    /* Modifier row below. */
    const int mod_x[5] = { 48, 72, 96, 120, 144 };
    for (int i = 0; i < 5; ++i) {
        bool pressed = !(v & (1u << (MCP_BIT_MOD_SHIFT + i)));
        char ch[2] = { pressed ? 'O' : '.', 0 };
        oled_text(mod_x[i], 56, " ", 0);
        oled_text(mod_x[i], 56, ch, pressed ? 0x0F : 0x05);
    }
    oled_show();
}

static void log_event(uint16_t v) {
    /* USB CDC: one line with the raw 16-bit state. Easy to grep. */
    printf("MCP state 0x%04X  cells=%c%c%c%c%c  mod=%c%c%c%c%c  jack=%c\n",
        v,
        (v & (1u<<MCP_BIT_CELL1)) ? '.' : '#',
        (v & (1u<<MCP_BIT_CELL2)) ? '.' : '#',
        (v & (1u<<MCP_BIT_CELL3)) ? '.' : '#',
        (v & (1u<<MCP_BIT_CELL4)) ? '.' : '#',
        (v & (1u<<MCP_BIT_CELL5)) ? '.' : '#',
        (v & (1u<<MCP_BIT_MOD_SHIFT))    ? '.' : '#',
        (v & (1u<<MCP_BIT_MOD_HOLD))     ? '.' : '#',
        (v & (1u<<MCP_BIT_MOD_DRONE))    ? '.' : '#',
        (v & (1u<<MCP_BIT_MOD_GENERATE)) ? '.' : '#',
        (v & (1u<<MCP_BIT_MOD_CLEAR))    ? '.' : '#',
        (v & (1u<<MCP_BIT_JACK)) ? 'I' : '-');
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
        oled_text(0, 32, "MCP23017 NOT FOUND", 0x0F);
        oled_show();
        /* Halt heartbeat to make the failure obvious. */
        printf("ERROR: MCP23017 did not ACK at 0x%02X\n", MCP_I2C_ADDR);
    } else {
        draw_live_state(mcp_state());
    }

    uint32_t tick = 0;
    while (true) {
        gpio_put(PIN_STATUS_LED, tick & 1);

        if (mcp_ok && mcp_irq_pending()) {
            uint16_t v = mcp_service();
            log_event(v);
            draw_live_state(v);
        }

        if ((tick & 3) == 0) {
            printf("%s — %s — heartbeat %lu\n",
                   FAM_FW_VERSION_STR, FAM_FW_BOARD_STR,
                   (unsigned long)(tick >> 2));
        }
        sleep_ms(500);
        tick++;
    }
}
