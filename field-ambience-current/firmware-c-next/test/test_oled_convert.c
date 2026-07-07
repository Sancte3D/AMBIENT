/*
 * test_oled_convert.c — host test for oled_convert_row(): the 4-bit-grey →
 * RGB565 row converter shared by the blocking and async DMA panel drivers.
 * Verifies nibble order (high = left), MS-byte-first packing, LUT lookup,
 * and bounds.
 */
#include "oled.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int checks = 0;
#define CHECK(c) do { assert(c); ++checks; } while (0)

int main(void) {
    /* identity LUT: grey n -> RGB565 value n, so output byte pairs are
     * (0x00, n) and the pixel math is trivial to predict. */
    uint16_t lut[16];
    for (int n = 0; n < 16; ++n) lut[n] = (uint16_t)n;

    static uint8_t out[OLED_WIDTH * 2];

    /* 1) uniform fill: every pixel = grey 5 -> every pair (0x00, 0x05) */
    oled_fill(5);
    memset(out, 0xEE, sizeof out);
    oled_convert_row(0, lut, out);
    int ok = 1;
    for (int x = 0; x < OLED_WIDTH; ++x) {
        if (out[2*x] != 0x00 || out[2*x+1] != 0x05) { ok = 0; break; }
    }
    CHECK(ok);
    printf("  uniform row: out[0..3] = %02x %02x %02x %02x\n",
           out[0], out[1], out[2], out[3]);

    /* 2) nibble order: high nibble = LEFT pixel (even x), low = right (odd x) */
    oled_fill(0);
    oled_pixel(0, 0, 0x0A);          /* left  pixel of byte 0 */
    oled_pixel(1, 0, 0x03);          /* right pixel of byte 0 */
    oled_convert_row(0, lut, out);
    CHECK(out[0] == 0x00 && out[1] == 0x0A);   /* x=0 -> lut[0xA] */
    CHECK(out[2] == 0x00 && out[3] == 0x03);   /* x=1 -> lut[0x3] */
    printf("  nibble order: x0=%02x%02x x1=%02x%02x\n",
           out[0], out[1], out[2], out[3]);

    /* 3) MS byte first: a LUT entry with both bytes set must emit hi then lo */
    uint16_t lut2[16] = {0};
    lut2[7] = 0xBEEF;
    oled_fill(7);
    oled_convert_row(0, lut2, out);
    CHECK(out[0] == 0xBE && out[1] == 0xEF);
    printf("  MSB-first: out[0..1] = %02x %02x\n", out[0], out[1]);

    /* 4) out-of-range row is a no-op (buffer left untouched) */
    memset(out, 0x5A, sizeof out);
    oled_convert_row(OLED_HEIGHT, lut, out);
    CHECK(out[0] == 0x5A && out[1] == 0x5A);

    /* 5) a middle row is addressed independently of row 0 */
    oled_fill(0);
    oled_pixel(4, 10, 0x0F);
    oled_convert_row(10, lut, out);
    CHECK(out[2*4] == 0x00 && out[2*4+1] == 0x0F);
    oled_convert_row(9, lut, out);
    CHECK(out[2*4+1] == 0x00);        /* neighbouring row is clear */

    printf("oled_convert_row: %d checks, 0 failures\n", checks);
    return 0;
}
