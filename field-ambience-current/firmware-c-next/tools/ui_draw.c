/* ui_draw — see ui_draw.h. */
#include "ui_draw.h"
#include "oled.h"

#include <math.h>

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

/* ---- signed-distance scanline primitives --------------------------------
 * Primitives do NOT blend into the line buffer. They accumulate COVERAGE into
 * a per-row byte mask with max(), and a whole group of same-coloured shapes is
 * blended once at the end.
 *
 * That is not an optimisation, it is the fix for a visible seam. Compositing
 * two overlapping shapes of the same colour in sequence never reaches full
 * opacity: where each covers half a pixel the result lands at 0.75 of the
 * colour, so every junction — branch into ring, stem into node, the four
 * strokes crossing in the FX icon — drew itself a darker hairline. Taking the
 * maximum of the coverages first makes a union behave like one shape.
 */
int ui_cov255(float d)          /* d = signed distance in px */
{
    float c = 0.5f - d;
    if (c <= 0.0f) return 0;
    if (c >= 1.0f) return 255;
    return (int)(c * 255.0f + 0.5f);
}

void ui_cov_put(uint8_t *cov, int x, int c)
{
    if (c > cov[x]) cov[x] = (uint8_t)c;
}

/* Blend the accumulated mask in one pass and clear it for the next group. */
void ui_cov_flush(uint16_t *line, uint8_t *cov, int cr, int cg, int cb,
                      int alpha)
{
    for (int x = 0; x < OLED_WIDTH; ++x) {
        if (cov[x]) {
            ui_blend_px(&line[x], cr, cg, cb, cov[x] * alpha / 255);
            cov[x] = 0;
        }
    }
}

void ui_cov_disc(uint8_t *cov, int y, float cx, float cy, float r)
{
    float dy = (float)y - cy;
    if (dy < -r - 1.0f || dy > r + 1.0f) return;
    int x0 = (int)(cx - r - 1.0f), x1 = (int)(cx + r + 2.0f);
    if (x0 < 0) x0 = 0;
    if (x1 > OLED_WIDTH) x1 = OLED_WIDTH;
    for (int x = x0; x < x1; ++x) {
        float dx = (float)x - cx;
        ui_cov_put(cov, x, ui_cov255(sqrtf(dx * dx + dy * dy) - r));
    }
}

/* Capsule: the branch. Distance to the segment, minus the half width. */
void ui_cov_capsule(uint8_t *cov, int y, float ax, float ay,
                        float bx, float by, float r)
{
    float ylo = (ay < by ? ay : by) - r - 1.0f;
    float yhi = (ay > by ? ay : by) + r + 1.0f;
    if ((float)y < ylo || (float)y > yhi) return;

    float xlo = (ax < bx ? ax : bx) - r - 1.0f;
    float xhi = (ax > bx ? ax : bx) + r + 2.0f;
    int x0 = (int)xlo, x1 = (int)xhi;
    if (x0 < 0) x0 = 0;
    if (x1 > OLED_WIDTH) x1 = OLED_WIDTH;

    float ex = bx - ax, ey = by - ay;
    float ee = ex * ex + ey * ey;
    if (ee < 1e-6f) ee = 1e-6f;

    for (int x = x0; x < x1; ++x) {
        float px = (float)x - ax, py = (float)y - ay;
        float t = (px * ex + py * ey) / ee;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        float qx = px - ex * t, qy = py - ey * t;
        ui_cov_put(cov, x, ui_cov255(sqrtf(qx * qx + qy * qy) - r));
    }
}

/* Arc of an annulus, angles measured from 12 o'clock, positive clockwise.
 * The ends are rounded so a value fill terminates like the orb, not like a
 * cut. */
void ui_cov_arc(uint8_t *cov, int y, float cx, float cy, float r,
                    float half_t, float a0, float a1)
{
    float ro = r + half_t;
    float dy = (float)y - cy;
    if (dy >= -ro - 1.0f && dy <= ro + 1.0f) {
        int x0 = (int)(cx - ro - 1.0f), x1 = (int)(cx + ro + 2.0f);
        if (x0 < 0) x0 = 0;
        if (x1 > OLED_WIDTH) x1 = OLED_WIDTH;
        for (int x = x0; x < x1; ++x) {
            float dx = (float)x - cx;
            float dist = sqrtf(dx * dx + dy * dy);
            float dr = fabsf(dist - r) - half_t;
            if (dr > 0.75f) continue;                 /* outside the band */
            float ang = atan2f(dx, -dy) / UI_DEG2RAD;    /* 0 = up, + = right */
            if (ang >= a0 && ang <= a1) ui_cov_put(cov, x, ui_cov255(dr));
        }
    }
    for (int e = 0; e < 2; ++e) {                     /* rounded caps */
        float a = (e ? a1 : a0) * UI_DEG2RAD;
        ui_cov_disc(cov, y, cx + r * sinf(a), cy - r * cosf(a), half_t);
    }
}


/* A stroked open polyline: the workhorse of monoline drawing, and 297 of the
 * 304 elements in the OP-1 screen this vocabulary was measured from are
 * exactly this. Chained capsules go into ONE coverage mask, so the joints are
 * a union rather than a stack of overlapping strokes — without that, every
 * corner draws itself a darker dot. */
void ui_cov_polyline(uint8_t *cov, int y, const float *pts, int n, float half_w)
{
    for (int i = 0; i + 1 < n; ++i)
        ui_cov_capsule(cov, y, pts[i * 2], pts[i * 2 + 1],
                       pts[(i + 1) * 2], pts[(i + 1) * 2 + 1], half_w);
}
