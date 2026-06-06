/*
 * Vector font — Step 12b #7. Centre-line stroke glyphs on a 0..10 × 0..14 em
 * grid, rendered with a signed-distance field so they're crisp + anti-aliased
 * at any size. Stream format per glyph: repeated [n, x0,y0 … x(n-1),y(n-1)],
 * terminated by a single 0. Coordinates: x→right, y→down (0 top, 14 baseline).
 */

#include "vfont.h"
#include "oled.h"
#include <math.h>

#define EMH       14.0f
#define STROKE_R  0.95f      /* stroke half-width, em units */
#define AA_BAND   0.85f      /* anti-alias ramp, em units   */

/* ---- glyph stroke data ---- */
static const int8_t G_SP[] = { 0 };
static const int8_t G_A[]  = { 3,1,14,5,0,9,14, 2,3,9,7,9, 0 };
static const int8_t G_B[]  = { 2,1,0,1,14, 5,1,0,6,0,8,2,6,7,1,7, 5,1,7,7,7,9,10,7,14,1,14, 0 };
static const int8_t G_C[]  = { 10,9,3,7,1,4,0,2,1,0,5,0,9,2,13,4,14,7,13,9,11, 0 };
static const int8_t G_D[]  = { 2,1,0,1,14, 6,1,0,5,0,8,3,8,11,5,14,1,14, 0 };
static const int8_t G_E[]  = { 4,8,0,1,0,1,14,8,14, 2,1,7,6,7, 0 };
static const int8_t G_F[]  = { 3,8,0,1,0,1,14, 2,1,7,6,7, 0 };
static const int8_t G_G[]  = { 10,9,3,6,0,3,1,0,4,0,10,3,14,6,14,9,12,9,8, 2,9,8,6,8, 0 };
static const int8_t G_H[]  = { 2,1,0,1,14, 2,9,0,9,14, 2,1,7,9,7, 0 };
static const int8_t G_I[]  = { 2,5,0,5,14, 2,2,0,8,0, 2,2,14,8,14, 0 };
static const int8_t G_J[]  = { 5,8,0,8,11,6,14,3,14,1,12, 0 };
static const int8_t G_K[]  = { 2,1,0,1,14, 2,8,0,1,7, 2,1,7,8,14, 0 };
static const int8_t G_L[]  = { 3,1,0,1,14,8,14, 0 };
static const int8_t G_M[]  = { 5,1,14,1,0,5,6,9,0,9,14, 0 };
static const int8_t G_N[]  = { 4,1,14,1,0,9,14,9,0, 0 };
static const int8_t G_O[]  = { 11,5,0,2,1,0,5,0,9,2,13,5,14,8,13,10,9,10,5,8,1,5,0, 0 };
static const int8_t G_P[]  = { 2,1,14,1,0, 6,1,0,6,0,8,2,8,5,6,7,1,7, 0 };
static const int8_t G_Q[]  = { 11,5,0,2,1,0,5,0,9,2,13,5,14,8,13,10,9,10,5,8,1,5,0, 2,6,10,10,14, 0 };
static const int8_t G_R[]  = { 2,1,14,1,0, 6,1,0,6,0,8,2,8,5,6,7,1,7, 2,4,7,9,14, 0 };
static const int8_t G_S[]  = { 13,9,3,7,1,4,0,2,1,1,3,2,5,5,6,7,8,8,10,7,13,4,14,1,13,0,11, 0 };
static const int8_t G_T[]  = { 2,0,0,10,0, 2,5,0,5,14, 0 };
static const int8_t G_U[]  = { 6,1,0,1,11,3,14,7,14,9,11,9,0, 0 };
static const int8_t G_V[]  = { 3,1,0,5,14,9,0, 0 };
static const int8_t G_W[]  = { 5,1,0,3,14,5,6,7,14,9,0, 0 };
static const int8_t G_X[]  = { 2,1,0,9,14, 2,9,0,1,14, 0 };
static const int8_t G_Y[]  = { 3,1,0,5,7,9,0, 2,5,7,5,14, 0 };
static const int8_t G_Z[]  = { 4,1,0,9,0,1,14,9,14, 0 };

static const int8_t G_0[]  = { 11,4,0,1,2,0,5,0,9,1,12,4,14,7,12,8,9,8,5,7,2,4,0, 2,2,12,6,2, 0 };
static const int8_t G_1[]  = { 3,2,3,4,1,4,14, 2,2,14,6,14, 0 };
static const int8_t G_2[]  = { 8,1,3,2,1,5,0,7,1,8,3,7,6,1,14,8,14, 0 };
static const int8_t G_3[]  = { 3,1,1,7,1,4,6, 6,4,6,7,8,7,11,5,14,2,14,0,12, 0 };
static const int8_t G_4[]  = { 4,6,14,6,0,0,9,8,9, 0 };
static const int8_t G_5[]  = { 9,8,0,2,0,1,6,5,6,7,8,7,11,5,14,2,14,0,12, 0 };
static const int8_t G_6[]  = { 12,7,1,4,0,2,1,0,5,0,11,2,14,5,14,7,12,7,9,5,7,2,7,0,9, 0 };
static const int8_t G_7[]  = { 3,0,0,8,0,3,14, 0 };
static const int8_t G_8[]  = { 9,4,6,2,5,2,2,4,0,6,0,8,2,8,5,6,6,4,6, 10,4,6,2,7,1,10,2,13,4,14,6,14,8,13,8,10,6,7,4,6, 0 };
static const int8_t G_9[]  = { 12,1,13,4,14,6,13,8,9,8,3,6,1,3,0,1,2,1,5,3,7,6,7,8,5, 0 };

static const int8_t G_HASH[]  = { 2,3,2,3,12, 2,6,2,6,12, 2,1,6,8,6, 2,1,9,8,9, 0 };
static const int8_t G_PCT[]   = { 2,8,1,1,13, 5,1,2,3,2,3,4,1,4,1,2, 5,6,10,8,10,8,12,6,12,6,10, 0 };
static const int8_t G_DASH[]  = { 2,1,7,7,7, 0 };
static const int8_t G_SLASH[] = { 2,8,1,1,13, 0 };
static const int8_t G_DOT[]   = { 5,4,13,5,13,5,14,4,14,4,13, 0 };

typedef struct { char c; const int8_t *s; int8_t width; } vglyph_t;
static const vglyph_t GLYPHS[] = {
    {' ',G_SP,4},
    {'A',G_A,10},{'B',G_B,10},{'C',G_C,10},{'D',G_D,10},{'E',G_E,9},{'F',G_F,9},
    {'G',G_G,10},{'H',G_H,10},{'I',G_I,10},{'J',G_J,9},{'K',G_K,9},{'L',G_L,9},
    {'M',G_M,10},{'N',G_N,10},{'O',G_O,10},{'P',G_P,9},{'Q',G_Q,11},{'R',G_R,10},
    {'S',G_S,10},{'T',G_T,10},{'U',G_U,10},{'V',G_V,10},{'W',G_W,10},{'X',G_X,10},
    {'Y',G_Y,10},{'Z',G_Z,10},
    {'0',G_0,9},{'1',G_1,8},{'2',G_2,9},{'3',G_3,9},{'4',G_4,9},{'5',G_5,9},
    {'6',G_6,9},{'7',G_7,9},{'8',G_8,9},{'9',G_9,9},
    {'#',G_HASH,9},{'%',G_PCT,9},{'-',G_DASH,8},{'/',G_SLASH,9},{'.',G_DOT,5},
};
#define N_GLYPHS (int)(sizeof GLYPHS / sizeof GLYPHS[0])
#define GLYPH_GAP 3       /* em units between glyphs */

static char up(char c) { return (c >= 'a' && c <= 'z') ? (char)(c - 32) : c; }

static const vglyph_t *lookup(char c) {
    c = up(c);
    for (int i = 0; i < N_GLYPHS; ++i) if (GLYPHS[i].c == c) return &GLYPHS[i];
    return &GLYPHS[0];     /* space */
}

/* squared distance from point (px,py) to segment (ax,ay)-(bx,by). */
static float dist2_seg(float px, float py, float ax, float ay, float bx, float by) {
    float vx = bx - ax, vy = by - ay;
    float wx = px - ax, wy = py - ay;
    float c1 = vx * wx + vy * wy;
    float t = 0.0f;
    if (c1 > 0) {
        float c2 = vx * vx + vy * vy;
        t = (c1 >= c2) ? 1.0f : c1 / c2;
    }
    float dx = px - (ax + t * vx), dy = py - (ay + t * vy);
    return dx * dx + dy * dy;
}

/* min distance (em) from (ex,ey) to any stroke segment in the glyph. */
static float glyph_dist(const int8_t *s, float ex, float ey) {
    float best = 1e9f;
    while (*s) {
        int n = *s++;
        float ax = (float)s[0], ay = (float)s[1];
        for (int i = 1; i < n; ++i) {
            float bx = (float)s[i * 2], by = (float)s[i * 2 + 1];
            float d2 = dist2_seg(ex, ey, ax, ay, bx, by);
            if (d2 < best) best = d2;
            ax = bx; ay = by;
        }
        s += n * 2;
    }
    return sqrtf(best);
}

int vfont_width(const char *str, int height_px) {
    float scale = (float)height_px / EMH;
    int total = 0;
    for (const char *p = str; *p; ++p) total += lookup(*p)->width + GLYPH_GAP;
    if (total > 0) total -= GLYPH_GAP;     /* no trailing gap */
    return (int)(total * scale + 0.5f);
}

void vfont_draw(int x, int y_top, const char *str, int height_px, uint8_t gs) {
    float scale = (float)height_px / EMH;
    float curx = (float)x;
    for (const char *p = str; *p; ++p) {
        const vglyph_t *g = lookup(*p);
        if (*g->s) {                       /* has strokes (not space) */
            int gx0 = (int)(curx) - 2;
            int gx1 = (int)(curx + g->width * scale) + 2;
            int gy0 = y_top - 2;
            int gy1 = y_top + height_px + 2;
            for (int py = gy0; py < gy1; ++py) {
                float ey = ((float)py + 0.5f - (float)y_top) / scale;
                for (int px = gx0; px < gx1; ++px) {
                    float ex = ((float)px + 0.5f - curx) / scale;
                    float d = glyph_dist(g->s, ex, ey);
                    float cov = (STROKE_R - d) / AA_BAND + 0.5f;
                    if (cov > 0.0f) {
                        if (cov > 1.0f) cov = 1.0f;
                        oled_pixel_max(px, py, (uint8_t)((float)gs * cov + 0.5f));
                    }
                }
            }
        }
        curx += (g->width + GLYPH_GAP) * scale;
    }
}
