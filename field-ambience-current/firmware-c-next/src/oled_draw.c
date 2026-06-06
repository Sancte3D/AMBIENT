/*
 * OLED draw layer — pure framebuffer operations, no SPI / no Pico SDK.
 * Split out of src/oled.c so the menu can be host-rendered to a preview
 * image without touching hardware. The SPI side (oled_init / oled_show)
 * stays in src/oled.c and shares the same `fb` buffer via oled_framebuffer().
 *
 * Layout: 256×64 4-bit grey, 2 px per byte, high nibble = left pixel
 * (matches the SSD1322 GDDRAM as it is written, so oled_show streams
 * the buffer with no transform).
 */

#include "oled.h"
#include <string.h>

static uint8_t fb[OLED_FB_SIZE];

extern const uint8_t *font_glyph(char c);

const uint8_t *oled_framebuffer(void) { return fb; }

void oled_fill(uint8_t gs) {
    uint8_t b = (uint8_t)(((gs & 0x0F) << 4) | (gs & 0x0F));
    memset(fb, b, sizeof fb);
}

static inline void plot(int x, int y, uint8_t gs) {
    if ((unsigned)x >= OLED_WIDTH || (unsigned)y >= OLED_HEIGHT) return;
    int idx = (y * OLED_WIDTH + x) >> 1;
    uint8_t cur = fb[idx];
    if (x & 1) fb[idx] = (uint8_t)((cur & 0xF0) | (gs & 0x0F));        /* right pixel = low nibble */
    else       fb[idx] = (uint8_t)((cur & 0x0F) | ((gs & 0x0F) << 4)); /* left = high nibble */
}

void oled_pixel(int x, int y, uint8_t gs) { plot(x, y, gs); }

void oled_rect_fill(int x, int y, int w, int h, uint8_t gs) {
    if (w <= 0 || h <= 0) return;
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx)
            plot(xx, yy, gs);
}

void oled_pill(int x, int y, int w, int h, uint8_t gs) {
    /* Pill = central rect + two semicircular caps of radius r=h/2. */
    if (w <= 0 || h <= 0) return;
    int r = h / 2;
    if (r < 1) { oled_rect_fill(x, y, w, h, gs); return; }
    /* Centre rectangle (between the cap centres). */
    oled_rect_fill(x + r, y, w - 2 * r, h, gs);
    /* Two filled semicircles. */
    int cxl = x + r, cxr = x + w - 1 - r, cy = y + r;
    int r2 = r * r;
    for (int dy = -r; dy < r; ++dy) {
        int dy2 = dy * dy;
        for (int dx = -r; dx <= 0; ++dx) {
            if (dx * dx + dy2 <= r2) {
                plot(cxl + dx, cy + dy, gs);
                plot(cxr - dx, cy + dy, gs);
            }
        }
    }
}

static void blit_glyph(int x, int y, const uint8_t *rows, uint8_t gs, int scale) {
    /* Each glyph is 8 rows of 8 horizontal pixels (MSB = leftmost). Scale is
     * an integer block size — scale=1 reproduces the original 8x8 cell. */
    if (!rows) return;
    for (int gy = 0; gy < 8; ++gy) {
        uint8_t row = rows[gy];
        for (int gx = 0; gx < 8; ++gx) {
            if (row & (uint8_t)(0x80u >> gx)) {
                if (scale == 1) {
                    plot(x + gx, y + gy, gs);
                } else {
                    int bx = x + gx * scale, by = y + gy * scale;
                    for (int sy = 0; sy < scale; ++sy)
                        for (int sx = 0; sx < scale; ++sx)
                            plot(bx + sx, by + sy, gs);
                }
            }
        }
    }
}

void oled_text(int x, int y, const char *s, uint8_t gs) {
    while (*s) { blit_glyph(x, y, font_glyph(*s), gs, 1); x += 8; ++s; }
}

void oled_text_scaled(int x, int y, const char *s, uint8_t gs, int scale) {
    if (scale < 1) scale = 1;
    int step = 8 * scale;
    while (*s) { blit_glyph(x, y, font_glyph(*s), gs, scale); x += step; ++s; }
}

int oled_text_width(const char *s, int scale) {
    if (scale < 1) scale = 1;
    int n = 0; while (s[n]) ++n;
    return n * 8 * scale;
}
