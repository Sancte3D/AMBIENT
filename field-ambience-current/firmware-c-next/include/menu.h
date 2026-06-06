#ifndef FAM_MENU_H
#define FAM_MENU_H

/*
 * Menu / display state — Step 12b #7.
 *
 * UI is the mockup the user designed: parameter label top-left, big value
 * centred, battery top-right, a row of pills at the bottom — one pill per
 * parameter, the current one elongated + full white, the others dimmed. Pure
 * monochrome white on black, using the SSD1322's 4-bit grey to fade inactive
 * elements. Host-renderable to a preview PNG.
 *
 * Navigation (Display encoder):
 *   - rotate in browse mode → next/previous parameter (pill moves)
 *   - push                  → enter edit mode (value emphasised)
 *   - rotate in edit mode   → change the current parameter's value
 *   - push                  → back to browse
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
    MP_KEY = 0,
    MP_MODE,
    MP_VIBE,
    MP_VOICE,
    MP_DRONE,
    MP_GENERATIVE,
    MP_TEXTURE,
    MP_BASS,
    MP_SPACE,
    MP_MOOD,
    MP_VOLUME,
    MP_COUNT
} menu_param_t;

/* Reset to defaults + register the engine apply callback (so a value change
 * actually drives the audio). The callback is invoked synchronously from
 * menu_rotate / menu_push, never from the audio thread. */
typedef struct {
    void (*set_key)        (int midi_root);          /* 60 = C4 */
    void (*set_mode)       (int idx);                /* 0..5 */
    void (*set_vibe)       (int idx);                /* 0..3 */
    void (*set_pad_voice)  (int idx);                /* 0..2 */
    void (*set_drone)      (bool on);
    void (*set_generative) (bool on, int program);   /* program <0 = Markov */
    void (*set_texture)    (float v01);
    void (*set_bass_depth) (float v01);
    void (*set_space)      (float v01);
    void (*set_mood)       (float v01);
    void (*set_master)     (float v01);
} menu_callbacks_t;

void menu_init(const menu_callbacks_t *cb);

/* Encoder events. delta is typically ±1; multiples are honoured for fast
 * scrolling. push toggles MENU_BROWSE ⇄ MENU_EDIT. */
void menu_rotate(int delta);
void menu_push(void);

/* Read-only state for the renderer and the host tests. */
menu_param_t menu_current(void);
menu_mode_t  menu_mode(void);
const char  *menu_current_label(void);
const char  *menu_current_value_text(void);  /* formatted for the big display */
int          menu_value_index(menu_param_t p);   /* discrete params */
int          menu_value_int  (menu_param_t p);   /* continuous → 0..100 % */

/* Draw the current state into the OLED framebuffer (does NOT push to panel —
 * caller invokes oled_show() afterwards on device). Host-portable. */
void menu_render(void);

/* Low-level: draw just the bottom position bar for `total` entries with
 * `active` highlighted. menu_render uses it with the live entry count; future
 * paged menus (different entry counts per page) and the preview tool call it
 * directly. Adapts to any total: fits all when possible, else scroll-windows
 * with edge fade + chevrons. */
void menu_render_bar_only(int total, int active);

#endif
