/*
 * oled_color.c — accent-tinted grey → RGB565 LUT with a crossfading accent
 * (ADR-0015 step 1 + accent animation).
 *
 * See include/oled_color.h. Host-portable: no SDK, no hardware — compiled into
 * the host tests, the preview tool and both device drivers so the conversion
 * is identical everywhere.
 *
 * The LIVE accent is what the LUT is built from. A world change sets the
 * TARGET; oled_accent_tick() eases the live accent toward it each frame so the
 * whole screen morphs colour instead of snapping.
 */

#include "oled_color.h"
#include <math.h>

#define ACCENT_TAU_MS  120.0f         /* crossfade time constant */

static float   live_r = 255.0f, live_g = 255.0f, live_b = 255.0f;
static uint8_t tgt_r  = 255,    tgt_g  = 255,    tgt_b  = 255;
static uint16_t lut[16];
static int      dirty = 1;

static uint32_t last_ms = 0;
static int      have_last = 0;

/* grey8 = nib·17 (0,17,…,255). Scale each channel by live/255, pack RGB565
 * (r5<<11 | g6<<5 | b5). live=255,255,255 ⇒ the legacy neutral grey ramp. */
static void rebuild(void) {
    int ar = (int)(live_r + 0.5f), ag = (int)(live_g + 0.5f), ab = (int)(live_b + 0.5f);
    for (int n = 0; n < 16; ++n) {
        int g8  = n * 17;
        int r8  = (g8 * ar + 127) / 255;
        int gg8 = (g8 * ag + 127) / 255;
        int b8  = (g8 * ab + 127) / 255;
        int r5 = r8 >> 3, g6 = gg8 >> 2, b5 = b8 >> 3;
        lut[n] = (uint16_t)((r5 << 11) | (g6 << 5) | b5);
    }
    dirty = 0;
}

void oled_set_accent(uint8_t r, uint8_t g, uint8_t b) {
    tgt_r = r; tgt_g = g; tgt_b = b;
    live_r = r; live_g = g; live_b = b;
    dirty = 1;
}

void oled_set_accent_target(uint8_t r, uint8_t g, uint8_t b) {
    tgt_r = r; tgt_g = g; tgt_b = b;
}

void oled_get_accent(uint8_t *r, uint8_t *g, uint8_t *b) {
    if (r) *r = (uint8_t)(live_r + 0.5f);
    if (g) *g = (uint8_t)(live_g + 0.5f);
    if (b) *b = (uint8_t)(live_b + 0.5f);
}

void oled_accent_settle(void) {
    if (live_r == tgt_r && live_g == tgt_g && live_b == tgt_b) return;
    live_r = tgt_r; live_g = tgt_g; live_b = tgt_b;
    dirty = 1;
}

int oled_accent_tick(uint32_t now_ms) {
    if (!have_last) { last_ms = now_ms; have_last = 1; }
    uint32_t dt = now_ms - last_ms;
    last_ms = now_ms;

    float dr = (float)tgt_r - live_r, dg = (float)tgt_g - live_g, db = (float)tgt_b - live_b;
    if (dr == 0.0f && dg == 0.0f && db == 0.0f) return 0;

    float k = 1.0f - expf(-(float)dt / ACCENT_TAU_MS);
    if (k > 1.0f) k = 1.0f;
    live_r += dr * k; live_g += dg * k; live_b += db * k;

    /* snap when within half a code so we actually reach the target */
    if (fabsf((float)tgt_r - live_r) < 0.5f) live_r = tgt_r;
    if (fabsf((float)tgt_g - live_g) < 0.5f) live_g = tgt_g;
    if (fabsf((float)tgt_b - live_b) < 0.5f) live_b = tgt_b;

    dirty = 1;
    return 1;
}

uint16_t oled_grey565(uint8_t grey4) {
    if (dirty) rebuild();
    return lut[grey4 & 0x0F];
}

const uint16_t *oled_grey565_lut(void) {
    if (dirty) rebuild();
    return lut;
}
