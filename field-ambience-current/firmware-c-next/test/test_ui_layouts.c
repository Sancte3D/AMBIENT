/*
 * test_ui_layouts — deterministic guards for the things that made the first
 * 320x170 UI unreadable.
 *
 * Two failures are being locked out here.
 *
 * 1. OFF-GRID TYPE. Bitcount is a grid font on a 0.1 em module, so a ppem that
 *    is not a multiple of 10 puts every dot on a fractional pixel and the text
 *    turns to mush. The three baked faces must stay at 10/20/30 ppem, which
 *    shows up as digit heights of exactly 6/12/20 px. Those heights are what
 *    the legibility arithmetic in ui_layouts.h is built on (0.125 mm/px ->
 *    6.4'/12.9'/21.4' at 40 cm), so if they drift the whole argument moves and
 *    a human has to look again.
 *
 * 2. CONTENT ESCAPING THE CARD. Every layout draws over a plate whose glass
 *    card is a fixed rectangle. A value that is one character too long, or a
 *    right-aligned run measured with the wrong face, silently walks off the
 *    card and onto the glow — which reads as a rendering bug on hardware and
 *    is invisible in a 3x preview. Composing every layout in every selection
 *    and checking that no modified pixel leaves the content box catches it.
 */
#include "../tools/ui_layouts.h"
#include "plate_plain.h"
#include "baked_font.h"
#include "oled.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond, ...) do {                              \
    if (!(cond)) { printf("FAIL: "); printf(__VA_ARGS__);   \
                   printf("\n"); ++failures; }              \
} while (0)

/* Content box, from ui_layouts.c. The left bound allows the UI_PAGES focus
 * tick at CX0-13; the right bound allows the battery nub at CX1+2. */
#define BOX_X0 (47 - 13)
#define BOX_X1 (273 + 2)
#define BOX_Y0 (14 - 2)
#define BOX_Y1 (156 + 2)

static int digit_h(const bakedfont_t *f)
{
    return f->glyphs['0' - f->first].h;
}

static void test_font_grid(void)
{
    CHECK(digit_h(&font_hn_label) == 6,
          "font_hn_label digit height %d, expected 6 (ppem 10)",
          digit_h(&font_hn_label));
    CHECK(digit_h(&font_hn_value_small) == 12,
          "font_hn_value_small digit height %d, expected 12 (ppem 20)",
          digit_h(&font_hn_value_small));
    CHECK(digit_h(&font_hn_value) == 20,
          "font_hn_value digit height %d, expected 20 (ppem 30)",
          digit_h(&font_hn_value));
}

/* Compose one screen and report the bounding box of everything that differs
 * from the bare plate, plus how many pixels changed. */
static void drawn_bounds(const ui_state_t *st,
                         int *x0, int *y0, int *x1, int *y1, long *n)
{
    uint16_t line[OLED_WIDTH];
    *x0 = OLED_WIDTH; *y0 = OLED_HEIGHT; *x1 = -1; *y1 = -1; *n = 0;
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        ui_compose_row(st, y, line);
        for (int x = 0; x < OLED_WIDTH; ++x) {
            if (line[x] == plate_plain[(size_t)y * OLED_WIDTH + x]) continue;
            if (x < *x0) *x0 = x;
            if (x > *x1) *x1 = x;
            if (y < *y0) *y0 = y;
            if (y > *y1) *y1 = y;
            ++*n;
        }
    }
}

static void test_stays_in_card(void)
{
    static const char *NAME[UI_LAYOUT_COUNT] = { "FOCUS", "CONTEXT", "PAGES" };

    for (int l = 0; l < UI_LAYOUT_COUNT; ++l) {
        for (int p = 0; p < UI_PARAM_COUNT; ++p) {
            for (int edit = 0; edit <= 1; ++edit) {
                ui_state_t st;
                ui_init(&st, (ui_layout_t)l);
                st.sel  = (uint8_t)p;
                st.edit = (uint8_t)edit;

                /* Push discrete parameters to their LAST option: that is the
                 * longest string in most tables and the one that overflows. */
                int nopt = ui_param_options(p);
                if (nopt) st.val[p] = (uint8_t)(nopt - 1);

                int x0, y0, x1, y1; long n;
                drawn_bounds(&st, &x0, &y0, &x1, &y1, &n);

                CHECK(n > 0, "%s/%s draws nothing", NAME[l], ui_param_label(p));
                CHECK(x0 >= BOX_X0 && x1 <= BOX_X1,
                      "%s/%s edit=%d escapes horizontally: x %d..%d, box %d..%d",
                      NAME[l], ui_param_label(p), edit, x0, x1, BOX_X0, BOX_X1);
                CHECK(y0 >= BOX_Y0 && y1 <= BOX_Y1,
                      "%s/%s edit=%d escapes vertically: y %d..%d, box %d..%d",
                      NAME[l], ui_param_label(p), edit, y0, y1, BOX_Y0, BOX_Y1);
            }
        }
    }
}

/* Every parameter must be reachable, and stepping must be closed: 16 moves
 * from any start returns to it. Cheap, but it is the invariant the encoder
 * relies on and it broke once already when MP_COUNT grew. */
static void test_navigation(void)
{
    ui_state_t st;
    ui_init(&st, UI_FOCUS);
    int seen[UI_PARAM_COUNT];
    memset(seen, 0, sizeof seen);
    int start = st.sel;
    for (int i = 0; i < UI_PARAM_COUNT; ++i) { seen[st.sel] = 1; ui_move(&st, +1); }
    CHECK(st.sel == start, "16 forward moves did not return to the start");
    for (int i = 0; i < UI_PARAM_COUNT; ++i)
        CHECK(seen[i], "parameter %d (%s) unreachable", i, ui_param_label(i));

    /* Continuous values clamp, discrete values wrap. */
    ui_init(&st, UI_FOCUS);
    st.sel = 4;                                   /* Space, continuous */
    for (int i = 0; i < 200; ++i) ui_edit(&st, +1, 1);
    CHECK(st.val[4] == 100, "continuous did not clamp high: %d", st.val[4]);
    for (int i = 0; i < 200; ++i) ui_edit(&st, -1, 1);
    CHECK(st.val[4] == 0, "continuous did not clamp low: %d", st.val[4]);

    st.sel = 0;                                   /* World, discrete */
    int n = ui_param_options(0);
    st.val[0] = 0;
    for (int i = 0; i < n; ++i) ui_edit(&st, +1, 0);
    CHECK(st.val[0] == 0, "discrete did not wrap after %d steps: %d", n, st.val[0]);
}

/* Each of the 4 pages must hold exactly 4 parameters, or UI_PAGES either
 * clips a row off the bottom or leaves a hole. */
static void test_pages_balanced(void)
{
    int count[UI_PAGE_COUNT] = { 0 };
    for (int p = 0; p < UI_PARAM_COUNT; ++p) {
        int pg = ui_param_page(p);
        CHECK(pg >= 0 && pg < UI_PAGE_COUNT, "%s on page %d", ui_param_label(p), pg);
        if (pg >= 0 && pg < UI_PAGE_COUNT) ++count[pg];
    }
    for (int i = 0; i < UI_PAGE_COUNT; ++i)
        CHECK(count[i] == UI_PARAM_COUNT / UI_PAGE_COUNT,
              "page %d (%s) holds %d parameters, expected %d",
              i, ui_page_name(i), count[i], UI_PARAM_COUNT / UI_PAGE_COUNT);
}

int main(void)
{
    test_font_grid();
    test_stays_in_card();
    test_navigation();
    test_pages_balanced();

    if (failures) {
        printf("test_ui_layouts: %d failure(s)\n", failures);
        return 1;
    }
    printf("test_ui_layouts: OK — font grid, card bounds (%d screens), "
           "navigation, page balance\n",
           UI_LAYOUT_COUNT * UI_PARAM_COUNT * 2);
    return 0;
}
