/*
 * Field Ambience — native C firmware, Step 1: bare-bones skeleton.
 *
 * What this file does right now:
 *   - Initialises USB CDC for stdio.
 *   - Blinks the STATUS LED on GP26 at 1 Hz.
 *   - Prints a heartbeat string on USB CDC every 2 seconds so a host can
 *     confirm the device is alive and which firmware is running.
 *
 * What it does NOT do yet (later steps):
 *   - OLED rendering (Step 2)
 *   - I²C / MCP23017 button scan (Step 3)
 *   - Encoder quadrature decode (Step 4)
 *   - I²S DMA audio output (Step 5)
 *   - Anything that makes a sound
 *
 * Pin assignment matches SPEC v0.6 §5 for the pins we touch today
 * (STATUS LED on GP26). The I²S reassignment of GP0/GP1/GP4 happens in
 * Step 5/6 — not here.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "version.h"

/* GP26 = STATUS_LED, per SPEC v0.6 §5 (the same pin the MicroPython
 * firmware blinks). Re-used so the visible-LED behaviour is identical
 * between the two firmware paths during the transition. */
#define PIN_STATUS_LED  26

int main(void) {
    stdio_init_all();

    gpio_init(PIN_STATUS_LED);
    gpio_set_dir(PIN_STATUS_LED, GPIO_OUT);

    /* Heartbeat: LED toggles every 500 ms (=> 1 Hz blink), and once every
     * 4 toggles (=> every 2 s) the USB CDC gets a line. The host sees a
     * steady, non-blocking "still alive" trace and a recognisable banner. */
    uint32_t tick = 0;
    while (true) {
        gpio_put(PIN_STATUS_LED, tick & 1);
        if ((tick & 3) == 0) {
            printf("%s — %s — heartbeat %lu\n",
                   FAM_FW_VERSION_STR, FAM_FW_BOARD_STR,
                   (unsigned long)(tick >> 2));
        }
        sleep_ms(500);
        tick++;
    }
}
