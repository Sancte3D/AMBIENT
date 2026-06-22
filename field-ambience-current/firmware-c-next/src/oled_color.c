/*
 * oled_color.c — accent-tinted grey → RGB565 LUT (ADR-0015 step 1).
 *
 * See include/oled_color.h. Host-portable: no SDK, no hardware — compiled into
 * the host tests, the preview tool and both device drivers so the conversion
 * is identical everywhere.
 */

#include "oled_color.h"

static uint8_t  acc_r = 255, acc_g = 255, acc_b = 255;   /* default = white */
static uint16_t lut[16];
static int      dirty = 1;

/* grey8 = nib·17 (0,17,…,255). Scale each channel by accent/255, then pack
 * RGB565 (r5<<11 | g6<<5 | b5). accent=255,255,255 ⇒ r=g=b=grey8 ⇒ the legacy
 * neutral grey ramp (grey 15 → 0xFFFF, grey 0 → 0x0000). */
static void rebuild(void) {
    for (int n = 0; n < 16; ++n) {
        int g8  = n * 17;
        int r8  = (g8 * acc_r + 127) / 255;
        int gg8 = (g8 * acc_g + 127) / 255;
        int b8  = (g8 * acc_b + 127) / 255;
        int r5 = r8 >> 3, g6 = gg8 >> 2, b5 = b8 >> 3;
        lut[n] = (uint16_t)((r5 << 11) | (g6 << 5) | b5);
    }
    dirty = 0;
}

void oled_set_accent(uint8_t r, uint8_t g, uint8_t b) {
    if (r == acc_r && g == acc_g && b == acc_b) return;
    acc_r = r; acc_g = g; acc_b = b;
    dirty = 1;
}

void oled_get_accent(uint8_t *r, uint8_t *g, uint8_t *b) {
    if (r) *r = acc_r;
    if (g) *g = acc_g;
    if (b) *b = acc_b;
}

uint16_t oled_grey565(uint8_t grey4) {
    if (dirty) rebuild();
    return lut[grey4 & 0x0F];
}

const uint16_t *oled_grey565_lut(void) {
    if (dirty) rebuild();
    return lut;
}
