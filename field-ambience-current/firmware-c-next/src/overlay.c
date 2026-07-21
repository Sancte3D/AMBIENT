/*
 * overlay.c — transient value overlay. See overlay.h.
 */

#include "overlay.h"
#include "oled.h"
#include "baked_font.h"
#include <string.h>

/* Gleiche Layout-Konstanten wie menu.c (dort lokal definiert — bewusst
 * dupliziert statt menu.c zu exportieren: das Overlay soll wie das Menue
 * AUSSEHEN, aber nicht an dessen Innereien haengen). */
#define OV_GS_BG     0
#define OV_GS_LABEL  9
#define OV_GS_VALUE  13
#define OV_PAD_L     22
#define OV_PAD_T     14

#define OV_MAXLEN    20

static char     s_label[OV_MAXLEN + 1];
static char     s_value[OV_MAXLEN + 1];
static uint32_t s_until;
static bool     s_armed;
static uint32_t s_gen;

void overlay_init(void) {
    s_label[0] = s_value[0] = '\0';
    s_until = 0;
    s_armed = false;
    s_gen   = 0;
}

static void copy_capped(char *dst, const char *src) {
    unsigned i = 0;
    if (src) for (; i < OV_MAXLEN && src[i]; ++i) dst[i] = src[i];
    dst[i] = '\0';
}

void overlay_show(const char *label, const char *value,
                  uint32_t now_ms, uint16_t hold_ms) {
    copy_capped(s_label, label);
    copy_capped(s_value, value);
    s_until = now_ms + (hold_ms ? hold_ms : OVERLAY_HOLD_MS);
    s_armed = true;
    ++s_gen;
}

bool overlay_active(uint32_t now_ms) {
    if (!s_armed) return false;
    /* Vorzeichenbehaftete Differenz: robust gegen ms-Wraparound. */
    if ((int32_t)(s_until - now_ms) <= 0) { s_armed = false; return false; }
    return true;
}

uint32_t overlay_gen(void) { return s_gen; }

void overlay_render(void) {
    oled_fill(OV_GS_BG);
    bfont_draw(&font_hn_label, OV_PAD_L, OV_PAD_T, s_label, OV_GS_LABEL);

    /* Wert gross, vertikal zwischen Label und Unterkante zentriert —
     * gleiche Logik wie menu.c render_value (inkl. Small-Fallback). */
    const bakedfont_t *f = &font_hn_value;
    int avail = OLED_WIDTH - OV_PAD_L - OV_PAD_L;
    if (bfont_width(f, s_value) > avail) f = &font_hn_value_small;
    int top    = OV_PAD_T + (int)font_hn_label.line;
    int bottom = OLED_HEIGHT - 12;
    int y = (top + bottom - (int)f->line) / 2;
    bfont_draw(f, OV_PAD_L, y, s_value, OV_GS_VALUE);
}
