/*
 * Display draw layer — pure framebuffer operations, no SPI / no Pico SDK.
 * Split out of the panel driver so the menu can be host-rendered to a preview
 * image without touching hardware. The SPI side (oled_init / oled_show) lives
 * in src/lcd_st7789.c (r16) and shares this `fb` via oled_framebuffer().
 *
 * Layout: 320×170 4-bit grey, 2 px per byte, high nibble = left pixel.
 * The device driver converts each grey pixel → RGB565 while streaming, so the
 * whole draw/font/menu stack is panel-agnostic (was SSD1322 256×64 pre-r16).
 */

#include "oled.h"
#include <string.h>
#include <math.h>

static uint8_t fb[OLED_FB_SIZE];

extern const uint8_t *font_glyph(char c);

const uint8_t *oled_framebuffer(void) { return fb; }

/* Convert one framebuffer row (OLED_WIDTH 4-bit grey pixels, 2 px/byte, high
 * nibble = left) to OLED_WIDTH RGB565 pixels, MS byte first, using the
 * accent LUT. `out` must hold OLED_WIDTH*2 bytes. Pure — no SPI — so the
 * panel drivers (blocking + async DMA) share one verified converter and the
 * host suite can check the pixel math. */
void oled_convert_row(int y, const uint16_t *lut565, uint8_t *out) {
    if ((unsigned)y >= OLED_HEIGHT) return;
    const uint8_t *row = fb + (size_t)y * (OLED_WIDTH / 2);
    uint8_t *o = out;
    for (int x = 0; x < OLED_WIDTH; x += 2) {
        uint8_t b = *row++;
        uint16_t hp = lut565[b >> 4];       /* even x = high nibble (left) */
        uint16_t lp = lut565[b & 0x0F];     /* odd  x = low  nibble        */
        *o++ = (uint8_t)(hp >> 8); *o++ = (uint8_t)hp;
        *o++ = (uint8_t)(lp >> 8); *o++ = (uint8_t)lp;
    }
}

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

/* Max-blend a pixel (keep the brighter) — for anti-aliased compositing. */
static inline void plot_max(int x, int y, uint8_t gs) {
    if ((unsigned)x >= OLED_WIDTH || (unsigned)y >= OLED_HEIGHT) return;
    int idx = (y * OLED_WIDTH + x) >> 1;
    uint8_t cur = (x & 1) ? (fb[idx] & 0x0F) : (fb[idx] >> 4);
    if ((gs & 0x0F) <= cur) return;
    plot(x, y, gs);
}
void oled_pixel_max(int x, int y, uint8_t gs) { plot_max(x, y, gs); }

/* Signed distance from point (px,py) to a rounded rect centred at (cx,cy) with
 * half-extents (hx,hy) and corner radius r. <0 inside. */
static float sdf_rrect(float px, float py, float cx, float cy,
                       float hx, float hy, float r) {
    float qx = fabsf(px - cx) - (hx - r);
    float qy = fabsf(py - cy) - (hy - r);
    float ax = qx > 0 ? qx : 0, ay = qy > 0 ? qy : 0;
    float outside = sqrtf(ax * ax + ay * ay);
    float inside  = (qx > qy ? qx : qy); if (inside > 0) inside = 0;
    return outside + inside - r;
}

/* Coverage from a signed distance with a ~1px anti-alias band. */
static float cov_from_sdf(float d) {
    float c = 0.5f - d;                 /* 1 at d=-0.5, 0 at d=+0.5 */
    return c < 0 ? 0 : (c > 1 ? 1 : c);
}

void oled_rrect_fill(int x, int y, int w, int h, int r, uint8_t gs) {
    if (w <= 0 || h <= 0) return;
    float cx = x + w * 0.5f, cy = y + h * 0.5f;
    float hx = w * 0.5f, hy = h * 0.5f;
    float rr = (float)r;
    if (rr > hx) rr = hx;
    if (rr > hy) rr = hy;
    for (int yy = y - 1; yy < y + h + 1; ++yy)
        for (int xx = x - 1; xx < x + w + 1; ++xx) {
            float d = sdf_rrect(xx + 0.5f, yy + 0.5f, cx, cy, hx, hy, rr);
            float c = cov_from_sdf(d);
            if (c > 0.0f) plot_max(xx, yy, (uint8_t)((float)gs * c + 0.5f));
        }
}

void oled_rrect_stroke(int x, int y, int w, int h, int r, int t, uint8_t gs) {
    if (w <= 0 || h <= 0) return;
    float cx = x + w * 0.5f, cy = y + h * 0.5f;
    float hx = w * 0.5f, hy = h * 0.5f;
    float rr = (float)r;
    if (rr > hx) rr = hx;
    if (rr > hy) rr = hy;
    float half = t * 0.5f;
    for (int yy = y - 1; yy < y + h + 1; ++yy)
        for (int xx = x - 1; xx < x + w + 1; ++xx) {
            float d = sdf_rrect(xx + 0.5f, yy + 0.5f, cx, cy, hx, hy, rr);
            float c = cov_from_sdf(fabsf(d) - half);   /* band around the edge */
            if (c > 0.0f) plot_max(xx, yy, (uint8_t)((float)gs * c + 0.5f));
        }
}

void oled_rect_fill(int x, int y, int w, int h, uint8_t gs) {
    if (w <= 0 || h <= 0) return;
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx)
            plot(xx, yy, gs);
}

void oled_pill(int x, int y, int w, int h, uint8_t gs) {
    /* Pill = anti-aliased rounded rect with full-height corner radius. */
    oled_rrect_fill(x, y, w, h, h / 2, gs);
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

/* Sample the 8x8 glyph bitmap as a float field (1 = pixel on) with clamping. */
static float glyph_at(const uint8_t *rows, int gx, int gy) {
    if (gx < 0) gx = 0;
    if (gx > 7) gx = 7;
    if (gy < 0) gy = 0;
    if (gy > 7) gy = 7;
    return (rows[gy] & (uint8_t)(0x80u >> gx)) ? 1.0f : 0.0f;
}

static void blit_glyph_smooth(int x, int y, const uint8_t *rows, uint8_t gs, int scale) {
    if (!rows) return;
    int cell = 8 * scale;
    float inv = 1.0f / (float)scale;
    for (int ty = 0; ty < cell; ++ty) {
        /* source-space centre of this output pixel */
        float sy = ((float)ty + 0.5f) * inv - 0.5f;
        int   y0 = (int)((sy < 0) ? sy - 1 : sy);
        float fy = sy - (float)y0;
        for (int tx = 0; tx < cell; ++tx) {
            float sx = ((float)tx + 0.5f) * inv - 0.5f;
            int   x0 = (int)((sx < 0) ? sx - 1 : sx);
            float fx = sx - (float)x0;
            /* bilinear blend of the 4 surrounding glyph cells */
            float a = glyph_at(rows, x0,   y0);
            float b = glyph_at(rows, x0+1, y0);
            float c = glyph_at(rows, x0,   y0+1);
            float d = glyph_at(rows, x0+1, y0+1);
            float top = a + (b - a) * fx;
            float bot = c + (d - c) * fx;
            float cov = top + (bot - top) * fy;        /* 0..1 coverage */
            if (cov > 0.04f) {
                int g = (int)((float)gs * cov + 0.5f);
                if (g > 0) plot(x + tx, y + ty, (uint8_t)g);
            }
        }
    }
}

void oled_text_smooth(int x, int y, const char *s, uint8_t gs, int scale) {
    if (scale < 2) { oled_text_scaled(x, y, s, gs, scale < 1 ? 1 : scale); return; }
    int step = 8 * scale;
    while (*s) { blit_glyph_smooth(x, y, font_glyph(*s), gs, scale); x += step; ++s; }
}
