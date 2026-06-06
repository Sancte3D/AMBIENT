#ifndef FAM_VFONT_H
#define FAM_VFONT_H

/*
 * Vector font — Step 12b #7 (high-quality value display).
 *
 * Each glyph is a set of centre-line polyline strokes on a 0..10 (x) × 0..14
 * (y) em grid. Rendering is signed-distance-field: per output pixel, coverage
 * = how close it is to the nearest stroke, mapped to the SSD1322's 4-bit grey.
 * Result is crisp AND anti-aliased at ANY size — no pixel blocks, no blur.
 *
 * Uppercase A–Z, digits 0–9, and the few symbols the menu uses (# % - / .
 * space). Lowercase falls back to uppercase.
 */

#include <stdint.h>

/* Draw `s` so the cap height is `height_px`, top-left at (x, y_top), grey gs.
 * Pixels are max-blended (AA-safe). */
void vfont_draw(int x, int y_top, const char *s, int height_px, uint8_t gs);

/* Pixel width of `s` at the given cap height (for centring). */
int  vfont_width(const char *s, int height_px);

#endif
