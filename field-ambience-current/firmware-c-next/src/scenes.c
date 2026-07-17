/*
 * scenes.c — 5 Scene-Slots + Scenes-UI. Siehe scenes.h.
 */

#include "scenes.h"
#include "menu.h"
#include "params.h"
#include "engine.h"
#include "oled.h"
#include "baked_font.h"
#include <string.h>
#include <stdio.h>

#define SCENE_MAGIC 0x53434E34u   /* "SCN4" — r19.32: menu_state_t + chord color */

typedef struct {
    uint32_t     magic;           /* SCENE_MAGIC = belegt                  */
    menu_state_t menu;
    int8_t       drive_pct;       /* 0..100                                */
    int16_t      bright_hz;       /* -600..800                             */
    uint32_t     gen_seed;
} scene_slot_t;

typedef struct {
    uint32_t     magic;           /* Store-Gueltigkeit                     */
    scene_slot_t slot[SCENES_COUNT];
} scene_store_t;

static scene_store_t   s_store;
static scenes_write_fn s_write;
static scenes_read_fn  s_read;
static int             s_active = -1;

static bool     s_ui_on = false;
static uint32_t s_ui_last;

void scenes_init(scenes_write_fn write_fn, scenes_read_fn read_fn) {
    s_write = write_fn;
    s_read  = read_fn;
    s_active = -1;
    s_ui_on  = false;
    memset(&s_store, 0, sizeof s_store);
    if (s_read && s_read(&s_store, sizeof s_store) &&
        s_store.magic == SCENE_MAGIC) {
        /* geladener Store ist gueltig — Slots behalten ihre Magic-Marken */
    } else {
        memset(&s_store, 0, sizeof s_store);
        s_store.magic = SCENE_MAGIC;
    }
}

bool scenes_used(int slot) {
    return slot >= 0 && slot < SCENES_COUNT &&
           s_store.slot[slot].magic == SCENE_MAGIC;
}

int scenes_active(void) { return s_active; }

bool scenes_save(int slot, uint32_t now_ms) {
    if (slot < 0 || slot >= SCENES_COUNT) return false;
    scene_slot_t *sl = &s_store.slot[slot];
    sl->magic = SCENE_MAGIC;
    menu_get_state(&sl->menu);
    sl->drive_pct = (int8_t)params_drive_pct();
    sl->bright_hz = (int16_t)params_bright_hz();
    sl->gen_seed  = engine_gen_seed();
    s_active  = slot;
    s_ui_last = now_ms;
    if (s_write) (void)s_write(&s_store, sizeof s_store);
    return true;
}

bool scenes_recall(int slot, uint32_t now_ms) {
    if (!scenes_used(slot)) return false;
    const scene_slot_t *sl = &s_store.slot[slot];
    menu_apply_state(&sl->menu);
    params_apply_scene(sl->drive_pct, (float)sl->bright_hz);
    engine_set_gen_seed(sl->gen_seed);
    s_active  = slot;
    s_ui_last = now_ms;
    return true;
}

/* --- Scenes-UI ----------------------------------------------------------- */

void scenes_ui_open(uint32_t now_ms) { s_ui_on = true;  s_ui_last = now_ms; }
void scenes_ui_close(void)           { s_ui_on = false; }
bool scenes_ui_active(void)          { return s_ui_on; }

void scenes_ui_tick(uint32_t now_ms) {
    if (s_ui_on && (uint32_t)(now_ms - s_ui_last) >= SCENES_UI_IDLE_MS)
        s_ui_on = false;
}

void scenes_ui_cell(uint8_t cell, bool shift, uint32_t now_ms) {
    if (!s_ui_on || cell >= SCENES_COUNT) return;
    if (shift) scenes_save((int)cell, now_ms);
    else       (void)scenes_recall((int)cell, now_ms);
    s_ui_last = now_ms;
}

/* Screen: "SCENES" oben, 5 Slot-Pillen unten (leer/belegt/aktiv) —
 * gleiche Typo/Idiome wie menu.c, aber ohne dessen Interna. */
#define SC_GS_BG     0
#define SC_GS_DIM    5
#define SC_GS_LABEL  9
#define SC_GS_VALUE  13
#define SC_GS_ACTIVE 15
#define SC_PAD_L     22
#define SC_PAD_T     14

void scenes_ui_render(void) {
    oled_fill(SC_GS_BG);
    bfont_draw(&font_hn_label, SC_PAD_L, SC_PAD_T, "SCENES", SC_GS_LABEL);

    /* Hinweistext: Cell = laden, SHIFT+Cell = speichern */
    bfont_draw(&font_hn_label, SC_PAD_L, SC_PAD_T + (int)font_hn_label.line + 4,
               "CELL = LOAD   SHIFT+CELL = SAVE", SC_GS_DIM);

    /* 5 Slot-Pillen, mittig — Hoehe/Idiom wie die Menue-Bar. */
    const int pill_w = 34, pill_h = 22, gap = 10;
    int total = SCENES_COUNT * pill_w + (SCENES_COUNT - 1) * gap;
    int x0 = (OLED_WIDTH - total) / 2;
    int y0 = (OLED_HEIGHT - pill_h) / 2 + 8;
    for (int i = 0; i < SCENES_COUNT; ++i) {
        int x = x0 + i * (pill_w + gap);
        bool used   = scenes_used(i);
        bool active = (scenes_active() == i) && used;
        if (active)      oled_rrect_fill(x, y0, pill_w, pill_h, 6, SC_GS_ACTIVE);
        else if (used)   oled_rrect_fill(x, y0, pill_w, pill_h, 6, SC_GS_DIM);
        else             oled_rrect_fill(x, y0 + pill_h/2 - 1, pill_w, 2, 1, SC_GS_DIM);
        char n[2] = { (char)('1' + i), 0 };
        int tw = bfont_width(&font_hn_label, n);
        bfont_draw(&font_hn_label, x + (pill_w - tw) / 2,
                   y0 + (pill_h - (int)font_hn_label.line) / 2,
                   n, active ? SC_GS_BG : SC_GS_VALUE);
    }
}
