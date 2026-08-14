/* ui_draw — see ui_draw.h. */
#include "ui_draw.h"
#include "oled.h"

#include <string.h>

void ui_blend_px(uint16_t *px, int r, int g, int b, int a)
{
    if (a <= 0) return;
    if (a > 255) a = 255;
    int dr = (*px >> 11) & 0x1F, dg = (*px >> 5) & 0x3F, db = *px & 0x1F;
    dr = (dr << 3) | (dr >> 2);
    dg = (dg << 2) | (dg >> 4);
    db = (db << 3) | (db >> 2);
    *px = ui_pack565(dr + ((r - dr) * a) / 255,
                     dg + ((g - dg) * a) / 255,
                     db + ((b - db) * a) / 255);
}

void ui_blend_span(uint16_t *line, int x0, int x1, int r, int g, int b, int a)
{
    if (x0 < 0) x0 = 0;
    if (x1 > OLED_WIDTH) x1 = OLED_WIDTH;
    for (int x = x0; x < x1; ++x) ui_blend_px(&line[x], r, g, b, a);
}

int ui_cap_inset(int dy, int h)
{
    int rad = h / 2;
    int d = dy < rad ? (rad - dy) : (dy - (h - 1 - rad));
    if (d <= 0) return 0;
    int i = 0;
    while (i < rad && (rad - i) * (rad - i) + d * d > rad * rad) ++i;
    return i;
}

void ui_row_pill(uint16_t *line, int y, int top, int h, int x, int w,
                 int r, int g, int b, int a)
{
    int dy = y - top;
    if (dy < 0 || dy >= h || w <= 0) return;
    int inset = ui_cap_inset(dy, h);
    ui_blend_span(line, x + inset, x + w - inset, r, g, b, a);
}

int ui_text_w(const bakedfont_t *f, const char *s)
{
    int w = 0;
    for (; *s; ++s) {
        int i = (uint8_t)*s - f->first;
        if (i < 0 || i >= f->count) continue;
        w += f->glyphs[i].adv;
    }
    return w;
}

const char *ui_fit_text(const bakedfont_t *f, const char *s, int maxw,
                        char *buf, int bufsz)
{
    if (ui_text_w(f, s) <= maxw) return s;
    int w = 0, n = 0;
    while (s[n] && n < bufsz - 1) {
        int i = (uint8_t)s[n] - f->first;
        int adv = (i >= 0 && i < f->count) ? f->glyphs[i].adv : 0;
        if (w + adv > maxw) break;
        w += adv;
        ++n;
    }
    memcpy(buf, s, (size_t)n);
    buf[n] = 0;
    return buf;
}

void ui_row_text(uint16_t *line, int y, int ytop, int x,
                 const bakedfont_t *f, const char *s,
                 int r, int g, int b, int a)
{
    if (y < ytop || y >= ytop + f->line + 4) return;   /* descenders included */
    int base = ytop + f->ascent;
    int pen  = x;
    for (; *s; ++s) {
        int i = (uint8_t)*s - f->first;
        if (i < 0 || i >= f->count) continue;
        const bglyph_t *gl = &f->glyphs[i];
        int gy = y - (base - gl->top);
        if (gy >= 0 && gy < gl->h) {
            for (int gx = 0; gx < gl->w; ++gx) {
                uint32_t nib = gl->off + (uint32_t)gy * gl->w + gx;
                uint8_t v4 = (nib & 1) ? (f->data[nib >> 1] & 0x0F)
                                       : (f->data[nib >> 1] >> 4);
                if (v4 >= 8) {                       /* binary — never AA */
                    int px = pen + gl->left + gx;
                    if (px >= 0 && px < OLED_WIDTH)
                        ui_blend_px(&line[px], r, g, b, a);
                }
            }
        }
        pen += gl->adv;
    }
}
