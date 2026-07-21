#ifndef FAM_MENU_H
#define FAM_MENU_H

/*
 * Menu / display state — WORLD model (r18.38).
 *
 * Reworked from the old V1 parameter list (Key/Mode/Vibe/Voice/Texture/Bass/
 * Space/Mood) to the world-instrument model the project settled on: the user
 * picks one of a few curated WORLDS (Tokyo City / Crystal Coast / Midnight
 * Drive / After Hours) and tunes a handful of global macros over the top.
 * Each world also carries default macro values, so selecting a world loads its
 * preset; the user can then nudge from there.
 *
 * UI is unchanged in spirit: parameter label top-left, big value centred,
 * battery top-right, a row of pills at the bottom — one pill per parameter,
 * the current one elongated + full white, the others dimmed. Pure monochrome
 * white on black on the 4-bit grey framebuffer (320×170 ST7789 LCD).
 *
 * Navigation (Display encoder):
 *   - rotate in browse mode → next/previous parameter (pill moves)
 *   - push                  → enter edit mode (value emphasised)
 *   - rotate in edit mode   → change the current parameter's value
 *   - push                  → back to browse
 *
 * Parameters in the bottom pill row, in order (r18.58 — Reddit-style
 * performance macros, drops the duplicate Brightness-encoder / Tone and
 * the under-spec adaptive Drums):
 *   WORLD      discrete, 4 worlds   (selecting one loads its macro preset)
 *   SPACE      % reverb size + decay + wet (one macro = many internal params)
 *   ATMOS      % ambience layer level (rain / waves / wind / vinyl per world)
 *   MOTION     % pad LFO depth (filter sweep movement — the "alive" feel)
 *   AGE        % tape hiss + saturation drive (the "vinyl/cassette" feel)
 *
 * DRIVE + BRIGHTNESS sit on dedicated encoders (params.c) — not menu entries
 * so they never duplicate-fight the menu.
 *
 * DRONE / HOLD / SHIFT / GENERATE / CLEAR are MODIFIER BUTTONS (hardware), not
 * menu entries — so software and hardware never compete for the same value.
 * Display backlight is SHIFT+encoder (transient overlay), also not a menu row.
 *
 * State applies via engine_set_*() callbacks that the engine layer registers.
 */

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MENU_BROWSE = 0,
    MENU_EDIT,
} menu_mode_t;

/* Parameter slots, in the order shown in the bottom pill row. */
typedef enum {
    MP_WORLD = 0,
    MP_KEY,        /* r18.98: tonic pitch class 0..11 (world default on world change) */
    MP_TUNING,     /* r19.6: 0 Equal / 1 Just (user-global) */
    MP_VOICE,      /* r18.98: melody voice 0 PAD / 1 STRING / 2 GLASS (user-global) */
    MP_SPACE,
    MP_SHIMMER,    /* r18.99: octave-up hall regeneration (the halo)   */
    MP_ATMOS,
    MP_MOTION,
    MP_AGE,
    MP_ECHO,
    MP_BLUR,
    MP_SYNTH,     /* r19.16: sound-core 0 Ambient / 1..6 V2 synth (user-global) */
    MP_CELL,      /* r19.23/r19.27: cell mode 0 Note / 1 Bloom / 2 Land (user-global) */
    MP_BASS,      /* r19.31: HARMONY bass 0 Off / 1 Root / 2 Fifth / 3 Drift */
    MP_COLOR,     /* r19.32: HARMONY chord color 0 Pure / 1 Open / 2 Warm / 3 Deep */
    MP_FX,        /* r19.41: master-effects page 0 Bypass .. 8 Dream (default 8) */
    MP_COUNT
} menu_param_t;

/* Number of curated worlds. Single source of truth is WORLD_COUNT in
 * worlds.h (ADR-0017); MENU_WORLD_COUNT is kept as an alias for backward
 * compat with existing tests / tools / the JS sim port. */
#include "worlds.h"
#define MENU_WORLD_COUNT WORLD_COUNT

/* Reset to defaults + register the engine apply callback (so a value change
 * actually drives the audio). The callback is invoked synchronously from
 * menu_rotate / menu_push, never from the audio thread. Pass NULL for a
 * display-only build (the Pico bench tool, the preview renderer). */
typedef struct {
    void (*set_world)      (int idx);                /* 0..MENU_WORLD_COUNT-1 */
    void (*set_key)        (int pc);                 /* tonic pitch class 0..11 */
    void (*set_tuning)     (int just);               /* 0 Equal / 1 Just (r19.6) */
    void (*set_voice)      (int idx);                /* 0 PAD / 1 STRING / 2 GLASS */
    void (*set_space)      (float v01);              /* reverb / hall amount  */
    void (*set_shimmer)    (float v01);              /* octave-up halo (r18.99) */
    void (*set_atmosphere) (float v01);              /* ambience layer level  */
    void (*set_motion)     (float v01);              /* pad LFO depth         */
    void (*set_age)        (float v01);              /* tape hiss + sat       */
    void (*set_echo)       (float v01);              /* tape-style delay      */
    void (*set_blur)       (float v01);              /* granular cloud        */
    void (*set_synth)      (int idx);                /* r19.16: 0 Ambient / 1..6 V2 core */
    void (*set_cell)       (int mode);               /* r19.23/r19.27: 0 Note / 1 Bloom / 2 Land */
    void (*set_bass)       (int mode);               /* r19.31: 0 Off / 1 Root / 2 Fifth / 3 Drift */
    void (*set_color)      (int color);              /* r19.32: 0 Pure / 1 Open / 2 Warm / 3 Deep */
    void (*set_fx)         (int mode);               /* r19.41: 0 Bypass .. 8 Dream Chain */
} menu_callbacks_t;

void menu_init(const menu_callbacks_t *cb);

/* Encoder events. delta is typically ±1; multiples are honoured for fast
 * scrolling. push toggles MENU_BROWSE ⇄ MENU_EDIT. */
void menu_rotate(int delta);
void menu_push(void);

/* Read-only state for the renderer and the host tests. */
/* --- r19.22 Parameter-Locks (Orchid-Prinzip) ---------------------------
 * Gesperrte Parameter ueberlebt der World-Wechsel: load_world_preset()
 * setzt nur UNgesperrte Werte auf das neue World-Preset. Sperrbar sind
 * genau die 8 Werte, die ein World-Wechsel ueberschreibt (Key + die 7
 * Makros) — alles andere (Voice/Tuning/Synth) wird ohnehin nie angefasst.
 * Toggle: DISPLAY lang druecken IM EDIT-MODUS (knobs.c routet). */
bool menu_toggle_lock_current(void);     /* Rueckgabe: jetzt gesperrt?     */
bool menu_param_locked(menu_param_t p);
uint16_t menu_locks(void);               /* Bitmask (Bit = menu_param_t)   */
void menu_set_locks(uint16_t mask);      /* Scenes-Recall stellt sie wieder her */

/* --- r19.22 Scenes: kompletter einstellbarer Zustand -------------------
 * Snapshot dessen, was eine Scene speichert (KEINE gehaltenen Noten,
 * KEIN Volume — Lautstaerke springt beim Recall nie). */
typedef struct {
    uint8_t  world, key_pc, tuning, voice, synth, cell, bass, color, fx;
    uint8_t  space, shimmer, atmos, motion, age, echo, blur;   /* % */
    uint16_t locks;
} menu_state_t;
void menu_get_state(menu_state_t *out);
/* Werte setzen + ALLE Engine-Callbacks feuern (Recall = live). */
void menu_apply_state(const menu_state_t *st);

menu_param_t menu_current(void);
menu_mode_t  menu_mode(void);
const char  *menu_current_label(void);
const char  *menu_current_value_text(void);  /* formatted for the big display */
int          menu_value_index(menu_param_t p);   /* discrete params */
int          menu_value_int  (menu_param_t p);   /* continuous → 0..100 % */
int          menu_value_count(menu_param_t p);   /* # discrete options, 0 = % */

/* Current world index + its flavour subtitle (e.g. "night · rain"), for a
 * renderer that wants to show the world identity beyond the big value. */
int          menu_world_index(void);
const char  *menu_world_subtitle(void);

/* Draw the current state into the OLED framebuffer (does NOT push to panel —
 * caller invokes oled_show() afterwards on device). Host-portable. */
void menu_render(void);

/* Low-level: draw just the bottom option bar with `total` pills, `active`
 * highlighted, edge-to-edge. menu_render derives total/active from the current
 * setting's value space; the preview tool calls it directly to show how the
 * pill row scales for different option counts. */
void menu_render_bar_only(int total, int active);

#endif
