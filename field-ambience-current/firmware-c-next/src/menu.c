/*
 * Menu/state — Step 12b #7. Mockup-style: label · big value · battery ·
 * bottom pill row. Mono white on black, inactive elements dimmed via grey.
 *
 * The parameter table holds each slot's label, value-renderer and the
 * apply callback into the engine. Browse rotation cycles the slot; edit
 * rotation moves the slot's value through its range and immediately fires
 * the engine callback (live update — the "global follows live" rule).
 */

#include "menu.h"
#include "oled.h"
#include "baked_font.h"
#include "battery.h"

#include <stdio.h>
#include <string.h>

/* --- value spaces ----------------------------------------------------- */

/* Helvetica Neue is baked with full case, so values use clean Title Case. */
static const char * const KEY_NAMES[12] = {
    "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};
static const char * const MODE_NAMES[6] = {
    "Ionian","Dorian","Phrygian","Lydian","Mixolyd.","Aeolian"
};
static const char * const VIBE_NAMES[4]  = { "Warm","Bright","Deep","Floating" };
static const char * const VOICE_NAMES[3] = { "Warm","Strings","Brass" };

/* --- live state ------------------------------------------------------- */

static menu_callbacks_t cb;
static menu_param_t  cur     = MP_KEY;
static menu_mode_t   mode    = MENU_BROWSE;
static int           key     = 60;        /* C4 */
static int           mode_i  = 0;
static int           vibe_i  = 0;
static int           voice_i = 0;
static int           texture = 20;        /* % 0..100 */
static int           bass    = 50;
static int           space   = 50;
static int           mood    = 50;
static int           backlight = 70;     /* LCD brightness % */

static int clampi(int v, int lo, int hi) { return v<lo?lo:(v>hi?hi:v); }
static int wrapi (int v, int n)          { v %= n; if (v < 0) v += n; return v; }

/* Apply the current slot's value to the engine via callback. */
static void apply_current(void) {
    switch (cur) {
        case MP_KEY:        if (cb.set_key)        cb.set_key(60 + (wrapi(key - 60, 12))); break;
        case MP_MODE:       if (cb.set_mode)       cb.set_mode(mode_i);                   break;
        case MP_VIBE:       if (cb.set_vibe)       cb.set_vibe(vibe_i);                   break;
        case MP_VOICE:      if (cb.set_pad_voice)  cb.set_pad_voice(voice_i);             break;
        case MP_TEXTURE:    if (cb.set_texture)    cb.set_texture(texture / 100.0f);      break;
        case MP_BASS:       if (cb.set_bass_depth) cb.set_bass_depth(bass    / 100.0f);   break;
        case MP_SPACE:      if (cb.set_space)      cb.set_space   (space   / 100.0f);     break;
        case MP_MOOD:       if (cb.set_mood)       cb.set_mood    (mood    / 100.0f);     break;
        case MP_BACKLIGHT:  if (cb.set_backlight)  cb.set_backlight(backlight / 100.0f);  break;
        default: break;
    }
}

void menu_init(const menu_callbacks_t *cbs) {
    if (cbs) cb = *cbs; else memset(&cb, 0, sizeof cb);
    cur = MP_KEY;
    mode = MENU_BROWSE;
    /* Don't push defaults — engine_init already set them. */
}

/* --- state queries ---------------------------------------------------- */

menu_param_t menu_current(void) { return cur; }
menu_mode_t  menu_mode(void)    { return mode; }

const char *menu_current_label(void) {
    static const char * const LABELS[MP_COUNT] = {
        "Key","Mode","Vibe","Voice","Texture","Bass","Space","Mood","Backlight"
    };
    return LABELS[cur];
}

int menu_value_index(menu_param_t p) {
    switch (p) {
        case MP_KEY:        return wrapi(key - 60, 12);
        case MP_MODE:       return mode_i;
        case MP_VIBE:       return vibe_i;
        case MP_VOICE:      return voice_i;
        default: return 0;
    }
}

int menu_value_int(menu_param_t p) {
    switch (p) {
        case MP_TEXTURE:   return texture;
        case MP_BASS:      return bass;
        case MP_SPACE:     return space;
        case MP_MOOD:      return mood;
        case MP_BACKLIGHT: return backlight;
        default:           return 0;
    }
}

/* Number of discrete options a parameter can step through. 0 = continuous (a
 * 0..100 % amount) — rendered as a fill bar instead of one pill per option. */
int menu_value_count(menu_param_t p) {
    switch (p) {
        case MP_KEY:        return 12;
        case MP_MODE:       return 6;
        case MP_VIBE:       return 4;
        case MP_VOICE:      return 3;
        default:            return 0;   /* TEXTURE/BASS/SPACE/MOOD = % */
    }
}

const char *menu_current_value_text(void) {
    static char buf[12];
    switch (cur) {
        case MP_KEY:        return KEY_NAMES[wrapi(key - 60, 12)];
        case MP_MODE:       return MODE_NAMES[mode_i];
        case MP_VIBE:       return VIBE_NAMES[vibe_i];
        case MP_VOICE:      return VOICE_NAMES[voice_i];
        case MP_TEXTURE:    snprintf(buf, sizeof buf, "%d%%", texture);   return buf;
        case MP_BASS:       snprintf(buf, sizeof buf, "%d%%", bass);      return buf;
        case MP_SPACE:      snprintf(buf, sizeof buf, "%d%%", space);     return buf;
        case MP_MOOD:       snprintf(buf, sizeof buf, "%d%%", mood);      return buf;
        case MP_BACKLIGHT:  snprintf(buf, sizeof buf, "%d%%", backlight); return buf;
        default: return "";
    }
}

/* --- input ------------------------------------------------------------ */

void menu_push(void) {
    mode = (mode == MENU_BROWSE) ? MENU_EDIT : MENU_BROWSE;
}

void menu_rotate(int delta) {
    if (mode == MENU_BROWSE) {
        cur = (menu_param_t)wrapi((int)cur + delta, MP_COUNT);
        return;
    }
    /* edit: step the current slot's value */
    switch (cur) {
        case MP_KEY:        key  = 60 + wrapi(key - 60 + delta, 12); break;
        case MP_MODE:       mode_i  = wrapi(mode_i  + delta, 6); break;
        case MP_VIBE:       vibe_i  = wrapi(vibe_i  + delta, 4); break;
        case MP_VOICE:      voice_i = wrapi(voice_i + delta, 3); break;
        case MP_TEXTURE:    texture = clampi(texture + 5*delta, 0, 100); break;
        case MP_BASS:       bass    = clampi(bass    + 5*delta, 0, 100); break;
        case MP_SPACE:      space     = clampi(space     + 5*delta, 0, 100); break;
        case MP_MOOD:       mood      = clampi(mood      + 5*delta, 0, 100); break;
        case MP_BACKLIGHT:  backlight = clampi(backlight + 5*delta, 0, 100); break;
        default: break;
    }
    apply_current();
}

/* --- render ----------------------------------------------------------- */
/*
 * OP-1-Field-style composition: deep black, strong contrast, left-aligned with
 * a consistent left margin and generous negative space to the right. One focus
 * value, large; quiet label above it; battery top-right; a minimal dash row at
 * the bottom. Active = bright white, inactive = subtle grey.
 */

/* Brightness levels. */
#define GS_BG       0
#define GS_DASH     3      /* inactive dash — subtle */
#define GS_DIM      5
#define GS_LABEL    9      /* quiet secondary text */
#define GS_MID      9
#define GS_ACTIVE  15      /* bright focus */

#define PAD_L      22      /* consistent left margin (320×170 LCD) */
#define PAD_R      18
#define PAD_T      14

/* Round the 4 screen corners to black so content respects the physical rounded
 * display window (nothing lit pokes past the bezel). */
static void mask_corners(void) {
    const int r = 16;
    for (int yy = 0; yy < r; ++yy)
        for (int xx = 0; xx < r; ++xx) {
            int dx = r - 1 - xx, dy = r - 1 - yy;
            if (dx * dx + dy * dy > r * r) {
                oled_pixel(xx, yy, GS_BG);
                oled_pixel(OLED_WIDTH - 1 - xx, yy, GS_BG);
                oled_pixel(xx, OLED_HEIGHT - 1 - yy, GS_BG);
                oled_pixel(OLED_WIDTH - 1 - xx, OLED_HEIGHT - 1 - yy, GS_BG);
            }
        }
}

/* Battery: rounded outline + terminal nub + rounded fill, top-right. Sized to
 * match the OP-1-style minimal HUD — visually small enough to feel like a
 * status hint, not an icon competing with the value. */
static void render_battery(void) {
    const int w = 26, h = 12, r = 4;
    const int x = OLED_WIDTH - PAD_R - w - 3;   /* leave room for the nub */
    const int y = PAD_T + 2;                    /* optical centre vs label */
    oled_rrect_stroke(x, y, w, h, r, 1, GS_MID);
    oled_rrect_fill(x + w + 1, y + 3, 2, h - 6, 1, GS_MID);   /* terminal */

    int pct = battery_pct();
    int inner = w - 6;
    int fw = (inner * pct + 50) / 100;
    if (fw < 2 && pct > 0) fw = 2;
    if (fw > inner) fw = inner;
    if (fw > 0)
        oled_rrect_fill(x + 3, y + 3, fw, h - 6, 2,
                        battery_usb_present() ? GS_ACTIVE : GS_MID);
    if (battery_usb_present())
        oled_rrect_fill(x + w / 2 - 1, y + 2, 2, h - 4, 1, GS_ACTIVE);
}

/* Bottom selector — one pill per OPTION OF THE CURRENT SETTING, spanning
 * EDGE-TO-EDGE between the side margins. The pill count therefore depends on
 * the setting: Mode → 6 pills, Vibe → 4, Voice → 3, Drone → 2, Key → 12. The
 * active pill (= current value) is bigger and bright and slides left↔right as
 * the value changes; the others are small and dim. Pill sizes scale with N so
 * few options → fat pills, many → narrow — no scrolling, no chevrons.
 * Continuous % settings don't use this; they get render_fillbar() instead. */
#define BAR_Y      148
#define BAR_H_INA  5      /* inactive pill height */
#define BAR_H_ACT  8      /* active pill height — slightly taller for emphasis */
#define BAR_GAP    6      /* gap between pills */
#define BAR_ACT_K  24     /* active = K/10 × inactive width (≈2.4×) */

static void render_bar(int n, int active) {
    if (n < 1) return;
    int usable = OLED_WIDTH - PAD_L - PAD_R;

    /* Layout: (n-1) gaps + (n-1) inactive widths + 1 active width = usable
     * with active = (BAR_ACT_K/10) * inactive. Tighten gap on dense bars so
     * pills don't shrink below ~4 px. */
    int gap = BAR_GAP;
    int gaps_total = (n - 1) * gap;
    /* solve: inactive * ((n-1) + K/10) = usable - gaps_total */
    int num = (usable - gaps_total) * 10;
    int den = (n - 1) * 10 + BAR_ACT_K;
    int ina_w = den > 0 ? num / den : usable;
    if (ina_w < 4 && n > 1) {
        gap = 3;
        gaps_total = (n - 1) * gap;
        num = (usable - gaps_total) * 10;
        ina_w = num / den;
        if (ina_w < 3) ina_w = 3;
    }
    int act_w = (ina_w * BAR_ACT_K + 5) / 10;
    if (n == 1) { ina_w = 0; act_w = usable; }

    /* Distribute the rounding residue across early inactive pills so the row
     * ends exactly at the right margin (edge-to-edge). */
    int used = act_w + (n - 1) * ina_w + gaps_total;
    int residue = usable - used;            /* 0..n-1 px */

    int x = PAD_L;
    for (int i = 0; i < n; ++i) {
        bool act = (i == active);
        int w = act ? act_w : ina_w;
        if (!act && residue > 0) { w += 1; --residue; }
        int h  = act ? BAR_H_ACT : BAR_H_INA;
        int yy = BAR_Y + (BAR_H_ACT - h) / 2;   /* vertically centred on the row */
        uint8_t gs = act ? GS_ACTIVE : GS_DASH;
        oled_pill(x, yy, w, h, gs);
        x += w + gap;
    }
}

/* Continuous % setting (Texture/Bass/Space/Mood/Volume): a single rounded track
 * spanning edge-to-edge with a bright fill proportional to the 0..100 value —
 * the honest representation of a stepless amount (no fake discrete pills). */
static void render_fillbar(int pct) {
    int x = PAD_L;
    int w = OLED_WIDTH - PAD_L - PAD_R;
    int y = BAR_Y;
    int h = BAR_H_ACT;
    int r = h / 2;
    oled_pill(x, y, w, h, GS_DASH);            /* dim track */
    int fw = (w * pct + 50) / 100;
    if (fw < h && pct > 0) fw = h;             /* keep a visible rounded cap */
    if (fw > w) fw = w;
    if (fw > 0) oled_rrect_fill(x, y, fw, h, r, GS_ACTIVE);
}

/* Big value in Helvetica Neue Light, LEFT-aligned at the left margin. Vertically
 * centred between the label baseline above and the pill row below. Drops to the
 * small face only if a long word would run into the battery / right edge. */
static void render_value(void) {
    const char *txt = menu_current_value_text();
    uint8_t gs = (mode == MENU_EDIT) ? GS_ACTIVE : 13;
    const bakedfont_t *f = &font_hn_value;
    int avail = OLED_WIDTH - PAD_L - PAD_R;
    if (bfont_width(f, txt) > avail) f = &font_hn_value_small;

    /* Centre the value's line box vertically between (label bottom) and (bar top). */
    int top    = PAD_T + (int)font_hn_label.line;   /* label baseline area ends here */
    int bottom = BAR_Y;
    int y = (top + bottom - (int)f->line) / 2;
    bfont_draw(f, PAD_L, y, txt, gs);
}

void menu_render(void) {
    oled_fill(GS_BG);

    /* Quiet label, top-left, Helvetica Neue Thin. */
    uint8_t lbl_gs = (mode == MENU_EDIT) ? GS_DIM : GS_LABEL;
    bfont_draw(&font_hn_label, PAD_L, PAD_T, menu_current_label(), lbl_gs);

    render_battery();
    render_value();

    /* Bottom bar has TWO meanings depending on mode:
     *  - BROWSE: a STABLE menu-position indicator (one pill per setting, the
     *    current one highlighted) — scrolling through settings only slides the
     *    active pill, layout stays put.
     *  - EDIT:   the current setting's value space — one pill per discrete
     *    option (active = current value), or a proportional fill bar for the
     *    continuous % settings. Layout adapts to the value, which is fine
     *    once the user has entered edit mode by pushing.
     */
    if (mode == MENU_BROWSE) {
        render_bar(MP_COUNT, (int)cur);
    } else {
        int n = menu_value_count(cur);
        if (n > 0) render_bar(n, menu_value_index(cur));
        else       render_fillbar(menu_value_int(cur));
    }

    mask_corners();                   /* rounded screen window */
}

void menu_render_bar_only(int total, int active) { render_bar(total, active); }
