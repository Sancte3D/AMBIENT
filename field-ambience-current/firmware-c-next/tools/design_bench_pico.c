/*
 * design_bench_pico — drive the approved AMBIENT design on a real ST7789,
 * with the bars live.
 *
 * Bench tool for the Pico-2 breadboard, NOT a product build. Its job is to
 * answer one question the desktop preview cannot: does the approved artwork
 * hold up on the actual panel, and does moving a bar still look right.
 *
 * ARCHITECTURE — this is the split the product will use too:
 *
 *   background   the approved design, baked once offline into RGB565 and
 *                living in flash as `plate_bg` (108,800 byte). It carries the
 *                gradient, bloom, title, labels, the 100 % pill and the EMPTY
 *                bar tracks. Never copied to RAM.
 *   foreground   only what actually changes — the mint fills, the value
 *                caption and the selection halo — drawn per row on the way to
 *                the panel.
 *
 * There is no framebuffer. Each row is composited into a 640-byte line buffer
 * straight from flash and pushed to SPI, so the whole UI costs two line
 * buffers instead of a 106 KB colour framebuffer. That matters: on the H743
 * the RAM budget is already at 87 % (D1) and 96 % (D2), while flash sits at
 * 11 %.
 *
 * CONTROLS (single encoder + one button, same wiring as display_hw_test)
 *   rotate        change the selected row's value
 *   push          select the next row
 *   SHIFT + push  select the previous row
 *   SHIFT + rotate  coarse steps (x5)
 *
 * The Key row is discrete: four segments, rotation steps between them.
 */

#include "plate_bg.h"
#include "baked_font.h"
#include "oled.h"

#ifndef DESIGN_BENCH_HOST          /* host harness compiles the compositor only */
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#endif

#include <string.h>
#include <stdint.h>

/* ---- geometry ------------------------------------------------------------
 * Measured off the approved reference (card 1502x970), carried through the
 * master at 1920x1020 and divided by 6. tools/build_display_asset.py prints
 * these same numbers when it bakes the plate — if the artwork moves, both
 * change together. */
#define UI_BAR_X      47
#define UI_TRACK_R    201
#define UI_BAR_H      12
#define UI_ROW_Y0     55
#define UI_ROW_PITCH  16
#define UI_ROWS        6
#define UI_KEY_ROW     3      /* discrete: 4 segments */
#define UI_KEY_SEGS    4
#define UI_KEY_SEGW   36
#define UI_KEY_GAP     3

/* Sampled from the reference artwork, not invented. */
#define MINT_R  12
#define MINT_G 250
#define MINT_B 149
#define CAP_R    6            /* value caption inside the fill */
#define CAP_G   70
#define CAP_B   44

#define RGB565(r, g, b) \
    ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

static const uint16_t MINT565 = RGB565(MINT_R, MINT_G, MINT_B);

/* ---- pins (mirrors display_hw_test so the same breadboard works) --------- */
#ifndef PIN_ENC_CLK
#define PIN_ENC_CLK 2
#endif
#ifndef PIN_ENC_DT
#define PIN_ENC_DT  3
#endif
#ifndef PIN_ENC_SW
#define PIN_ENC_SW  4
#endif
#ifndef PIN_SHIFT
#define PIN_SHIFT   5
#endif
#ifndef PIN_BL
#define PIN_BL     22
#endif

/* ---- state -------------------------------------------------------------- */
static uint8_t s_val[UI_ROWS] = { 36, 73, 29, 0, 89, 89 };   /* 0..100 */
static uint8_t s_sel = 0;

/* ---- one composited row ------------------------------------------------- */
static inline int row_of(int y)
{
    if (y < UI_ROW_Y0) return -1;
    int i = (y - UI_ROW_Y0) / UI_ROW_PITCH;
    if (i >= UI_ROWS) return -1;
    return (y - UI_ROW_Y0 - i * UI_ROW_PITCH) < UI_BAR_H ? i : -1;
}

/* Rounded end cap: how far the fill is inset on this scanline. Cheap integer
 * circle — the radius is BAR_H/2, so at most 6 px of inset. */
static inline int cap_inset(int dy, int h)
{
    int r = h / 2;
    int d = dy < r ? (r - dy) : (dy - (h - 1 - r));
    if (d <= 0) return 0;
    int inset = 0;
    while (inset < r && (r - inset) * (r - inset) + d * d > r * r) inset++;
    return inset;
}

static int fill_width(int row)
{
    int track = UI_TRACK_R - UI_BAR_X;
    if (row == UI_KEY_ROW) return UI_KEY_SEGW;          /* one segment */
    int w = (track * s_val[row]) / 100;
    return w < UI_BAR_H ? UI_BAR_H : w;
}

/* Glyph blit for the value caption, straight out of the baked font. Coverage
 * is binary for Bitcount, so a nibble is either on or off. */
static void draw_caption(uint16_t *line, int y, int rowtop, int fillend)
{
    const bakedfont_t *f = &font_hn_label;
    char s[5];
    int v = s_val[s_sel], n = 0;
    if (v >= 100) s[n++] = '1';
    if (v >= 10)  s[n++] = (char)('0' + (v / 10) % 10);
    s[n++] = (char)('0' + v % 10);
    s[n++] = '%';
    s[n]   = 0;

    int adv = 0;
    for (int i = 0; i < n; ++i) adv += f->glyphs[(uint8_t)s[i] - f->first].adv;
    /* keep the caption inside the fill; on a short bar it would otherwise
     * slide off the left end and sit on the bare gradient. */
    int pen = fillend - adv - 5;
    if (pen < UI_BAR_X + 4) pen = UI_BAR_X + 4;
    int base = rowtop + 1 + f->ascent;

    for (int i = 0; i < n; ++i) {
        const bglyph_t *g = &f->glyphs[(uint8_t)s[i] - f->first];
        int gy = y - (base - g->top);
        if (gy >= 0 && gy < g->h) {
            for (int gx = 0; gx < g->w; ++gx) {
                uint32_t nib = g->off + (uint32_t)gy * g->w + gx;
                uint8_t  v4  = (nib & 1) ? (f->data[nib >> 1] & 0x0F)
                                         : (f->data[nib >> 1] >> 4);
                if (v4 >= 8) {
                    int x = pen + g->left + gx;
                    if (x >= 0 && x < OLED_WIDTH)
                        line[x] = RGB565(CAP_R, CAP_G, CAP_B);
                }
            }
        }
        pen += g->adv;
    }
}

static void compose_row(int y, uint16_t *line)
{
    memcpy(line, &plate_bg[(size_t)y * OLED_WIDTH], OLED_WIDTH * sizeof(uint16_t));

    int row = row_of(y);
    if (row < 0) return;

    int rowtop = UI_ROW_Y0 + row * UI_ROW_PITCH;
    int dy     = y - rowtop;
    int inset  = cap_inset(dy, UI_BAR_H);

    if (row == UI_KEY_ROW) {
        int seg = (s_val[row] * UI_KEY_SEGS) / 101;      /* 0..3 */
        int x0  = UI_BAR_X + seg * (UI_KEY_SEGW + UI_KEY_GAP);
        for (int x = x0 + inset; x < x0 + UI_KEY_SEGW - inset; ++x)
            if (x >= 0 && x < OLED_WIDTH) line[x] = MINT565;
        if (row == s_sel && (dy == 0 || dy == UI_BAR_H - 1))
            for (int x = UI_BAR_X; x < UI_TRACK_R; ++x)
                if (x >= 0 && x < OLED_WIDTH) line[x] = MINT565;
        return;
    }

    int w   = fill_width(row);
    int end = UI_BAR_X + w;
    for (int x = UI_BAR_X + inset; x < end - inset; ++x)
        if (x >= 0 && x < OLED_WIDTH) line[x] = MINT565;

    /* selection: a 1 px mint edge above and below, cheap and unmistakable
     * on the panel without needing a blur. */
    if (row == s_sel) {
        if (dy == 0 || dy == UI_BAR_H - 1) {
            for (int x = UI_BAR_X; x < end; ++x)
                if (x >= 0 && x < OLED_WIDTH) line[x] = MINT565;
        }
        draw_caption(line, y, rowtop, end);
    }
}

#ifndef DESIGN_BENCH_HOST
/* ---- panel -------------------------------------------------------------- */
extern void lcd_set_window_full(void);        /* from lcd_st7789_pico.c */
extern void lcd_stream_begin(void);
extern void lcd_stream_row(const uint8_t *row, size_t n);
extern void lcd_stream_end(void);

static void present(void)
{
    static uint16_t line[OLED_WIDTH];
    static uint8_t  out[OLED_WIDTH * 2];
    lcd_set_window_full();
    lcd_stream_begin();
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        compose_row(y, line);
        for (int x = 0; x < OLED_WIDTH; ++x) {      /* ST7789 wants MSB first */
            out[x * 2]     = (uint8_t)(line[x] >> 8);
            out[x * 2 + 1] = (uint8_t)line[x];
        }
        lcd_stream_row(out, sizeof out);
    }
    lcd_stream_end();
}

/* ---- input -------------------------------------------------------------- */
static void gpio_in_pullup(uint pin)
{
    gpio_init(pin); gpio_set_dir(pin, GPIO_IN); gpio_pull_up(pin);
}

int main(void)
{
    stdio_init_all();

    gpio_in_pullup(PIN_ENC_CLK);
    gpio_in_pullup(PIN_ENC_DT);
    gpio_in_pullup(PIN_ENC_SW);
    gpio_in_pullup(PIN_SHIFT);

    gpio_set_function(PIN_BL, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_BL);
    pwm_set_wrap(slice, 255);
    pwm_set_gpio_level(PIN_BL, 220);
    pwm_set_enabled(slice, true);

    oled_init();
    present();

    bool last_clk = gpio_get(PIN_ENC_CLK);
    bool last_sw  = true;
    absolute_time_t sw_guard = get_absolute_time();

    for (;;) {
        bool dirty = false;
        bool shift = !gpio_get(PIN_SHIFT);

        bool clk = gpio_get(PIN_ENC_CLK);
        if (last_clk && !clk) {                         /* falling edge */
            int dir = gpio_get(PIN_ENC_DT) ? +1 : -1;
            int step = shift ? 5 : 1;
            if (s_sel == UI_KEY_ROW) step = 34;         /* jump a segment */
            int v = (int)s_val[s_sel] + dir * step;
            if (v < 0)   v = 0;
            if (v > 100) v = 100;
            s_val[s_sel] = (uint8_t)v;
            dirty = true;
        }
        last_clk = clk;

        bool sw = gpio_get(PIN_ENC_SW);
        if (last_sw && !sw && absolute_time_diff_us(sw_guard, get_absolute_time()) > 0) {
            s_sel = shift ? (uint8_t)((s_sel + UI_ROWS - 1) % UI_ROWS)
                          : (uint8_t)((s_sel + 1) % UI_ROWS);
            sw_guard = make_timeout_time_ms(180);
            dirty = true;
        }
        last_sw = sw;

        if (dirty) present();
        sleep_ms(2);
    }
}
#endif /* DESIGN_BENCH_HOST */
