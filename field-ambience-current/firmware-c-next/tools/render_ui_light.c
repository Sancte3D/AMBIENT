/*
 * render_ui_light.c — PROPOSAL preview for the light two-column ("duo") menu.
 *
 * Draws with the REAL device primitives (oled_draw.c) and the REAL baked
 * Helvetica fonts into the REAL 320x170 4-bit framebuffer, so what this
 * writes out is what the panel would show — not a mockup in another tool.
 *
 * Two things are being proposed here:
 *
 * 1. LAYOUT. The current UI shows ONE parameter at a time over a 21-dot pill
 *    row; at 320 px that is 15 px per dot, which reads as a progress bar, not
 *    a map. Here the 21 parameters keep their identity (nothing is merged —
 *    they do different things to the sound) but are laid out as
 *    category-rail + parameter-list, so the whole state is visible at once.
 *
 * 2. LIGHT THEME. The device LUT (oled_color.c) maps grey multiplicatively
 *    onto the accent, so level 0 is always black — a dark theme by
 *    construction. A light theme does NOT need any change to the drawing
 *    code: keep drawing "ink = high level" exactly as today and invert the
 *    LUT instead, so level 0 = paper and level 15 = ink. The per-world accent
 *    survives; it now tints the PAPER instead of the glow.
 *
 * Build: tools/render_ui_light.sh   Output: PPM (4x upscaled).
 */

#include "oled.h"
#include "baked_font.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ---------------------------------------------------------------- layout */

#define RAIL_X      14      /* category label left edge      */
#define RAIL_TRK     3      /* extra tracking, small caps    */
#define RULE_X     110      /* vertical hairline             */
#define LIST_X     126      /* parameter name left edge      */
#define LIST_R     306      /* parameter value right edge    */
#define ROW_TOP     34      /* first row's line top          */
#define ROW_H       21      /* row pitch                     */
#define HEAD_Y       8      /* header line top               */

/* Ink levels. The framebuffer convention is unchanged: HIGH = more ink.
 * Only the LUT decides whether that reads as bright or dark. */
#define INK_FULL    15      /* the one selected item              */
#define INK_HEAD     6      /* header / meta text                 */
#define INK_IDLE     6      /* everything not selected (Giopato)  */
#define INK_RULE     3      /* hairlines                          */

typedef struct { const char *name; const char *value; } row_t;
typedef struct { const char *name; int n; row_t rows[6]; } cat_t;

/* All 21 parameters — nothing merged, nothing dropped. Grouped only for
 * WHERE THEY ARE ON SCREEN, not for what they do. */
static const cat_t CATS[5] = {
    { "FELD", 3, {
        { "World",      "Open Sea" },
        { "Atmosphere", "62%"      },
        { "Cell",       "Harmony"  } } },
    { "HARMONIE", 4, {
        { "Key",    "D"     },
        { "Tuning", "Just"  },
        { "Bass",   "Fifth" },
        { "Color",  "Warm"  } } },
    { "KLANG", 3, {
        { "Voice",     "Bowed" },
        { "Synth",     "Ambient" },
        { "Resonance", "55%"   } } },
    { "FORM", 5, {
        { "Attack",  "40%" },
        { "Release", "65%" },
        { "Motion",  "35%" },
        { "Sweep",   "50%" },
        { "EnvMod",  "30%" } } },
    { "RAUM", 6, {
        { "Space",   "72%"   },
        { "Shimmer", "45%"   },
        { "Echo",    "28%"   },
        { "Blur",    "15%"   },
        { "Age",     "33%"   },
        { "FX",      "Dream" } } },
};

/* ------------------------------------------------------------------ paint */

static void hairline_v(int x, int y0, int y1, uint8_t gs) {
    for (int y = y0; y < y1; ++y) oled_pixel(x, y, gs);
}

/* Small caps with tracking — the reference navigation look. The 8x8 face has
 * no tracking of its own, so advance per character by hand. */
static void small_caps(int x, int y, const char *s, uint8_t gs) {
    char one[2] = { 0, 0 };
    for (const char *p = s; *p; ++p) {
        one[0] = *p;
        oled_text(x, y, one, gs);
        x += 8 + RAIL_TRK;
    }
}
static int small_caps_w(const char *s) {
    int n = (int)strlen(s);
    return n ? n * (8 + RAIL_TRK) - RAIL_TRK : 0;
}

/* One frame: category `ci` open, parameter `pi` selected inside it. */
static void draw(int ci, int pi, int editing, const char *world,
                 const char *world_title) {
    oled_fill(0);                                   /* paper */

    /* header: the loaded WORLD — the one piece of context worth the space */
    small_caps(RAIL_X, HEAD_Y + 4, world, INK_HEAD);

    /* battery, top right — reuse the device widget */
    oled_rrect_stroke(288, HEAD_Y + 3, 22, 11, 3, 1, INK_HEAD);
    oled_rect_fill(310, HEAD_Y + 6, 2, 5, INK_HEAD);
    oled_rrect_fill(290, HEAD_Y + 5, 13, 7, 2, INK_HEAD);

    /* left rail — the five categories */
    for (int i = 0; i < 5; ++i) {
        int y = ROW_TOP + i * ROW_H + 6;      /* centre 8 px against a 20 px row */
        small_caps(RAIL_X, y, CATS[i].name, i == ci ? INK_FULL : INK_IDLE);
    }

    /* hairline between the columns */
    hairline_v(RULE_X, ROW_TOP - 6, ROW_TOP + 5 * ROW_H + 2, INK_RULE);

    /* right column — the parameters of the open category */
    const cat_t *c = &CATS[ci];
    /* While editing, the unselected rows step further back so the big value
     * may overlap them and still read as depth rather than as a collision. */
    const uint8_t idle = editing ? 3 : INK_IDLE;
    for (int i = 0; i < c->n; ++i) {
        int y   = ROW_TOP + i * ROW_H;
        int sel = (i == pi);
        uint8_t gs = sel ? INK_FULL : idle;

        bfont_draw(&font_hn_label, LIST_X, y, c->rows[i].name, gs);

        /* the World row always mirrors the header — same fact, one source */
        const char *val = (ci == 0 && i == 0) ? world_title : c->rows[i].value;

        /* value right-aligned; the selected one is set in the big face so it
         * stays readable at arm's length while playing */
        if (sel && editing) {
            int w = bfont_width(&font_hn_value_small, val);
            bfont_draw(&font_hn_value_small, LIST_R - w, y - 8,
                       val, INK_FULL);
        } else {
            int w = bfont_width(&font_hn_label, val);
            bfont_draw(&font_hn_label, LIST_R - w, y, val, gs);
        }
    }

    /* selection marker: a short rule under the open category, echoing the
     * column hairline instead of a filled bar (a filled bar needs knockout
     * text, which the max-blend font path cannot do) */
    int my = ROW_TOP + ci * ROW_H + 17;
    int mw = small_caps_w(CATS[ci].name);
    for (int x = RAIL_X; x < RAIL_X + mw; ++x) oled_pixel(x, my, INK_FULL);
}

/* -------------------------------------------------------------- LUT + out */

/* Paper/ink LUT: level 0 -> paper (accent-tinted near-white),
 * level 15 -> ink (near-black). This is the whole light theme. */
static void build_lut(uint16_t lut[16], int pr, int pg, int pb) {
    const int ir = 0x1c, ig = 0x1c, ib = 0x1e;      /* ink */
    for (int n = 0; n < 16; ++n) {
        int r = pr + (ir - pr) * n / 15;
        int g = pg + (ig - pg) * n / 15;
        int b = pb + (ib - pb) * n / 15;
        lut[n] = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
    }
}

#define UP 4
static void write_ppm(const char *path, const uint16_t lut[16]) {
    const uint8_t *fb = oled_framebuffer();
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fprintf(f, "P6\n%d %d\n255\n", OLED_WIDTH * UP, OLED_HEIGHT * UP);
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        for (int rep = 0; rep < UP; ++rep) {
            for (int x = 0; x < OLED_WIDTH; ++x) {
                int idx = y * OLED_WIDTH + x;
                uint8_t nib = (idx & 1) ? (fb[idx >> 1] & 0x0f)
                                        : (fb[idx >> 1] >> 4);
                uint16_t c = lut[nib];
                uint8_t rgb[3] = { (uint8_t)(((c >> 11) & 0x1f) << 3),
                                   (uint8_t)(((c >> 5)  & 0x3f) << 2),
                                   (uint8_t)(( c        & 0x1f) << 3) };
                for (int k = 0; k < UP; ++k) fwrite(rgb, 1, 3, f);
            }
        }
    }
    fclose(f);
}

int main(int argc, char **argv) {
    const char *dir = argc > 1 ? argv[1] : ".";
    char path[512];

    /* Per-world paper tints — the accent now colours the paper, so each world
     * still has its own light. Values are deliberately close to white; the
     * reference look lives on contrast, not on saturation. */
    struct { const char *tag; int r, g, b; int ci, pi, ed; const char *world, *title; } shots[] = {
        { "01_feld_opensea",  0xec, 0xee, 0xef, 0, 0, 0, "OPEN SEA", "Open Sea" },
        { "02_klang",         0xec, 0xee, 0xef, 2, 2, 0, "OPEN SEA", "Open Sea" },
        { "03_form_edit",     0xec, 0xee, 0xef, 3, 3, 1, "OPEN SEA", "Open Sea" },
        { "04_raum",          0xec, 0xee, 0xef, 4, 5, 0, "OPEN SEA", "Open Sea" },
        { "05_desert_paper",  0xf2, 0xed, 0xe3, 0, 0, 0, "DESERT", "Desert" },
        { "06_moss_paper",    0xe9, 0xee, 0xea, 1, 1, 0, "MOSS FIELDS", "Moss Fields" },
    };

    for (unsigned i = 0; i < sizeof shots / sizeof shots[0]; ++i) {
        uint16_t lut[16];
        build_lut(lut, shots[i].r, shots[i].g, shots[i].b);
        draw(shots[i].ci, shots[i].pi, shots[i].ed, shots[i].world, shots[i].title);
        snprintf(path, sizeof path, "%s/ui_%s.ppm", dir, shots[i].tag);
        write_ppm(path, lut);
    }
    printf("wrote %zu frames to %s/\n", sizeof shots / sizeof shots[0], dir);
    return 0;
}
