/*
 * display_hw_test — das Hardware-Gegenstück zur GitHub-Pages-Display-Sim.
 *
 * Die Sim (tools/display_sim.html) ist ein JS-Port von src/menu.c inklusive
 * Animations-Engine, Velocity-Acceleration und SHIFT+Backlight-Overlay.
 * DIESES Programm portiert all das zurück auf echte Hardware: das echte
 * menu.c als State-Machine, dazu ein animierter Renderer (Tween-Engine wie
 * in der Sim), Encoder-Beschleunigung und SHIFT-Gesten — Steckbrett-Aufbau
 * mit Pico 2, bevor es eine Geräte-Platine gibt.
 *
 * ───────────────────────────────────────────────────────────────────────────
 * HARDWARE
 *   - Raspberry Pi Pico 2 (RP2350)
 *   - Adafruit 1.9" 320×170 IPS TFT, ST7789 (Adafruit #5394 / buyzero)
 *   - EC11/KY-040 Push-Encoder (GND, +, SW, DT, CLK)
 *   - 1× Tactile-Button als SHIFT
 *
 * VERDRAHTUNG — Display (Pin-Namen wie auf dem Adafruit-Silk):
 *   Adafruit-Pin   Pico-GPIO   Pico-Pin (physisch)
 *   Vin            3V3 (OUT)   36          (Board hat eigenen Regler)
 *   Gnd            GND         38 (o. a. GND)
 *   SCK            GP18        24
 *   MOSI           GP19        25
 *   TFT_CS (TCS!)  GP17        22          ← TCS, NICHT SDCS
 *   D/C            GP16        21
 *   RST            GP20        26
 *   Lite           GP22        29          ← Backlight-PWM (SHIFT+drehen)
 *   MISO, SD_CS    unverbunden (SD-Slot wird nicht benutzt)
 *
 * VERDRAHTUNG — Encoder:
 *   Encoder-Pin    Pico-GPIO   Pico-Pin (physisch)
 *   GND            GND         3
 *   +              3V3 (OUT)   36
 *   CLK            GP2         4
 *   DT             GP3         5
 *   SW             GP4         6
 *
 * VERDRAHTUNG — SHIFT-Taster:
 *   ein Bein an GP5 (Pico-Pin 7), das andere an GND.
 *
 * (Alle Pins zentral unten bei "pin map" — per CMake überschreibbar.)
 *
 * BUILD (Pico SDK 2.x, PICO_SDK_PATH gesetzt):
 *   cd firmware-c-next && mkdir -p build && cd build
 *   cmake .. -DPICO_BOARD=pico2
 *   make -j display_hw_test
 *   → build/display_hw_test.uf2 per BOOTSEL auf den Pico 2 ziehen.
 *   (Oder ohne Toolchain: CI-Artefakt "firmware-c-next-pico2-uf2" laden.)
 *
 * BEDIENUNG (Mapping zur Sim):
 *   Encoder drehen        Menü browse / Wert editieren. Bei %-Werten gilt
 *                         Velocity-Acceleration wie in der Sim: langsam
 *                         drehen = feine 1 %-Schritte, schnell = grobe
 *                         Sprünge. Browse + diskrete Werte: 1 Schritt/Rastung.
 *   Encoder drücken       Edit-Modus rein/raus.
 *   SHIFT 1× tippen       Brightness-Modus AN: Overlay "Backlight XX%" bleibt
 *                         stehen, der Encoder regelt jetzt die Helligkeit
 *                         (PWM GP22). Velocity-Acceleration wie sonst.
 *     im Brightness-Modus:
 *       Encoder drehen    Helligkeit ±. Beschleunigung wie bei %-Werten.
 *       Encoder drücken   Helligkeit auf 100 %.
 *       SHIFT erneut      RAUS: Overlay fadet, Encoder zurück am Menü.
 *   SHIFT lang halten     Testbild weiterschalten (nur im normalen Browse,
 *   (1,5 s ohne Tippen)   nicht im Brightness-Modus, nicht in Testbildern):
 *                         MENU → RAMP → CHECKER → TYPE → MENU.
 *
 * TESTBILDER (Panel-Verifikation, gibt es in der Sim nicht):
 *   RAMP    16 Graustufen + 1-px-Rahmen + Ecken-Ticks. Fehlende Rahmenkante
 *           → LCD_Y_OFFSET/MADCTL in src/lcd_st7789.c anpassen.
 *   CHECKER 1-px-Schachbrett. Schmieren/Rauschen → SPI zu schnell für die
 *           Kabel (LCD_SPI_HZ in CMakeLists.txt senken).
 *   TYPE    Schrift-/Primitiven-Probe.
 *
 * TROUBLESHOOTING:
 *   Display schwarz, Backlight aus → Lite/GP22 prüfen (testweise an 3V3)
 *   Backlight an, kein Bild → SCK/MOSI/TCS/DC/RST prüfen; TCS nicht SDCS!
 *   Bild gespiegelt/kopfüber → LCD_MADCTL in lcd_st7789.c (0x60↔0xA0/0xC0)
 *   Menü dreht falsch herum → ENC_DIR unten auf -1
 *   Encoder reagiert nur auf jede 2. Rastung → ENC_HALF_STEP=1 (EC11-Variante
 *   mit Halb-Zyklus-Rastung)
 *   Animationen ruckeln → kürzere Kabel, dann LCD_SPI_HZ=32000000 probieren
 *   USB-Konsole (loggt jeden Event): screen /dev/ttyACM0 115200
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"

#include "oled.h"
#include "menu.h"
#include "battery.h"
#include "baked_font.h"

/* --- pin map (breadboard wiring; display SPI pins live in lcd_st7789.c,
 *     overridden for this target in CMakeLists.txt) ----------------------- */
#ifndef PIN_ENC_CLK
#define PIN_ENC_CLK  2
#endif
#ifndef PIN_ENC_DT
#define PIN_ENC_DT   3
#endif
#ifndef PIN_ENC_SW
#define PIN_ENC_SW   4
#endif
#ifndef PIN_SHIFT
#define PIN_SHIFT    5
#endif
#ifndef PIN_BL
#define PIN_BL      22       /* Lite — backlight PWM */
#endif

/* Flip to -1 if rotating clockwise moves the menu the wrong way. */
#ifndef ENC_DIR
#define ENC_DIR (+1)
#endif

/* Some EC11 variants detent every HALF quadrature cycle (2 transitions),
 * resting alternately at 00 and 11. Symptom on such a part: the menu reacts
 * only to every SECOND detent. Build with ENC_HALF_STEP=1 in that case. */
#ifndef ENC_HALF_STEP
#define ENC_HALF_STEP 0
#endif

#define LONG_PRESS_MS  1500  /* SHIFT long-press = scene switch             */
#define DEBOUNCE_MS    20
#define OVERLAY_HOLD_MS 1100 /* transient overlay fade-out (post-toggle)    */

/* --- layout constants — must mirror src/menu.c (and the sim) ------------- */
#define PAD_L      22
#define PAD_R      18
#define PAD_T      14
#define BAR_Y      148
#define BAR_H_INA  5
#define BAR_H_ACT  8
#define BAR_GAP    6
#define BAR_ACT_K  24        /* active = K/10 × inactive width              */
#define GS_DASH    3
#define GS_DIM     5
#define GS_LABEL   9
#define GS_MID     9
#define GS_VAL     13
#define GS_ACTIVE  15

/* ========================================================================= */
/* 1 kHz input sampler (timer IRQ) — emits debounced edges + raw detents.   */
/* All gesture LOGIC lives in the main loop; the sampler stays dumb.        */
/* ========================================================================= */

static volatile int  ev_detents;        /* signed encoder detents           */
static volatile bool ev_sw_press;       /* encoder switch press edge        */
static volatile bool ev_shift_press;    /* SHIFT press edge                 */
static volatile bool ev_shift_release;  /* SHIFT release edge               */
static volatile bool shift_level;       /* debounced SHIFT held state       */

static const int8_t QDELTA[16] = {
     0, +1, -1,  0,
    -1,  0,  0, +1,
    +1,  0,  0, -1,
     0, -1, +1,  0,
};

typedef struct {
    uint     pin;
    bool     stable;        /* debounced level (true = released, pull-up)  */
    uint16_t flip_ms;
} debounce_t;

static debounce_t db_sw    = { .pin = PIN_ENC_SW, .stable = true };
static debounce_t db_shift = { .pin = PIN_SHIFT,  .stable = true };

/* Returns +1 on press edge, -1 on release edge, 0 otherwise. */
static int debounce_tick(debounce_t *b) {
    bool raw = gpio_get(b->pin);
    if (raw != b->stable) {
        if (++b->flip_ms >= DEBOUNCE_MS) {
            b->stable  = raw;
            b->flip_ms = 0;
            return raw ? -1 : +1;       /* low = pressed (pull-up)          */
        }
    } else {
        b->flip_ms = 0;
    }
    return 0;
}

static bool sampler_1khz(struct repeating_timer *t) {
    (void)t;

    /* Quadrature: one detent per full 4-transition cycle, latched at the
     * rest position (CLK=DT=high on EC11/KY-040 detents). Invalid (bounce)
     * transitions contribute 0 — inherently debounced. */
    static uint8_t q_prev = 3;
    static int     q_acc  = 0;
    uint8_t cur = (uint8_t)((gpio_get(PIN_ENC_CLK) << 1) | gpio_get(PIN_ENC_DT));
    q_acc += QDELTA[(q_prev << 2) | cur];
    q_prev = cur;
#if ENC_HALF_STEP
    if (cur == 3 || cur == 0) {              /* rest at 00 AND 11           */
        if (q_acc >= 2)       ev_detents += ENC_DIR;
        else if (q_acc <= -2) ev_detents -= ENC_DIR;
        q_acc = 0;
    }
#else
    if (cur == 3) {
        if (q_acc >= 4)       ev_detents += ENC_DIR;
        else if (q_acc <= -4) ev_detents -= ENC_DIR;
        q_acc = 0;
    }
#endif

    if (debounce_tick(&db_sw) > 0) ev_sw_press = true;

    int se = debounce_tick(&db_shift);
    if (se > 0)      { ev_shift_press = true;   shift_level = true;  }
    else if (se < 0) { ev_shift_release = true; shift_level = false; }
    return true;
}

/* ========================================================================= */
/* Velocity acceleration — port of the sim's ACCEL_TIERS (SPEC §5).         */
/* One deliberate detent = 1 %; spin faster and each detent is worth more.  */
/* ========================================================================= */

static int accel_ticks(int dir, uint32_t now_ms) {
    static const struct { uint32_t max_dt; int ticks; } TIERS[] = {
        { 28, 8 }, { 60, 5 }, { 120, 3 }, { 240, 2 },
    };
    static uint32_t last_ms = 0;
    static bool     first   = true;
    uint32_t dt = first ? UINT32_MAX : now_ms - last_ms;
    last_ms = now_ms;
    first   = false;
    int mul = 1;
    for (unsigned i = 0; i < sizeof TIERS / sizeof TIERS[0]; ++i)
        if (dt < TIERS[i].max_dt) { mul = TIERS[i].ticks; break; }
    return dir * mul;
}

/* ========================================================================= */
/* Backlight — PWM on PIN_BL. Gamma-2 mapping so the % FEELS linear         */
/* (LED backlights look near-full already at 50 % duty). Boots to the       */
/* factory default 80 % (SPEC §12.5: never start dark).                     */
/* ========================================================================= */

#define BL_PWM_WRAP 999
static int backlight_pct = 80;

static void backlight_apply(void) {
    uint32_t p = (uint32_t)backlight_pct;
    uint32_t duty = (BL_PWM_WRAP * p * p) / (100u * 100u);   /* gamma 2 */
    if (duty == 0 && p > 0) duty = 1;   /* 1-3 % must glow, not turn OFF   */
    pwm_set_gpio_level(PIN_BL, (uint16_t)duty);
}

static void backlight_init(void) {
    gpio_set_function(PIN_BL, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_BL);
    pwm_set_wrap(slice, BL_PWM_WRAP);
    pwm_set_enabled(slice, true);
    backlight_apply();
}

/* ========================================================================= */
/* Tween engine — port of the sim's rAF loop. One channel per animated      */
/* value; starting a new tween on a channel replaces the old one (from the  */
/* current value), so rapid input never stutters. Ease-out cubic is the     */
/* house curve. Time base: ms since boot.                                   */
/* ========================================================================= */

enum {
    A_BART = 0,   /* fractional active position of the pill bar  */
    A_BARALPHA,   /* bar alpha — dips during mode changes        */
    A_FILLT,      /* animated 0..100 for the fill bar            */
    A_TEXTY,      /* vertical wipe offset for label+value        */
    A_TEXTALPHA,
    A_LABELC,     /* 0 = browse label tone, 1 = edit tone        */
    A_OVALPHA,    /* overlay fade                                */
    A_OVFILL,     /* animated 0..100 for the overlay fill        */
    A_COUNT
};

static float anim[A_COUNT] = { 0, 1, 20, 0, 1, 0, 0, 0 };

typedef struct {
    bool     on;
    float    from, to;
    uint32_t t0, dur;
} tween_t;

static tween_t tweens[A_COUNT];

static float ease_out_cubic(float t) {
    float u = 1.0f - t;
    return 1.0f - u * u * u;
}

static void tween_to(int key, float to, uint32_t dur, uint32_t now) {
    tweens[key] = (tween_t){ .on = true, .from = anim[key], .to = to,
                             .t0 = now, .dur = dur ? dur : 1 };
}

static bool tweens_tick(uint32_t now) {
    bool any = false;
    for (int k = 0; k < A_COUNT; ++k) {
        if (!tweens[k].on) continue;
        float u = (float)(now - tweens[k].t0) / (float)tweens[k].dur;
        if (u >= 1.0f) {
            anim[k] = tweens[k].to;
            tweens[k].on = false;
        } else {
            anim[k] = tweens[k].from
                    + (tweens[k].to - tweens[k].from) * ease_out_cubic(u);
            any = true;
        }
    }
    return any;
}

/* --- overlay state (Backlight readout) -----------------------------------
 * `sticky` = the user is in brightness toggle mode; the overlay stays up
 * until they tap SHIFT again. `fade_out_at` is only consulted when sticky
 * is false, so a non-sticky readout (e.g. a SHIFT-toggle confirmation that
 * may later be added for other readouts) still auto-dismisses. */
static struct {
    bool     active;
    bool     sticky;
    char     label[16];
    uint32_t fade_out_at;
    bool     closing;
} overlay;

/* --- bar fade-out → snap → fade-in sequence (sim's onModeChange) --------- */
static struct {
    bool  pending;          /* waiting for barAlpha to reach 0              */
    float snap_bar_t;       /* barT to snap to when it does                 */
    float snap_fill;        /* fillT to snap to (continuous edit only)      */
    bool  snap_fill_valid;
} barseq;

/* ========================================================================= */
/* Animation triggers — 1:1 ports of the sim's onCurChange / onValueChange  */
/* / onModeChange / showOverlay / dismissOverlay.                           */
/* ========================================================================= */

static bool cur_is_continuous(void) {
    return menu_value_count(menu_current()) == 0;
}

static void on_cur_change(int dir, uint32_t now) {
    anim[A_TEXTY]     = 7.0f * (dir >= 0 ? 1.0f : -1.0f);
    anim[A_TEXTALPHA] = 0.0f;
    tween_to(A_TEXTY, 0, 220, now);
    tween_to(A_TEXTALPHA, 1, 220, now);
    tween_to(A_BART, (float)menu_current(), 240, now);
    if (cur_is_continuous())
        tween_to(A_FILLT, (float)menu_value_int(menu_current()), 240, now);
}

static void on_value_change(uint32_t now) {
    if (cur_is_continuous()) {
        tween_to(A_FILLT, (float)menu_value_int(menu_current()), 200, now);
        /* value text reads anim[A_FILLT], so the number counts up smoothly */
    } else {
        tween_to(A_BART, (float)menu_value_index(menu_current()), 180, now);
        anim[A_TEXTALPHA] = 0.35f;
        tween_to(A_TEXTALPHA, 1, 220, now);
    }
}

static void on_mode_change(uint32_t now) {
    /* Bar topology changes: fade out (110 ms), snap, fade in (180 ms). */
    tween_to(A_BARALPHA, 0, 110, now);
    barseq.pending = true;
    if (menu_mode() == MENU_BROWSE) {
        barseq.snap_bar_t = (float)menu_current();
        barseq.snap_fill_valid = false;
    } else {
        barseq.snap_bar_t = (float)menu_value_index(menu_current());
        barseq.snap_fill_valid = cur_is_continuous();
        barseq.snap_fill = (float)menu_value_int(menu_current());
    }
    tween_to(A_LABELC, menu_mode() == MENU_EDIT ? 1.0f : 0.0f, 200, now);
    anim[A_TEXTALPHA] = 0.5f;
    tween_to(A_TEXTALPHA, 1, 220, now);
}

/* sticky=true → overlay stays up until dismiss_overlay() is called (used by
 * the SHIFT brightness toggle). sticky=false → auto-fades after the hold.    */
static void show_overlay(const char *label, int value, bool sticky, uint32_t now) {
    if (!overlay.active || strcmp(overlay.label, label) != 0)
        anim[A_OVFILL] = (float)value;       /* fresh start, no cross-slide */
    overlay.active = true;
    overlay.closing = false;
    overlay.sticky = sticky;
    snprintf(overlay.label, sizeof overlay.label, "%s", label);
    overlay.fade_out_at = now + OVERLAY_HOLD_MS;
    tween_to(A_OVALPHA, 1, 110, now);
    tween_to(A_OVFILL, (float)value, 180, now);
}

static void dismiss_overlay(uint32_t now) {
    if (!overlay.active && anim[A_OVALPHA] <= 0.01f) return;
    overlay.closing = true;
    overlay.sticky  = false;                 /* dismiss always wins         */
    tween_to(A_OVALPHA, 0, 160, now);
}

/* Per-frame housekeeping the sim does inside its rAF loop. */
static void anim_housekeeping(uint32_t now) {
    if (barseq.pending && !tweens[A_BARALPHA].on && anim[A_BARALPHA] <= 0.01f) {
        barseq.pending = false;
        anim[A_BART] = barseq.snap_bar_t;
        tweens[A_BART].on = false;
        if (barseq.snap_fill_valid) {
            anim[A_FILLT] = barseq.snap_fill;
            tweens[A_FILLT].on = false;
        }
        tween_to(A_BARALPHA, 1, 180, now);
    }
    /* Auto-dismiss only the transient (non-sticky) variant; the brightness
     * toggle is sticky and must persist until the user taps SHIFT again. */
    if (overlay.active && !overlay.sticky && !overlay.closing
        && now >= overlay.fade_out_at)
        dismiss_overlay(now);
    if (overlay.closing && !tweens[A_OVALPHA].on && anim[A_OVALPHA] <= 0.01f) {
        overlay.active = false;
        overlay.closing = false;
    }
}

/* ========================================================================= */
/* Animated renderer — port of the sim's drawMenu/drawPillBar/drawFillbar/  */
/* drawBattery/drawOverlayView. menu.c stays the state machine; this        */
/* renderer replaces the static menu_render() with the tweened version.     */
/* All draw primitives are max-blended, so the overlay crossfade (both      */
/* layers drawn dimmed) composites cleanly.                                 */
/* ========================================================================= */

static uint8_t gs_a(int gs, float alpha) {
    if (alpha <= 0) return 0;
    if (alpha >= 1) return (uint8_t)gs;
    int v = (int)((float)gs * alpha + 0.5f);
    return (uint8_t)(v > 15 ? 15 : v);
}

static void render_battery_hud(void) {
    const int w = 26, h = 12, r = 4;
    const int x = OLED_WIDTH - PAD_R - w - 3;
    const int y = PAD_T + 2;
    oled_rrect_stroke(x, y, w, h, r, 1, GS_MID);
    oled_rrect_fill(x + w + 1, y + 3, 2, h - 6, 1, GS_MID);

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

/* Pill row with a FRACTIONAL active position (sim's drawPillBar): while
 * activeT slides between two pills both grow/brighten proportionally, the
 * widths sum is conserved, so the row stays anchored at both margins. */
static void render_bar_anim(int n, float active_t, float alpha) {
    if (n < 1 || alpha <= 0.01f) return;
    float usable = (float)(OLED_WIDTH - PAD_L - PAD_R);
    float gap = BAR_GAP;
    float k   = (float)BAR_ACT_K / 10.0f;
    float ina = (usable - (float)(n - 1) * gap) / ((float)(n - 1) + k);
    if (ina < 4.0f && n > 1) {
        gap = 3;
        ina = (usable - (float)(n - 1) * gap) / ((float)(n - 1) + k);
        if (ina < 3.0f) ina = 3.0f;
    }
    float act = ina * k;
    if (n == 1) {
        oled_pill(PAD_L, BAR_Y, (int)usable, BAR_H_ACT, gs_a(GS_ACTIVE, alpha));
        return;
    }
    float x = PAD_L;
    for (int i = 0; i < n; ++i) {
        float di = 1.0f - (active_t > (float)i ? active_t - (float)i
                                               : (float)i - active_t);
        if (di < 0) di = 0;
        float w = ina + di * (act - ina);
        float h = (float)BAR_H_INA + di * (float)(BAR_H_ACT - BAR_H_INA);
        int   hi = (int)(h + 0.5f);
        int   yy = BAR_Y + (BAR_H_ACT - hi) / 2;
        float a  = 0.20f + di * 0.80f;          /* dim → active brightness  */
        oled_pill((int)(x + 0.5f), yy, (int)(w + 0.5f), hi,
                  gs_a(GS_ACTIVE, a * alpha));
        x += w + gap;
    }
}

static void render_fillbar_anim(float pct, float alpha) {
    if (alpha <= 0.01f) return;
    int x = PAD_L;
    int w = OLED_WIDTH - PAD_L - PAD_R;
    int h = BAR_H_ACT;
    oled_pill(x, BAR_Y, w, h, gs_a(GS_DASH, alpha));
    int fw = (int)((float)w * pct / 100.0f + 0.5f);
    if (fw < h && pct > 0.5f) fw = h;
    if (fw > w) fw = w;
    if (fw > 0) oled_rrect_fill(x, BAR_Y, fw, h, h / 2, gs_a(GS_ACTIVE, alpha));
}

/* Label top-left + big value, with the wipe offset/alpha applied (sim's
 * drawValueRow). `layer_alpha` folds in the overlay crossfade. */
static void render_value_row(const char *label, const char *value,
                             int label_gs, int value_gs, float layer_alpha,
                             bool apply_wipe) {
    float a   = layer_alpha * (apply_wipe ? anim[A_TEXTALPHA] : 1.0f);
    int   dy  = apply_wipe ? (int)(anim[A_TEXTY] + (anim[A_TEXTY] >= 0 ? 0.5f : -0.5f)) : 0;
    if (a <= 0.01f) return;

    bfont_draw(&font_hn_label, PAD_L, PAD_T + dy, label, gs_a(label_gs, a));

    const bakedfont_t *f = &font_hn_value;
    int avail = OLED_WIDTH - PAD_L - PAD_R;
    if (bfont_width(f, value) > avail) f = &font_hn_value_small;
    int top    = PAD_T + (int)font_hn_label.line;
    int bottom = BAR_Y;
    int y = (top + bottom - (int)f->line) / 2;
    bfont_draw(f, PAD_L, y + dy, value, gs_a(value_gs, a));
}

static float lerpf(float a, float b, float t) { return a + (b - a) * t; }

static void render_menu_layer(float layer_alpha) {
    /* label tone crossfades browse(9) → edit(5); value 13 → 15 (sim). */
    float lc = anim[A_LABELC];
    int label_gs = (int)(lerpf(GS_LABEL, GS_DIM, lc) + 0.5f);
    int value_gs = (int)(lerpf(GS_VAL, GS_ACTIVE, lc) + 0.5f);

    char buf[8];
    const char *vtxt;
    if (cur_is_continuous()) {
        /* number follows the ANIMATED fill so it counts up smoothly */
        snprintf(buf, sizeof buf, "%d%%", (int)(anim[A_FILLT] + 0.5f));
        vtxt = buf;
    } else {
        vtxt = menu_current_value_text();
    }
    render_value_row(menu_current_label(), vtxt, label_gs, value_gs,
                     layer_alpha, true);

    float bar_a = anim[A_BARALPHA] * layer_alpha;
    if (menu_mode() == MENU_BROWSE) {
        render_bar_anim(MP_COUNT, anim[A_BART], bar_a);
    } else {
        int n = menu_value_count(menu_current());
        if (n > 0) render_bar_anim(n, anim[A_BART], bar_a);
        else       render_fillbar_anim(anim[A_FILLT], bar_a);
    }
}

static void render_overlay_layer(float layer_alpha) {
    char buf[8];
    snprintf(buf, sizeof buf, "%d%%", (int)(anim[A_OVFILL] + 0.5f));
    render_value_row(overlay.label, buf, GS_LABEL, GS_ACTIVE, layer_alpha, false);
    render_fillbar_anim(anim[A_OVFILL], layer_alpha);
}

/* Round the 4 screen corners to black (mirrors menu.c's mask_corners). */
static void mask_corners(void) {
    const int r = 16;
    for (int yy = 0; yy < r; ++yy)
        for (int xx = 0; xx < r; ++xx) {
            int dx = r - 1 - xx, dy = r - 1 - yy;
            if (dx * dx + dy * dy > r * r) {
                oled_pixel(xx, yy, 0);
                oled_pixel(OLED_WIDTH - 1 - xx, yy, 0);
                oled_pixel(xx, OLED_HEIGHT - 1 - yy, 0);
                oled_pixel(OLED_WIDTH - 1 - xx, OLED_HEIGHT - 1 - yy, 0);
            }
        }
}

static void render_menu_scene(void) {
    oled_fill(0);
    render_battery_hud();                    /* always-on HUD, full alpha   */
    float ov = anim[A_OVALPHA];
    if (overlay.active || ov > 0.01f) {
        render_menu_layer(1.0f - ov);        /* menu dims under the overlay */
        render_overlay_layer(ov);
    } else {
        render_menu_layer(1.0f);
    }
    mask_corners();
}

/* ========================================================================= */
/* Test scenes (panel verification — unchanged from the first bench build). */
/* Labels: the 8x8 font has no 'Q'/'Z'/'=' — stick to 0-9 A-P R-Y . - + # % */
/* ========================================================================= */

typedef enum { SCENE_MENU = 0, SCENE_RAMP, SCENE_CHECKER, SCENE_TYPE, SCENE_COUNT } scene_t;

static const char *SCENE_NAME[SCENE_COUNT] = { "MENU", "RAMP", "CHECKER", "TYPE" };

static void render_frame_and_corners(void) {
    oled_rect_fill(0, 0, OLED_WIDTH, 1, 15);
    oled_rect_fill(0, OLED_HEIGHT - 1, OLED_WIDTH, 1, 15);
    oled_rect_fill(0, 0, 1, OLED_HEIGHT, 15);
    oled_rect_fill(OLED_WIDTH - 1, 0, 1, OLED_HEIGHT, 15);
    oled_rect_fill(1, 1, 12, 3, 15);
    oled_rect_fill(1, 1, 3, 12, 15);
    oled_rect_fill(OLED_WIDTH - 7, 1, 6, 3, 15);
    oled_rect_fill(1, OLED_HEIGHT - 4, 6, 3, 15);
}

static void render_ramp(void) {
    oled_fill(0);
    for (int i = 0; i < 16; ++i) {
        int x0 = (OLED_WIDTH * i) / 16;
        int x1 = (OLED_WIDTH * (i + 1)) / 16;
        oled_rect_fill(x0, 24, x1 - x0, 104, (uint8_t)i);
    }
    render_frame_and_corners();
    oled_text(8, 8,   "RAMP 0-15", 12);
    oled_text(8, 150, "FRAME - PANEL EDGE - OFFSET OK", 8);
}

static void render_checker(void) {
    for (int y = 0; y < OLED_HEIGHT; ++y)
        for (int x = 0; x < OLED_WIDTH; ++x)
            oled_pixel(x, y, ((x ^ y) & 1) ? 15 : 0);
    oled_rect_fill(6, OLED_HEIGHT - 18, 96, 12, 0);
    oled_text(8, OLED_HEIGHT - 16, "CHECKER 1PX", 12);
}

static void render_type(void) {
    oled_fill(0);
    oled_text(8, 8, "TYPE TEST", 12);
    oled_text(8, 26, "GREY 4 8 12 15", 4);
    oled_text(72, 26, "8", 8);
    oled_text(88, 26, "12", 12);
    oled_text(112, 26, "15", 15);
    oled_text_smooth(8, 44, "SMOOTH 2X", 15, 2);
    oled_text_smooth(8, 70, "SMOOTH 3X", 15, 3);
    oled_pill(8, 108, 80, 10, 15);
    oled_pill(96, 108, 24, 10, 6);
    oled_rrect_stroke(140, 100, 160, 56, 16, 2, 12);
    oled_text(152, 122, "RRECT R16", 10);
    render_frame_and_corners();
}

/* ========================================================================= */
/* Battery demo (mirrors the sim's "Akku −10" / charge-source buttons).     */
/* ========================================================================= */
/* (Battery is hard-coded to a sane value below — on a real PCB the ADC      */
/* shim wired in battery.c will replace that. The sim's "Akku −10" button    */
/* lived in JS and has no useful place on the bench.)                        */
/* ========================================================================= */

/* ========================================================================= */
/* Main                                                                      */
/* ========================================================================= */

int main(void) {
    stdio_init_all();

    const uint pins[] = { PIN_ENC_CLK, PIN_ENC_DT, PIN_ENC_SW, PIN_SHIFT };
    for (unsigned i = 0; i < sizeof pins / sizeof pins[0]; ++i) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
    }

    backlight_init();                        /* 80 % before the panel wakes */
    oled_init();
    menu_init(NULL);                         /* no engine on the bench      */
    battery_set_usb_present(false);
    battery_set_pct(100);

    scene_t scene = SCENE_MENU;
    anim[A_BART]  = (float)menu_current();
    anim[A_FILLT] = (float)menu_value_int(menu_current());
    render_menu_scene();
    oled_show();

    struct repeating_timer timer;
    add_repeating_timer_us(-1000, sampler_1khz, NULL, &timer);

    printf("display_hw_test up — breadboard wiring, scene=%s\n", SCENE_NAME[scene]);
    printf("display: SCK GP18 MOSI GP19 TCS GP17 DC GP16 RST GP20 BL GP%d\n", PIN_BL);
    printf("encoder: CLK GP%d DT GP%d SW GP%d  SHIFT GP%d\n",
           PIN_ENC_CLK, PIN_ENC_DT, PIN_ENC_SW, PIN_SHIFT);

    /* SHIFT is a TOGGLE button:
     *   1× tap  → brightness_mode = true  (overlay sticks, encoder = BL)
     *   2× tap  → brightness_mode = false (overlay fades, encoder = menu)
     *   long-hold (no rotate) → cycle test scene (panel verification)
     * `shift_t0` tracks the current press for long-press classification;
     * `shift_rotated` and `shift_longed` veto the tap action on release so
     * a long-press or a press-while-held-rotate never doubles as a toggle. */
    static bool brightness_mode = false;
    bool     shift_rotated = false;
    bool     shift_longed  = false;
    uint32_t shift_t0      = 0;

    uint32_t last_beat = to_ms_since_boot(get_absolute_time());

    while (true) {
        uint32_t now = to_ms_since_boot(get_absolute_time());

        /* drain sampler events atomically */
        uint32_t irq = save_and_disable_interrupts();
        int  detents   = ev_detents;      ev_detents = 0;
        bool sw_press  = ev_sw_press;     ev_sw_press = false;
        bool sh_press  = ev_shift_press;  ev_shift_press = false;
        bool sh_release= ev_shift_release;ev_shift_release = false;
        bool sh_held   = shift_level;
        restore_interrupts(irq);

        bool dirty = false;

        /* --- SHIFT bookkeeping -----------------------------------------
         * Release BEFORE press: a release in the same drain as a new press
         * belongs to the previous press, so flags must still be those of
         * the press that ended (otherwise a quick second tap would clobber
         * shift_rotated/longed and turn a finished long-press into a
         * spurious toggle). */
        if (sh_release && !shift_rotated && !shift_longed) {
            /* short tap → TOGGLE brightness mode */
            brightness_mode = !brightness_mode;
            if (brightness_mode) {
                show_overlay("Backlight", backlight_pct, /*sticky=*/true, now);
            } else {
                dismiss_overlay(now);
            }
            printf("shift toggle -> brightness_mode=%d\n", (int)brightness_mode);
            dirty = true;
        }
        if (sh_press) {
            shift_t0 = now;
            shift_rotated = false;
            shift_longed = false;
        }
        /* Long-press = scene cycle. Only fires from the normal browse mode,
         * not while inside brightness_mode (long press there would be a
         * confusing "did my toggle work?" moment) and not from inside a
         * test scene (the same hold should not skip MENU back to RAMP
         * immediately). The user can always exit a scene by short-tapping
         * SHIFT first (which toggles brightness on, then off, harmless),
         * but in practice scenes are exited by another long-press: the
         * cycle ends back at MENU. */
        if (sh_held && !shift_longed && !shift_rotated && !brightness_mode
            && now - shift_t0 >= LONG_PRESS_MS) {
            shift_longed = true;
            scene = (scene_t)((scene + 1) % SCENE_COUNT);
            if (scene != SCENE_MENU) {
                /* leaving the menu: tear the menu's transient overlay down  */
                dismiss_overlay(now);
            }
            printf("scene -> %s\n", SCENE_NAME[scene]);
            dirty = true;
        }

        /* --- encoder rotation -------------------------------------------
         * While animating, one loop iteration blocks ~36 ms in oled_show(),
         * so fast spinning can accumulate several detents per drain. The
         * magnitude must NOT be dropped: browse/discrete steps once per
         * detent, accelerated %-edits multiply by the detent count. */
        if (detents != 0) {
            int dir = detents > 0 ? 1 : -1;
            int n   = detents > 0 ? detents : -detents;
            if (sh_held) shift_rotated = true;   /* veto the on-release tap  */

            if (brightness_mode) {
                /* Brightness mode is independent of scene — adjusting the
                 * panel while judging RAMP greys is exactly the point. */
                backlight_pct += accel_ticks(dir, now) * n;
                if (backlight_pct < 0)   backlight_pct = 0;
                if (backlight_pct > 100) backlight_pct = 100;
                backlight_apply();
                if (scene == SCENE_MENU)
                    show_overlay("Backlight", backlight_pct, true, now);
                printf("backlight %d%%\n", backlight_pct);
                dirty = true;
            } else if (scene == SCENE_MENU) {
                menu_mode_t  m0 = menu_mode();
                menu_param_t c0 = menu_current();
                bool cont_edit = (m0 == MENU_EDIT) && cur_is_continuous();
                if (cont_edit) {
                    menu_rotate(accel_ticks(dir, now) * n);
                } else {
                    for (int k = 0; k < n; ++k) menu_rotate(dir);
                }
                if (menu_current() != c0)      on_cur_change(dir, now);
                else if (m0 == MENU_EDIT)      on_value_change(now);
                printf("rotate %+d -> %s = %s\n", detents,
                       menu_current_label(), menu_current_value_text());
                dirty = true;
            } else {
                printf("rotate (ignored in %s)\n", SCENE_NAME[scene]);
            }
        }

        /* --- encoder push ----------------------------------------------- */
        if (sw_press) {
            if (brightness_mode) {
                /* push inside brightness mode = jump to 100 % */
                backlight_pct = 100;
                backlight_apply();
                if (scene == SCENE_MENU)
                    show_overlay("Backlight", backlight_pct, true, now);
                printf("backlight -> 100%%\n");
                dirty = true;
            } else if (scene == SCENE_MENU) {
                menu_push();
                on_mode_change(now);
                printf("push -> mode=%s\n",
                       menu_mode() == MENU_EDIT ? "EDIT" : "BROWSE");
                dirty = true;
            } else {
                printf("push (ignored in %s)\n", SCENE_NAME[scene]);
            }
        }

        /* --- animate + render ------------------------------------------- */
        if (scene == SCENE_MENU) {
            bool animating = tweens_tick(now);
            anim_housekeeping(now);
            bool overlay_busy = overlay.active || anim[A_OVALPHA] > 0.01f;
            if (dirty || animating || overlay_busy || barseq.pending) {
                render_menu_scene();
                oled_show();                 /* frame pacing = SPI stream  */
                continue;                    /* keep animating, no sleep   */
            }
        } else if (dirty) {
            switch (scene) {
                case SCENE_RAMP:    render_ramp();    break;
                case SCENE_CHECKER: render_checker(); break;
                case SCENE_TYPE:    render_type();    break;
                default:            render_menu_scene(); break;
            }
            oled_show();
        }

        if (now - last_beat >= 5000) {
            last_beat = now;
            printf("alive — scene=%s bat=%d%% bl=%d%%\n",
                   SCENE_NAME[scene], battery_pct(), backlight_pct);
        }

        sleep_ms(2);                         /* idle poll                  */
    }
}
