/*
 * ui_draw — scanline drawing primitives shared by every 320x170 UI candidate.
 *
 * Everything here takes the current `y` and draws only the part of itself that
 * intersects that row, straight into a 320-entry RGB565 line buffer. There is
 * no framebuffer: on the H743 the RAM budget is at 87 % (D1) and 96 % (D2)
 * against 11 % flash, so a 106 KB colour framebuffer is not available and the
 * whole UI has to cost two line buffers instead.
 *
 * Two rules that are not style preferences:
 *
 *   - Blending expands 5/6/5 back to 8 bit before mixing. Blending in packed
 *     form makes every overlay drift dark, and overlays stack here.
 *   - Text is drawn with BINARY coverage, geometry with antialiased coverage.
 *     Bitcount is a grid font; antialiasing smears its lattice and is what made
 *     an earlier render illegible. Geometry is the opposite case — a branch
 *     rotating without AA crawls.
 */
#ifndef FAM_UI_DRAW_H
#define FAM_UI_DRAW_H

#include <stdint.h>
#include "baked_font.h"

static inline uint16_t ui_pack565(int r, int g, int b)
{
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

/* Blend (r,g,b) at coverage `a` (0..255) over one packed pixel. */
void ui_blend_px(uint16_t *px, int r, int g, int b, int a);
void ui_blend_span(uint16_t *line, int x0, int x1, int r, int g, int b, int a);

/* Horizontal inset of a rounded cap on scanline `dy` of an `h`-tall pill. */
int  ui_cap_inset(int dy, int h);

/* One row of a rounded pill; draws nothing when `y` falls outside it. */
void ui_row_pill(uint16_t *line, int y, int top, int h, int x, int w,
                 int r, int g, int b, int a);

/* Total advance width of `s`. */
int  ui_text_w(const bakedfont_t *f, const char *s);

/* Copy as much of `s` as fits in `maxw` into `buf` — hard cut, no ellipsis:
 * at this size an ellipsis costs a whole glyph and reads as noise. Returns `s`
 * itself when it already fits. */
const char *ui_fit_text(const bakedfont_t *f, const char *s, int maxw,
                        char *buf, int bufsz);

/* One scanline of a text run whose line box starts at `ytop`. */
void ui_row_text(uint16_t *line, int y, int ytop, int x,
                 const bakedfont_t *f, const char *s,
                 int r, int g, int b, int a);

#endif
