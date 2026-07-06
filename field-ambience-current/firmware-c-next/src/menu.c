/*
 * Menu/state — WORLD model (r18.38). Mockup-style: label · big value ·
 * battery · bottom pill row. Mono white on black, inactive elements dimmed
 * via grey. Same renderer as the old V1 menu; only the parameter content
 * changed from Key/Mode/Vibe/... to World + global macros.
 *
 * Browse rotation cycles the slot; edit rotation moves the slot's value
 * through its range and immediately fires the engine callback (live update —
 * the "global follows live" rule). Selecting a WORLD loads that world's macro
 * preset so the four macros snap to sensible per-world values.
 */

#include "menu.h"
#include "oled.h"
#include "oled_color.h"
#include "baked_font.h"
#include "battery.h"
#include "worlds.h"

#include <stdio.h>
#include <string.h>

/* World identity + per-world presets + per-world accent colour all live in
 * worlds.c (ADR-0017). This module only owns *menu state* (cursor, edit mode,
 * live macro values). */

/* --- live state ------------------------------------------------------- */

static menu_callbacks_t cb;
static menu_param_t  cur     = MP_WORLD;
static menu_mode_t   mode    = MENU_BROWSE;
static int           world_i = 0;
static int           key_pc  = 0;         /* tonic pitch class 0..11 (r18.98) */
static int           voice_i = 0;         /* 0 PAD / 1 STRING / 2 GLASS       */
static int           space   = 42;        /* % 0..100 */
static int           atmos   = 35;
static int           motion  = 40;
static int           age     = 30;
static int           echo    = 35;
static int           blur    = 15;

static const char * const KEY_NAMES[12] = {
    "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"
};
static const char * const VOICE_NAMES[3] = { "Pad", "String", "Glass" };

static int clampi(int v, int lo, int hi) { return v<lo?lo:(v>hi?hi:v); }
static int wrapi (int v, int n)          { v %= n; if (v < 0) v += n; return v; }

/* Push the current world's display accent to the colour layer. `animate` =
 * crossfade toward it (world change); otherwise snap (boot/init). */
static void set_world_accent(bool animate) {
    const world_t *w = worlds_get(world_i);
    if (animate) oled_set_accent_target(w->accent_r, w->accent_g, w->accent_b);
    else         oled_set_accent(w->accent_r, w->accent_g, w->accent_b);
}

/* Apply the current slot's value to the engine via callback. */
static void apply_current(void) {
    switch (cur) {
        case MP_WORLD:  if (cb.set_world)      cb.set_world(world_i);            break;
        case MP_KEY:    if (cb.set_key)        cb.set_key  (key_pc);             break;
        case MP_VOICE:  if (cb.set_voice)      cb.set_voice(voice_i);            break;
        case MP_SPACE:  if (cb.set_space)      cb.set_space (space  / 100.0f);   break;
        case MP_ATMOS:  if (cb.set_atmosphere) cb.set_atmosphere(atmos / 100.0f);break;
        case MP_MOTION: if (cb.set_motion)     cb.set_motion(motion / 100.0f);   break;
        case MP_AGE:    if (cb.set_age)        cb.set_age   (age    / 100.0f);   break;
        case MP_ECHO:   if (cb.set_echo)       cb.set_echo  (echo   / 100.0f);   break;
        case MP_BLUR:   if (cb.set_blur)       cb.set_blur  (blur   / 100.0f);   break;
        default: break;
    }
}

/* Load a world's macro preset into the live macros + push them to the engine.
 * Called when the user changes the WORLD value. */
static void load_world_preset(void) {
    const world_t *w = worlds_get(world_i);
    key_pc = (int)w->key_midi % 12;    /* world identity includes its key;
                                        * VOICE stays — a player's choice   */
    space  = w->space_pct;
    atmos  = w->atmos_pct;
    motion = w->motion_pct;
    age    = w->age_pct;
    echo   = w->echo_pct;
    blur   = w->blur_pct;
    set_world_accent(true);        /* crossfade the UI tint to the new world */
    if (cb.set_world)      cb.set_world(world_i);
    if (cb.set_space)      cb.set_space     (space  / 100.0f);
    if (cb.set_atmosphere) cb.set_atmosphere(atmos  / 100.0f);
    if (cb.set_motion)     cb.set_motion    (motion / 100.0f);
    if (cb.set_age)        cb.set_age       (age    / 100.0f);
    if (cb.set_echo)       cb.set_echo      (echo   / 100.0f);
    if (cb.set_blur)       cb.set_blur      (blur   / 100.0f);
    if (cb.set_key)        cb.set_key       (key_pc);
}

void menu_init(const menu_callbacks_t *cbs) {
    if (cbs) cb = *cbs; else memset(&cb, 0, sizeof cb);
    cur = MP_WORLD;
    mode = MENU_BROWSE;
    world_i = 0;
    {
        const world_t *w = worlds_get(0);
        key_pc = (int)w->key_midi % 12;
        voice_i = 0;
        space  = w->space_pct;
        atmos  = w->atmos_pct;
        motion = w->motion_pct;
        age    = w->age_pct;
        echo   = w->echo_pct;
        blur   = w->blur_pct;
    }
    set_world_accent(false);       /* boot world's tint (world 0), snap */
    /* Don't push the macro defaults here — the engine sets its own at
     * engine_init. The accent IS set: it's a display concern, not an engine
     * callback, and the panel should boot already tinted to world 0. */
}

/* --- state queries ---------------------------------------------------- */

menu_param_t menu_current(void) { return cur; }
menu_mode_t  menu_mode(void)    { return mode; }

int          menu_world_index(void)    { return world_i; }
const char  *menu_world_subtitle(void) { return worlds_get(world_i)->subtitle; }

const char *menu_current_label(void) {
    static const char * const LABELS[MP_COUNT] = {
        "World","Key","Voice","Space","Atmosphere","Motion","Age","Echo","Blur"
    };
    return LABELS[cur];
}

int menu_value_index(menu_param_t p) {
    switch (p) {
        case MP_WORLD: return world_i;
        case MP_KEY:   return key_pc;
        case MP_VOICE: return voice_i;
        default: return 0;
    }
}

int menu_value_int(menu_param_t p) {
    switch (p) {
        case MP_SPACE:  return space;
        case MP_ATMOS:  return atmos;
        case MP_MOTION: return motion;
        case MP_AGE:    return age;
        case MP_ECHO:   return echo;
        case MP_BLUR:   return blur;
        default:        return 0;
    }
}

/* Number of discrete options a parameter can step through. 0 = continuous (a
 * 0..100 % amount) — rendered as a fill bar instead of one pill per option. */
int menu_value_count(menu_param_t p) {
    switch (p) {
        case MP_WORLD: return worlds_count();
        case MP_KEY:   return 12;
        case MP_VOICE: return 3;
        default:       return 0;   /* SPACE / ATMOS / MOTION / AGE = % */
    }
}

const char *menu_current_value_text(void) {
    static char buf[12];
    switch (cur) {
        case MP_WORLD:  return worlds_get(world_i)->name;
        case MP_KEY:    return KEY_NAMES[key_pc];
        case MP_VOICE:  return VOICE_NAMES[voice_i];
        case MP_SPACE:  snprintf(buf, sizeof buf, "%d%%", space);  return buf;
        case MP_ATMOS:  snprintf(buf, sizeof buf, "%d%%", atmos);  return buf;
        case MP_MOTION: snprintf(buf, sizeof buf, "%d%%", motion); return buf;
        case MP_AGE:    snprintf(buf, sizeof buf, "%d%%", age);    return buf;
        case MP_ECHO:   snprintf(buf, sizeof buf, "%d%%", echo);   return buf;
        case MP_BLUR:   snprintf(buf, sizeof buf, "%d%%", blur);   return buf;
        default: return "";
    }
}

/* --- input ------------------------------------------------------------ */

void menu_push(void) {
    mode = (mode == MENU_BROWSE) ? MENU_EDIT : MENU_BROWSE;
}

/* `delta` is a signed TICK COUNT, not a fixed percent. The input layer owns
 * the feel: one deliberate detent = ±1, an accelerated fast spin passes a
 * larger magnitude. Continuous params move 1 % per tick. */
void menu_rotate(int delta) {
    if (mode == MENU_BROWSE) {
        int dir = (delta > 0) - (delta < 0);
        cur = (menu_param_t)wrapi((int)cur + dir, MP_COUNT);
        return;
    }
    /* edit: step the current slot's value */
    switch (cur) {
        case MP_WORLD:
            world_i = wrapi(world_i + delta, worlds_count());
            load_world_preset();        /* selecting a world loads its preset */
            return;                     /* preset push covers the callbacks   */
        case MP_KEY:    key_pc  = wrapi(key_pc  + delta, 12); break;
        case MP_VOICE:  voice_i = wrapi(voice_i + delta, 3);  break;
        case MP_SPACE:  space  = clampi(space  + delta, 0, 100); break;
        case MP_ATMOS:  atmos  = clampi(atmos  + delta, 0, 100); break;
        case MP_MOTION: motion = clampi(motion + delta, 0, 100); break;
        case MP_AGE:    age    = clampi(age    + delta, 0, 100); break;
        case MP_ECHO:   echo   = clampi(echo   + delta, 0, 100); break;
        case MP_BLUR:   blur   = clampi(blur   + delta, 0, 100); break;
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

/* Battery: rounded outline + terminal nub + rounded fill, top-right. */
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

/* Bottom selector — one pill per OPTION OF THE CURRENT SETTING, edge-to-edge.
 * Continuous % settings use render_fillbar() instead. */
#define BAR_Y      148
#define BAR_H_INA  5
#define BAR_H_ACT  8
#define BAR_GAP    6
#define BAR_ACT_K  24     /* active = K/10 × inactive width (≈2.4×) */

static void render_bar(int n, int active) {
    if (n < 1) return;
    int usable = OLED_WIDTH - PAD_L - PAD_R;

    int gap = BAR_GAP;
    int gaps_total = (n - 1) * gap;
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

    int used = act_w + (n - 1) * ina_w + gaps_total;
    int residue = usable - used;

    int x = PAD_L;
    for (int i = 0; i < n; ++i) {
        bool act = (i == active);
        int w = act ? act_w : ina_w;
        if (!act && residue > 0) { w += 1; --residue; }
        int h  = act ? BAR_H_ACT : BAR_H_INA;
        int yy = BAR_Y + (BAR_H_ACT - h) / 2;
        uint8_t gs = act ? GS_ACTIVE : GS_DASH;
        oled_pill(x, yy, w, h, gs);
        x += w + gap;
    }
}

/* Continuous % setting: a single rounded track spanning edge-to-edge with a
 * bright fill proportional to the 0..100 value. */
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

/* Big value in Helvetica Neue Light, LEFT-aligned. Falls to the small face if
 * a long word would run into the battery / right edge (e.g. world names). */
static void render_value(void) {
    const char *txt = menu_current_value_text();
    uint8_t gs = (mode == MENU_EDIT) ? GS_ACTIVE : 13;
    const bakedfont_t *f = &font_hn_value;
    int avail = OLED_WIDTH - PAD_L - PAD_R;
    if (bfont_width(f, txt) > avail) f = &font_hn_value_small;

    int top    = PAD_T + (int)font_hn_label.line;
    int bottom = BAR_Y;
    int y = (top + bottom - (int)f->line) / 2;
    bfont_draw(f, PAD_L, y, txt, gs);

    /* On the WORLD slot, show the flavour subtitle under the world name so the
     * world identity reads beyond just the name. Quiet, secondary tone. */
    if (cur == MP_WORLD) {
        int sy = y + (int)f->line + 2;
        if (sy + (int)font_hn_label.line < BAR_Y - 4)
            bfont_draw(&font_hn_label, PAD_L, sy, menu_world_subtitle(), GS_DIM);
    }
}

void menu_render(void) {
    oled_fill(GS_BG);

    uint8_t lbl_gs = (mode == MENU_EDIT) ? GS_DIM : GS_LABEL;
    bfont_draw(&font_hn_label, PAD_L, PAD_T, menu_current_label(), lbl_gs);

    render_battery();
    render_value();

    if (mode == MENU_BROWSE) {
        render_bar(MP_COUNT, (int)cur);
    } else {
        int n = menu_value_count(cur);
        if (n > 0) render_bar(n, menu_value_index(cur));
        else       render_fillbar(menu_value_int(cur));
    }

    mask_corners();
}

void menu_render_bar_only(int total, int active) { render_bar(total, active); }
