#ifndef FAM_MENU_H
#define FAM_MENU_H

/*
 * Menu / display state — Step 12b #7.
 *
 * UI is the mockup the user designed: parameter label top-left, big value
 * centred, battery top-right, a row of pills at the bottom — one pill per
 * parameter, the current one elongated + full white, the others dimmed. Pure
 * monochrome white on black, using the 4-bit grey framebuffer to fade inactive
 * elements (r16: 320×170 ST7789 LCD). Host-renderable to a preview PNG.
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

/* Parameter slots, in the order shown in the bottom pill row.
 *
 * The menu only holds SETTABLE values the user picks from a range. Dedicated
 * hardware controls are deliberately NOT here so software and hardware never
 * compete for the same value:
 *   - VOLUME / DRIVE / BRIGHTNESS → own endless encoders (SPEC §5).
 *     "Brightness" is the AUDIO tone macro (pad filter cutoff, Sound
 *     Constitution /fam/brightness), NOT the display backlight.
 *   - DRONE / GENERATE / HOLD     → own modifier buttons (SPEC §7, GPB1..GPB3).
 *     SHIFT+GENERATE cycles the generative algorithm (SPEC §12.3).
 *   - Display BACKLIGHT            → SHIFT+EN2 (Brightness encoder, secondary
 *     function per SPEC §12.3), shown as a transient overlay like Volume/Drive.
 *     It is NOT a menu entry: it boots to a factory default (never restored to
 *     a dark value, see SPEC §12.5) so the user can never get locked out. */
typedef enum {
    MP_KEY = 0,
    MP_MODE,
    MP_VIBE,
    MP_VOICE,
    MP_TEXTURE,
    MP_BASS,
    MP_SPACE,
    MP_MOOD,
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
    void (*set_texture)    (float v01);
    void (*set_bass_depth) (float v01);
    void (*set_space)      (float v01);
    void (*set_mood)       (float v01);
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
int          menu_value_count(menu_param_t p);   /* # discrete options, 0 = % */

/* Draw the current state into the OLED framebuffer (does NOT push to panel —
 * caller invokes oled_show() afterwards on device). Host-portable. */
void menu_render(void);

/* Low-level: draw just the bottom option bar with `total` pills, `active`
 * highlighted, edge-to-edge. menu_render derives total/active from the current
 * setting's value space; the preview tool calls it directly to show how the
 * pill row scales for different option counts. */
void menu_render_bar_only(int total, int active);

#endif
