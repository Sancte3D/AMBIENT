/*
 * ui_layouts — three candidate information densities for the 320x170 panel,
 * all drawn in the approved visual language.
 *
 * WHY THREE. The measured limit decides more than taste here. Active area is
 * 39.1 x 21.2 mm over 320 x 170 px, so one pixel is 0.125 mm. The baked
 * Bitcount faces give digit heights of 6 / 12 / 18 px = 0.75 / 1.50 / 2.24 mm,
 * which at a 40 cm viewing distance subtend 6.4' / 12.9' / 19.3'. Comfortable
 * reading starts around 20'; 5' is the bare acuity limit of a healthy eye.
 * The 6-row list shipped its labels at 6.4' — that is why it could not be read
 * on the panel, and no amount of styling fixes it.
 *
 * A legible row therefore costs the 30-ppem face, 36 px of line height. The
 * panel is 170 px tall. 170 / 36 = 4.7 lines, before a title, margins or a
 * bar exist. Six labelled rows do not fit at any legible size. These three
 * layouts are the honest points on that curve:
 *
 *   UI_FOCUS    one parameter, 19.3'. Position carried by 16 pills, not text.
 *               This is the model src/menu.c already implements.
 *   UI_CONTEXT  the selected parameter at 19.3' with one neighbour above and
 *               below at 12.9'. Keeps a sense of place, shows 3 of 16.
 *   UI_PAGES    16 grouped into 4 pages of 4, all four rows at 12.9'. Closest
 *               to a list; the riskiest legibility at low backlight.
 *
 * All three composite per scanline over the flash-resident plate — no colour
 * framebuffer. See design_bench_pico.c for why that matters on the H743.
 */
#ifndef FAM_UI_LAYOUTS_H
#define FAM_UI_LAYOUTS_H

#include <stdint.h>

typedef enum {
    UI_FOCUS = 0,
    UI_CONTEXT,
    UI_PAGES,
    UI_LAYOUT_COUNT
} ui_layout_t;

#define UI_PARAM_COUNT 16
#define UI_PAGE_COUNT   4

typedef struct {
    ui_layout_t layout;
    uint8_t     sel;                    /* 0..UI_PARAM_COUNT-1               */
    uint8_t     edit;                   /* 0 = browse, 1 = editing the value */
    uint8_t     val[UI_PARAM_COUNT];    /* 0..100, or option index if discrete */
    uint8_t     batt;                   /* 0..100                            */
} ui_state_t;

/* Number of discrete options, or 0 when the parameter is a continuous 0..100. */
int         ui_param_options(int p);
const char *ui_param_label(int p);
/* Value as it should read on screen ("72", "Tokyo City", ...). Returns a
 * pointer to a static buffer for the numeric case — copy if you need to keep
 * two alive at once. */
const char *ui_param_value(const ui_state_t *st, int p);

/* Which page a parameter lives on, and that page's name (UI_PAGES only). */
int         ui_param_page(int p);
const char *ui_page_name(int page);

void ui_init(ui_state_t *st, ui_layout_t layout);

/* Advance/retreat the selection, and step the selected value. Shared by the
 * Pico bench and the host renderer so both behave identically. */
void ui_move(ui_state_t *st, int dir);
void ui_edit(ui_state_t *st, int dir, int coarse);

/* One finished RGB565 scanline: the plate, plus whatever this layout draws
 * at row `y`. `line` holds 320 entries. */
void ui_compose_row(const ui_state_t *st, int y, uint16_t *line);

#endif
