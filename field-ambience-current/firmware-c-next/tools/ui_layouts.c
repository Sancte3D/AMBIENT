/*
 * ui_layouts — implementation. See ui_layouts.h for the legibility arithmetic
 * that produced these three candidates.
 *
 * Everything is composited one scanline at a time straight over the plate, so
 * every primitive here takes the current `y` and draws only the part of itself
 * that intersects that row. There is no framebuffer to draw into and no
 * back-to-front ordering to maintain — the cost of the whole UI is one 640-byte
 * line buffer.
 */
#include "ui_layouts.h"
#include "plate_plain.h"
#include "baked_font.h"
#include "oled.h"
#include "ui_draw.h"

#include <string.h>
#include <stdio.h>

/* ---- palette, sampled from the approved artwork ------------------------- */
#define MINT_R  12
#define MINT_G 250
#define MINT_B 149
#define INK_R    6         /* caption sitting inside a mint fill */
#define INK_G   70
#define INK_B   44

/* ---- geometry ------------------------------------------------------------
 * The glass card's bright edge was measured on the 1920x1020 master at
 * x 181..1737, y 13..1008; divided by 6 that is x 30..289, y 2..168. Content
 * is inset from there. CX0 = 47 is the same left margin the approved artwork
 * uses for its title and bars, so type keeps landing where the design put it. */
#define CX0  47
#define CX1 273
#define CY0  14
#define CY1 156

#define ALPHA_FULL   255
#define ALPHA_LABEL  160
#define ALPHA_CTX     96   /* neighbouring rows in UI_CONTEXT */
#define ALPHA_TRACK   28   /* empty bar track: white, barely there */
#define ALPHA_DIMPILL 60

/* ---- parameter model (mirrors include/menu.h + src/menu.c) --------------- */
static const char *const WORLD_NAMES[] = {
    "Tokyo City", "Crystal Coast", "Midnight Drive", "After Hours"
};
static const char *const KEY_NAMES[]   = { "C","C#","D","D#","E","F",
                                           "F#","G","G#","A","A#","B" };
static const char *const TUNING_NAMES[]= { "Equal", "Just" };
static const char *const VOICE_NAMES[] = { "Pad", "String", "Glass", "Ember" };
static const char *const SYNTH_NAMES[] = { "Ambient","Acid","FM Glass","Mist",
                                           "Storm","Orbit","Bamboo" };
static const char *const CELL_NAMES[]  = { "Note", "Harmony", "Land" };
static const char *const BASS_NAMES[]  = { "Off", "Root", "Fifth", "Drift" };
static const char *const COLOR_NAMES[] = { "Pure", "Open", "Warm", "Deep" };
static const char *const FX_NAMES[]    = { "Bypass","Reverb","Delay","Chorus",
                                           "Tape","Swell","Shimmer","Blur","Dream" };

/* `short_label` exists for UI_PAGES only, where a label and a value share one
 * 226 px line. At 12 px per Bitcount advance, "Atmosphere" alone is 120 px and
 * "Midnight Drive" is 168 — they do not fit together. Shrinking the type is
 * not an option at 12.9', so the label shortens and the value truncates. */
typedef struct {
    const char        *label;
    const char        *shortl;
    const char *const *opts;
    uint8_t            nopts;   /* 0 = continuous 0..100 */
    uint8_t            page;
} uiparam_t;

#define N(a) (uint8_t)(sizeof(a) / sizeof((a)[0]))

static const uiparam_t PARAMS[UI_PARAM_COUNT] = {
    { "World",      "World",  WORLD_NAMES,  N(WORLD_NAMES),  0 },
    { "Key",        "Key",    KEY_NAMES,    N(KEY_NAMES),    1 },
    { "Tuning",     "Tuning", TUNING_NAMES, N(TUNING_NAMES), 1 },
    { "Voice",      "Voice",  VOICE_NAMES,  N(VOICE_NAMES),  0 },
    { "Space",      "Space",  0,            0,               2 },
    { "Shimmer",    "Shimmer",0,            0,               2 },
    { "Atmosphere", "Atmos",  0,            0,               3 },
    { "Motion",     "Motion", 0,            0,               3 },
    { "Age",        "Age",    0,            0,               3 },
    { "Echo",       "Echo",   0,            0,               2 },
    { "Blur",       "Blur",   0,            0,               2 },
    { "Synth",      "Synth",  SYNTH_NAMES,  N(SYNTH_NAMES),  0 },
    { "Cell",       "Cell",   CELL_NAMES,   N(CELL_NAMES),   3 },
    { "Bass",       "Bass",   BASS_NAMES,   N(BASS_NAMES),   1 },
    { "Color",      "Color",  COLOR_NAMES,  N(COLOR_NAMES),  1 },
    { "FX",         "FX",     FX_NAMES,     N(FX_NAMES),     0 },
};

/* Page names must not collide with any parameter name on that page, or the
 * heading reads as a repeated row — "Space / Space 62" was exactly that. */
static const char *const PAGE_NAMES[UI_PAGE_COUNT] = {
    "Sound", "Tone", "Room", "Movement"
};

int         ui_param_options(int p) { return PARAMS[p].nopts; }
const char *ui_param_label(int p)   { return PARAMS[p].label; }
int         ui_param_page(int p)    { return PARAMS[p].page; }
const char *ui_page_name(int page)  { return PAGE_NAMES[page]; }

const char *ui_param_value(const ui_state_t *st, int p)
{
    static char buf[8];
    const uiparam_t *q = &PARAMS[p];
    if (q->nopts) {
        int i = st->val[p];
        if (i >= q->nopts) i = q->nopts - 1;
        return q->opts[i];
    }
    snprintf(buf, sizeof buf, "%d%%", st->val[p]);
    return buf;
}

void ui_init(ui_state_t *st, ui_layout_t layout)
{
    static const uint8_t SEED[UI_PARAM_COUNT] = {
        0, 9, 0, 0, 62, 38, 74, 45, 21, 56, 33, 0, 1, 1, 2, 8
    };
    memset(st, 0, sizeof *st);
    st->layout = layout;
    st->sel    = 4;                 /* Space — a continuous one, shows a bar */
    st->batt   = 78;
    memcpy(st->val, SEED, sizeof SEED);
}

void ui_move(ui_state_t *st, int dir)
{
    int s = (int)st->sel + dir;
    while (s < 0) s += UI_PARAM_COUNT;
    st->sel = (uint8_t)(s % UI_PARAM_COUNT);
}

void ui_edit(ui_state_t *st, int dir, int coarse)
{
    int p = st->sel, n = PARAMS[p].nopts;
    if (n) {
        int i = (int)st->val[p] + dir;
        while (i < 0) i += n;
        st->val[p] = (uint8_t)(i % n);
    } else {
        int v = (int)st->val[p] + dir * (coarse ? 10 : 2);
        if (v < 0)   v = 0;
        if (v > 100) v = 100;
        st->val[p] = (uint8_t)v;
    }
}

/* Scanline primitives (blend, pills, text) live in ui_draw.c — the wheel UI
 * uses the same ones, and a second copy of a glyph blitter is exactly the kind
 * of thing that silently diverges. */

/* ---- shared widgets ----------------------------------------------------- */

/* Continuous 0..100 as a track with a mint fill. */
static void widget_fill(uint16_t *line, int y, int top, int h, int pct)
{
    ui_row_pill(line, y, top, h, CX0, CX1 - CX0, 255, 255, 255, ALPHA_TRACK);
    int w = ((CX1 - CX0) * pct + 50) / 100;
    if (w < h && pct > 0) w = h;
    if (w > 0) ui_row_pill(line, y, top, h, CX0, w, MINT_R, MINT_G, MINT_B, 255);
}

/* Discrete parameter as one pill per option, active one mint. */
static void widget_options(uint16_t *line, int y, int top, int h,
                           int n, int active)
{
    if (n < 1) return;
    int gap   = n > 8 ? 3 : 5;
    int total = CX1 - CX0;
    int w     = (total - (n - 1) * gap) / n;
    if (w < 3) { gap = 2; w = (total - (n - 1) * gap) / n; }
    if (w < 1) w = 1;
    for (int i = 0; i < n; ++i) {
        int x = CX0 + i * (w + gap);
        if (i == active) ui_row_pill(line, y, top, h, x, w, MINT_R, MINT_G, MINT_B, 255);
        else             ui_row_pill(line, y, top - 1, h + 2, x, w, 255, 255, 255, ALPHA_TRACK);
    }
}

/* Where-am-I-in-16 strip. The active pill is 2.4x wider, the same ratio
 * src/menu.c already uses, so the position reads without counting. */
static void widget_position(uint16_t *line, int y, int top, int sel)
{
    const int n = UI_PARAM_COUNT, gap = 4, kact = 24;
    int usable = CX1 - CX0;
    int den = (n - 1) * 10 + kact;
    int ina = (usable - (n - 1) * gap) * 10 / den;
    if (ina < 2) ina = 2;
    int act = (ina * kact + 5) / 10;
    int x = CX0;
    for (int i = 0; i < n; ++i) {
        int w = (i == sel) ? act : ina;
        if (i == sel) ui_row_pill(line, y, top - 1, 7, x, w, MINT_R, MINT_G, MINT_B, 255);
        else          ui_row_pill(line, y, top + 1, 3, x, w, 255, 255, 255, ALPHA_DIMPILL);
        x += w + gap;
    }
}

static void widget_battery(uint16_t *line, int y, int top, int pct)
{
    const int w = 26, h = 12, x = CX1 - w;
    /* shell */
    if (y == top || y == top + h - 1) ui_blend_span(line, x, x + w, 255, 255, 255, 150);
    if (y > top && y < top + h - 1) {
        ui_blend_px(&line[x], 255, 255, 255, 150);
        ui_blend_px(&line[x + w - 1], 255, 255, 255, 150);
        if (y > top + 3 && y < top + h - 4)
            ui_blend_span(line, x + w, x + w + 2, 255, 255, 255, 150);
    }
    /* charge */
    int fw = ((w - 6) * pct + 50) / 100;
    if (y > top + 2 && y < top + h - 3 && fw > 0)
        ui_blend_span(line, x + 3, x + 3 + fw, 255, 255, 255, 210);
}

/* ---- layout A: one parameter, as large as the panel allows -------------- */
static void compose_focus(const ui_state_t *st, int y, uint16_t *line)
{
    int p = st->sel;

    ui_row_text(line, y, 16, CX0, &font_hn_value_small, ui_param_label(p),
             255, 255, 255, ALPHA_LABEL);
    widget_battery(line, y, 18, st->batt);

    char tmp[32];
    const char *v = ui_fit_text(&font_hn_value, ui_param_value(st, p),
                             CX1 - CX0, tmp, sizeof tmp);
    ui_row_text(line, y, 52, CX0, &font_hn_value, v,
             255, 255, 255, st->edit ? 255 : 230);

    int n = ui_param_options(p);
    if (n) widget_options(line, y, 106, 12, n, st->val[p]);
    else   widget_fill(line, y, 106, 14, st->val[p]);

    widget_position(line, y, 142, p);
}

/* ---- layout B: the selected parameter, plus one neighbour each way ------ */
static void ctx_row(const ui_state_t *st, int y, uint16_t *line, int p, int ytop)
{
    char tmp[32];
    const bakedfont_t *f = &font_hn_value_small;
    ui_row_text(line, y, ytop, CX0, f, ui_param_label(p),
             255, 255, 255, ALPHA_CTX);
    const char *v = ui_fit_text(f, ui_param_value(st, p), 132, tmp, sizeof tmp);
    ui_row_text(line, y, ytop, CX1 - ui_text_w(f, v), f, v, 255, 255, 255, ALPHA_CTX);
}

static void compose_context(const ui_state_t *st, int y, uint16_t *line)
{
    int p    = st->sel;
    int prev = (p + UI_PARAM_COUNT - 1) % UI_PARAM_COUNT;
    int next = (p + 1) % UI_PARAM_COUNT;

    ctx_row(st, y, line, prev, 10);

    /* Label sits ABOVE the value, not beside it. Beside it costs the value
     * ~90 px of width, which is what turned "Tokyo City" into "Tokyo Ci" —
     * and truncating the one thing the screen exists to show is the wrong
     * trade. Stacked, the value keeps the full 226 px. The two line boxes
     * overlap by 2 px; the glyphs do not (label glyphs end at y=55, value
     * glyphs start at y=66). */
    ui_row_text(line, y, 38, CX0, &font_hn_value_small, ui_param_label(p),
             255, 255, 255, ALPHA_LABEL);

    char tmp[32];
    const char *v = ui_fit_text(&font_hn_value, ui_param_value(st, p),
                             CX1 - CX0, tmp, sizeof tmp);
    ui_row_text(line, y, 60, CX0, &font_hn_value, v, 255, 255, 255, 255);

    int n = ui_param_options(p);
    if (n) widget_options(line, y, 102, 10, n, st->val[p]);
    else   widget_fill(line, y, 102, 12, st->val[p]);

    ctx_row(st, y, line, next, 120);
    widget_position(line, y, 150, p);
}

/* ---- layout C: 4 pages of 4 -------------------------------------------- */
#define PAGE_ROW_Y0    46
#define PAGE_ROW_PITCH 28

static void compose_pages(const ui_state_t *st, int y, uint16_t *line)
{
    int page = ui_param_page(st->sel);

    ui_row_text(line, y, CY0, CX0, &font_hn_value_small, ui_page_name(page),
             255, 255, 255, 210);

    /* page dots, top right — 4 of them, current one mint and wider */
    for (int i = 0; i < UI_PAGE_COUNT; ++i) {
        int w = (i == page) ? 14 : 6;
        int x = CX1 - (UI_PAGE_COUNT - i) * 18 + (18 - w);
        if (i == page) ui_row_pill(line, y, CY0 + 8, 6, x, w, MINT_R, MINT_G, MINT_B, 255);
        else           ui_row_pill(line, y, CY0 + 9, 4, x, w, 255, 255, 255, 110);
    }

    const bakedfont_t *f = &font_hn_value_small;
    int slot = 0;
    for (int p = 0; p < UI_PARAM_COUNT; ++p) {
        if (ui_param_page(p) != page) continue;
        int ytop = PAGE_ROW_Y0 + slot * PAGE_ROW_PITCH;
        int sel  = (p == st->sel);
        int a    = sel ? 255 : 150;

        if (sel)   /* focus marker: a mint tick in the left margin */
            ui_row_pill(line, y, ytop + 3, 18, CX0 - 13, 4,
                     MINT_R, MINT_G, MINT_B, 255);

        int lw = ui_text_w(f, PARAMS[p].shortl);
        ui_row_text(line, y, ytop, CX0, f, PARAMS[p].shortl,
                 sel ? MINT_R : 255, sel ? MINT_G : 255, sel ? MINT_B : 255, a);

        char tmp[32];
        const char *v = ui_fit_text(f, ui_param_value(st, p),
                                 CX1 - CX0 - lw - 12, tmp, sizeof tmp);
        ui_row_text(line, y, ytop, CX1 - ui_text_w(f, v), f, v, 255, 255, 255, a);

        if (!ui_param_options(p)) {   /* thin amount rule under the row */
            int by = ytop + 24;
            ui_row_pill(line, y, by, 3, CX0, CX1 - CX0, 255, 255, 255, ALPHA_TRACK);
            int w = ((CX1 - CX0) * st->val[p] + 50) / 100;
            if (w > 0) ui_row_pill(line, y, by, 3, CX0, w,
                                MINT_R, MINT_G, MINT_B, sel ? 255 : 170);
        }
        ++slot;
    }
}

/* ---- entry point -------------------------------------------------------- */
void ui_compose_row(const ui_state_t *st, int y, uint16_t *line)
{
    memcpy(line, &plate_plain[(size_t)y * OLED_WIDTH],
           OLED_WIDTH * sizeof(uint16_t));

    switch (st->layout) {
        case UI_FOCUS:   compose_focus(st, y, line);   break;
        case UI_CONTEXT: compose_context(st, y, line); break;
        case UI_PAGES:   compose_pages(st, y, line);   break;
        default: break;
    }
}
