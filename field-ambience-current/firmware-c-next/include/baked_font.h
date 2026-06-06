#ifndef FAM_BAKED_FONT_H
#define FAM_BAKED_FONT_H

/*
 * Baked Helvetica Neue fonts — Step 12b #7 (premium UI text).
 *
 * Glyphs are pre-rendered from the OTF at exact pixel sizes with anti-aliasing,
 * quantised to 4-bit grey and packed 2 px/byte (row-major, continuous nibble
 * stream). The Pico blits them 1:1 — NO runtime scaling — straight into the
 * SSD1322's grey framebuffer. This is the "native resolution + AA + no scaling"
 * path for maximum sharpness on a grayscale OLED.
 *
 * Three faces are baked (see tools/generate_fonts.py):
 *   font_hn_light40  — big value, primary
 *   font_hn_light24  — big value, fallback for long words
 *   font_hn_thin14   — labels / secondary text
 */

#include <stdint.h>

typedef struct {
    uint8_t  adv;     /* pen advance (px) */
    int8_t   left;    /* x bearing from pen */
    int8_t   top;     /* rows above baseline */
    uint8_t  w, h;    /* bitmap size */
    uint32_t off;     /* start offset in `data`, in NIBBLES (4-bit pixels) */
} bglyph_t;

typedef struct {
    uint8_t        ascent;   /* px from line top to baseline */
    uint8_t        line;     /* line height (px) */
    int            first;    /* first char code (32) */
    int            count;    /* glyph count */
    const bglyph_t *glyphs;  /* indexed by (char - first) */
    const uint8_t  *data;    /* packed 4-bit coverage */
} bakedfont_t;

extern const bakedfont_t font_hn_light40;
extern const bakedfont_t font_hn_light24;
extern const bakedfont_t font_hn_thin14;

/* Draw `s` with line-top at (x, y_top). Glyph coverage is scaled by maxgs
 * (0..15) so the same font can render bright (active) or dim (inactive).
 * Pixels are max-blended (AA-safe). */
void bfont_draw(const bakedfont_t *f, int x, int y_top, const char *s, uint8_t maxgs);

/* Total advance width of `s` in px (for centring). */
int  bfont_width(const bakedfont_t *f, const char *s);

#endif
