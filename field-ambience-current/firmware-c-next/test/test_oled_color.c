/*
 * test_oled_color.c — accent-tinted grey→RGB565 LUT (ADR-0015 step 1).
 *
 * Invariants:
 *  1. The default accent (white) reproduces the LEGACY neutral grey ramp
 *     bit-for-bit, so adopting the colour layer changes nothing until a world
 *     sets an accent.
 *  2. grey 0 → black, grey 15 → full white (with white accent).
 *  3. Luminance is monotonic non-decreasing across grey 0..15.
 *  4. Reducing an accent channel reduces that channel in the output (a cast,
 *     not arbitrary).
 *  5. The single-pixel path and the LUT path agree.
 */

#include "oled_color.h"

#include <stdio.h>
#include <stdint.h>

static int g_checks = 0, g_fails = 0;
#define CHECK(c, ...) do { ++g_checks; if (!(c)) { ++g_fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

/* Reference: the historical inline LUT from the Pico driver (neutral grey). */
static uint16_t legacy_grey565(int n) {
    int g8 = n * 17;
    int r5 = g8 >> 3, g6 = g8 >> 2, b5 = g8 >> 3;
    return (uint16_t)((r5 << 11) | (g6 << 5) | b5);
}

static int r_of(uint16_t p) { return (p >> 11) & 0x1F; }
static int g_of(uint16_t p) { return (p >> 5)  & 0x3F; }
static int b_of(uint16_t p) { return  p        & 0x1F; }
/* crude luminance proxy on the 565 fields (weights need not be exact, only
 * monotonic-preserving for a neutral ramp). */
static int lum(uint16_t p) { return r_of(p) * 2 + g_of(p) * 2 + b_of(p) * 2; }

int main(void) {
    /* 1 + 2: white accent == legacy ramp, endpoints correct. */
    oled_set_accent(255, 255, 255);
    for (int n = 0; n < 16; ++n)
        CHECK(oled_grey565((uint8_t)n) == legacy_grey565(n),
              "white accent grey %d: got %04x want %04x",
              n, oled_grey565((uint8_t)n), legacy_grey565(n));
    CHECK(oled_grey565(0)  == 0x0000, "grey 0 not black");
    CHECK(oled_grey565(15) == 0xFFFF, "grey 15 not full white");

    /* 3: monotonic non-decreasing luminance. */
    for (int n = 1; n < 16; ++n)
        CHECK(lum(oled_grey565((uint8_t)n)) >= lum(oled_grey565((uint8_t)(n - 1))),
              "luminance dipped at grey %d", n);

    /* 4: a cool-blue accent must keep blue >= red at the bright end, and must
     * pull red below the neutral ramp's red (cast, not recolour). */
    oled_set_accent(175, 205, 255);
    uint16_t hi = oled_grey565(15);
    CHECK(b_of(hi) >= r_of(hi), "blue accent: blue should dominate red at top");
    CHECK(r_of(hi) <  r_of(legacy_grey565(15)), "blue accent should reduce red");
    CHECK(oled_grey565(0) == 0x0000, "black stays black under any accent");

    /* 5: single-pixel path agrees with the LUT path. */
    const uint16_t *lut = oled_grey565_lut();
    for (int n = 0; n < 16; ++n)
        CHECK(lut[n] == oled_grey565((uint8_t)n), "LUT vs fn mismatch at %d", n);

    /* 6: accent crossfade — set a target, tick toward it, must converge and
     * then report "done" (0). Start live at white, target at amber. */
    oled_set_accent(255, 255, 255);
    oled_set_accent_target(255, 205, 150);
    uint8_t r0, g0, b0;
    oled_get_accent(&r0, &g0, &b0);
    CHECK(g0 == 255 && b0 == 255, "tick: live moved before any tick");
    int moved = 0;
    uint32_t t = 1000;
    for (int i = 0; i < 200 && oled_accent_tick(t += 16); ++i) moved = 1;
    CHECK(moved, "tick: never reported motion");
    oled_get_accent(&r0, &g0, &b0);
    CHECK(r0 == 255 && g0 == 205 && b0 == 150, "tick: did not converge (%d,%d,%d)", r0, g0, b0);
    CHECK(oled_accent_tick(t += 16) == 0, "tick: not idle at target");

    /* 7: settle snaps instantly. */
    oled_set_accent(255, 255, 255);
    oled_set_accent_target(100, 120, 255);
    oled_accent_settle();
    oled_get_accent(&r0, &g0, &b0);
    CHECK(r0 == 100 && g0 == 120 && b0 == 255, "settle: did not snap (%d,%d,%d)", r0, g0, b0);

    /* restore default so nothing leaks if linked alongside other suites. */
    oled_set_accent(255, 255, 255);

    printf("%d checks, %d failures\n", g_checks, g_fails);
    printf("RESULT: %s\n", g_fails ? "FAIL" : "PASS");
    return g_fails ? 1 : 0;
}
