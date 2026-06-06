/*
 * Baked-font renderer — Step 12b #7. Blits pre-rendered AA glyphs 1:1.
 */

#include "baked_font.h"
#include "oled.h"

static const bglyph_t *glyph(const bakedfont_t *f, char c) {
    int idx = (unsigned char)c - f->first;
    if (idx < 0 || idx >= f->count) idx = ' ' - f->first;   /* fallback: space */
    return &f->glyphs[idx];
}

void bfont_draw(const bakedfont_t *f, int x, int y_top, const char *s, uint8_t maxgs) {
    int pen = x;
    for (; *s; ++s) {
        const bglyph_t *g = glyph(f, *s);
        int bx = pen + g->left;
        int by = y_top + (int)f->ascent - g->top;
        uint32_t off = g->off;
        for (int gy = 0; gy < g->h; ++gy) {
            for (int gx = 0; gx < g->w; ++gx) {
                uint32_t idx = off + (uint32_t)gy * g->w + gx;
                uint8_t byte = f->data[idx >> 1];
                uint8_t nib  = (idx & 1) ? (byte & 0x0F) : (byte >> 4);
                if (nib) {
                    int v = (nib * maxgs + 7) / 15;        /* scale to maxgs */
                    if (v > 0) oled_pixel_max(bx + gx, by + gy, (uint8_t)v);
                }
            }
        }
        pen += g->adv;
    }
}

int bfont_width(const bakedfont_t *f, const char *s) {
    int w = 0;
    for (; *s; ++s) w += glyph(f, *s)->adv;
    return w;
}
