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
#include "battery.h"

#include <stdio.h>
#include <string.h>

/* --- value spaces ----------------------------------------------------- */

/* All UPPER-CASE: starter font has no lowercase yet, and synth UIs read fine
 * in caps anyway. # is the sharp glyph added in font_8x8.c. */
static const char * const KEY_NAMES[12] = {
    "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};
static const char * const MODE_NAMES[6] = {
    "IONIAN","DORIAN","PHRYG","LYDIAN","MIXO","AEOLIAN"
};
static const char * const VIBE_NAMES[4]  = { "WARM","BRIGHT","DEEP","FLOATY" };
static const char * const VOICE_NAMES[3] = { "WARM","STRINGS","BRASS" };
/* Generative: 0 = off, 1..5 = fixed PROGRESSIONS, 6 = Markov auto. Roman
 * numerals stay readable in caps too. */
static const char * const GEN_NAMES[7]  = { "OFF","I-IV","I-V-VI-IV","I-IV-V-I","I-VI-III-VII","I-IV-VI-V","MARKOV" };

/* --- live state ------------------------------------------------------- */

static menu_callbacks_t cb;
static menu_param_t  cur     = MP_KEY;
static menu_mode_t   mode    = MENU_BROWSE;
static int           key     = 60;        /* C4 */
static int           mode_i  = 0;
static int           vibe_i  = 0;
static int           voice_i = 0;
static bool          drone   = false;
static int           gen_i   = 0;         /* 0=off, 1..5=fixed, 6=markov */
static int           texture = 20;        /* % 0..100 */
static int           bass    = 50;
static int           space   = 50;
static int           mood    = 50;
static int           volume  = 60;

static int clampi(int v, int lo, int hi) { return v<lo?lo:(v>hi?hi:v); }
static int wrapi (int v, int n)          { v %= n; if (v < 0) v += n; return v; }

/* Apply the current slot's value to the engine via callback. */
static void apply_current(void) {
    switch (cur) {
        case MP_KEY:        if (cb.set_key)        cb.set_key(60 + (wrapi(key - 60, 12))); break;
        case MP_MODE:       if (cb.set_mode)       cb.set_mode(mode_i);                   break;
        case MP_VIBE:       if (cb.set_vibe)       cb.set_vibe(vibe_i);                   break;
        case MP_VOICE:      if (cb.set_pad_voice)  cb.set_pad_voice(voice_i);             break;
        case MP_DRONE:      if (cb.set_drone)      cb.set_drone(drone);                   break;
        case MP_GENERATIVE: if (cb.set_generative) {
            if (gen_i == 0)      cb.set_generative(false, 0);
            else if (gen_i == 6) cb.set_generative(true, -1);     /* Markov */
            else                  cb.set_generative(true, gen_i - 1);
        } break;
        case MP_TEXTURE:    if (cb.set_texture)    cb.set_texture(texture / 100.0f);      break;
        case MP_BASS:       if (cb.set_bass_depth) cb.set_bass_depth(bass    / 100.0f);   break;
        case MP_SPACE:      if (cb.set_space)      cb.set_space   (space   / 100.0f);     break;
        case MP_MOOD:       if (cb.set_mood)       cb.set_mood    (mood    / 100.0f);     break;
        case MP_VOLUME:     if (cb.set_master)     cb.set_master  (volume  / 100.0f);     break;
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
        "KEY","MODE","VIBE","VOICE","DRONE","GEN","TEXTURE","BASS","SPACE","MOOD","VOL"
    };
    return LABELS[cur];
}

int menu_value_index(menu_param_t p) {
    switch (p) {
        case MP_KEY:        return wrapi(key - 60, 12);
        case MP_MODE:       return mode_i;
        case MP_VIBE:       return vibe_i;
        case MP_VOICE:      return voice_i;
        case MP_DRONE:      return drone ? 1 : 0;
        case MP_GENERATIVE: return gen_i;
        default: return 0;
    }
}

int menu_value_int(menu_param_t p) {
    switch (p) {
        case MP_TEXTURE: return texture;
        case MP_BASS:    return bass;
        case MP_SPACE:   return space;
        case MP_MOOD:    return mood;
        case MP_VOLUME:  return volume;
        default:         return 0;
    }
}

const char *menu_current_value_text(void) {
    static char buf[12];
    switch (cur) {
        case MP_KEY:        return KEY_NAMES[wrapi(key - 60, 12)];
        case MP_MODE:       return MODE_NAMES[mode_i];
        case MP_VIBE:       return VIBE_NAMES[vibe_i];
        case MP_VOICE:      return VOICE_NAMES[voice_i];
        case MP_DRONE:      return drone ? "ON" : "OFF";
        case MP_GENERATIVE: return GEN_NAMES[gen_i];
        case MP_TEXTURE:    snprintf(buf, sizeof buf, "%d%%", texture); return buf;
        case MP_BASS:       snprintf(buf, sizeof buf, "%d%%", bass);    return buf;
        case MP_SPACE:      snprintf(buf, sizeof buf, "%d%%", space);   return buf;
        case MP_MOOD:       snprintf(buf, sizeof buf, "%d%%", mood);    return buf;
        case MP_VOLUME:     snprintf(buf, sizeof buf, "%d%%", volume);  return buf;
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
        case MP_KEY:        key  += delta; break;
        case MP_MODE:       mode_i  = wrapi(mode_i  + delta, 6); break;
        case MP_VIBE:       vibe_i  = wrapi(vibe_i  + delta, 4); break;
        case MP_VOICE:      voice_i = wrapi(voice_i + delta, 3); break;
        case MP_DRONE:      drone   = !drone;                    break;
        case MP_GENERATIVE: gen_i   = wrapi(gen_i   + delta, 7); break;
        case MP_TEXTURE:    texture = clampi(texture + 5*delta, 0, 100); break;
        case MP_BASS:       bass    = clampi(bass    + 5*delta, 0, 100); break;
        case MP_SPACE:      space   = clampi(space   + 5*delta, 0, 100); break;
        case MP_MOOD:       mood    = clampi(mood    + 5*delta, 0, 100); break;
        case MP_VOLUME:     volume  = clampi(volume  + 5*delta, 0, 100); break;
        default: break;
    }
    apply_current();
}

/* --- render ----------------------------------------------------------- */

/* Brightness levels: active = full white, inactive = dimmed but still readable. */
#define GS_BG       0
#define GS_DIM      4
#define GS_MID      8
#define GS_ACTIVE  15

/* Battery indicator: a thin pill outline + filled bar inside, in the top-right
 * corner (matches the screenshot mockup). Width depends on charge. USB present
 * draws a different style (filled + small "+"). */
static void render_battery(void) {
    const int x = 220, y = 6, w = 28, h = 12;
    /* outline */
    oled_rect_fill(x,            y,         w,     1,    GS_MID);
    oled_rect_fill(x,            y + h - 1, w,     1,    GS_MID);
    oled_rect_fill(x,            y,         1,     h,    GS_MID);
    oled_rect_fill(x + w - 1,    y,         1,     h,    GS_MID);
    /* small +tip on the right */
    oled_rect_fill(x + w,        y + 3,     2,     6,    GS_MID);

    int pct = battery_pct();
    int fill_w = ((w - 4) * pct + 50) / 100;
    if (fill_w < 0) fill_w = 0;
    if (fill_w > w - 4) fill_w = w - 4;
    oled_rect_fill(x + 2, y + 2, fill_w, h - 4, battery_usb_present() ? GS_ACTIVE : GS_MID);

    if (battery_usb_present()) {
        /* small "+" inside */
        oled_rect_fill(x + 12, y + 5, 4, 2, GS_BG);
        oled_rect_fill(x + 13, y + 4, 2, 4, GS_BG);
    }
}

/* Pill row at the bottom. The active slot is wider + bright, others dimmed. */
static void render_pills(void) {
    const int N = MP_COUNT;            /* 11 */
    const int total_w = 240;           /* across the screen, leaving 8 px margin */
    const int y = 55;
    const int h = 6;
    const int gap = 4;
    /* normal pill width covers the non-active total */
    const int active_w = 24;
    int inactive_w = (total_w - active_w - (N - 1) * gap) / (N - 1);
    if (inactive_w < 8) inactive_w = 8;
    /* recompute total to centre */
    int width_used = active_w + (N - 1) * inactive_w + (N - 1) * gap;
    int x = (OLED_WIDTH - width_used) / 2;
    for (int i = 0; i < N; ++i) {
        int w = (i == (int)cur) ? active_w : inactive_w;
        uint8_t gs = (i == (int)cur) ? GS_ACTIVE : GS_DIM;
        oled_pill(x, y, w, h, gs);
        x += w + gap;
    }
}

/* Big value text, scaled to roughly fill the centre band. Long values shrink. */
static void render_value(void) {
    const char *txt = menu_current_value_text();
    /* Pick the largest scale that still fits inside ~232 px wide. */
    int scale = 4;
    while (scale > 1 && oled_text_width(txt, scale) > 232) --scale;
    int w = oled_text_width(txt, scale);
    int x = (OLED_WIDTH - w) / 2;
    int y = 20;
    uint8_t gs = (mode == MENU_EDIT) ? GS_ACTIVE : GS_MID;
    oled_text_scaled(x, y, txt, gs, scale);
}

void menu_render(void) {
    oled_fill(GS_BG);

    /* Label top-left, slightly dim while editing so the value pops. */
    uint8_t lbl_gs = (mode == MENU_EDIT) ? GS_DIM : GS_ACTIVE;
    oled_text(8, 6, menu_current_label(), lbl_gs);

    render_battery();
    render_value();
    render_pills();
}
