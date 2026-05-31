/*
 * Field Ambience — native C firmware.
 *
 * Step 2: the OLED is alive. The panel is initialised, framebuffer rendered,
 * and a static banner is shown:
 *
 *     FIELD AMBIENCE
 *     V0.9 STEP 2
 *
 * The Step 1 behaviour is preserved: STATUS LED on GP26 blinks at 1 Hz and
 * a USB CDC heartbeat lands every 2 seconds so the host can confirm liveness.
 *
 * Pin assignment matches SPEC v0.6 §5 for the pins we touch (STATUS LED
 * on GP26; SPI0 OLED on GP5/6/7/8/9). The I²S reassignment of GP0/GP1/GP4
 * happens in Step 5/6, not here.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "version.h"
#include "oled.h"

#define PIN_STATUS_LED  26

static void draw_banner(void) {
    /* Two centred-ish lines on the 256x64 panel. Font is 8x8, 8 px advance,
     * so "FIELD AMBIENCE" (14 chars) = 112 px wide -> x=72 centres it. */
    oled_fill(0);
    oled_text(72, 16, "FIELD AMBIENCE", 0x0F);
    oled_text(88, 32, "V0.9 STEP 2",    0x0A);  /* dimmer subtitle */
    oled_show();
}

int main(void) {
    stdio_init_all();

    gpio_init(PIN_STATUS_LED);
    gpio_set_dir(PIN_STATUS_LED, GPIO_OUT);

    oled_init();
    draw_banner();

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
