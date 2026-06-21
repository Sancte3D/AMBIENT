#ifndef FAM_CONTROLS_H
#define FAM_CONTROLS_H

/*
 * controls.c — modifier + cell hold-latch state machine.
 *
 * The Cell-LED Independent-Latches behaviour from ADR-0008 r2, implemented
 * as a hardware-independent C module so the host tests cover it and the
 * STM32 + Pico builds share one source of truth.
 *
 * State per cell:
 *   hold_base[c]   — cell c sustains its base-octave root
 *   hold_shift[c]  — cell c sustains its shifted (+12) root
 * Both bits are independent: both lit at once = octave-stack (the rich
 * ambient drone effect from ADR-0008 r2).
 *
 * Modifier states (toggle-on-press, like real synth panels):
 *   shift      — picks WHICH branch a Hold-Tap toggles
 *   hold       — without this, cell taps are momentary (release on tap-up)
 *   drone      — toggles engine_set_drone(); root drone follows brain key
 *   generate   — toggles engine_set_generative(true,…); advance bar timer
 *   clear      — momentary: clears all hold bits and any sustained voices
 *
 * Tap semantics (Hold latched + cell c tapped):
 *   shift OFF → toggle hold_base[c]:  if turning ON  → engine_note_on(c, root)
 *                                     if turning OFF → engine_note_off(c)
 *   shift ON  → toggle hold_shift[c]: if turning ON  → engine_note_on(c+9, root+12)
 *                                     if turning OFF → engine_note_off(c+9)
 *
 * Source-ID scheme (matches render_performance.c):
 *   base cell c   → source c       (0..4)
 *   shift cell c  → source c + 9   (9..13)
 *
 * Without Hold latched, a cell tap is a momentary note: down-edge → note_on,
 * up-edge → note_off. The Hall-velocity model in cells.c determines amp.
 */

#include <stdint.h>
#include <stdbool.h>

#define CTRL_CELL_COUNT 5

typedef enum {
    MOD_SHIFT = 0,
    MOD_HOLD,
    MOD_DRONE,
    MOD_GENERATE,
    MOD_CLEAR,
    MOD_COUNT,
} mod_id_t;

/* Reset all modifier + hold-latch state. Call once at boot. */
void controls_init(void);

/* Modifier press edge (pressed=true) / release edge (pressed=false).
 * SHIFT and HOLD latch on press (next press toggles off). CLEAR is
 * momentary: a press clears everything and re-arms. DRONE and GENERATE
 * latch and forward to engine. */
void controls_modifier(mod_id_t mod, bool pressed);

/* Cell tap (momentary press edge with velocity amp 0..1, ADR-0013). Velocity
 * comes from cells.c. If Hold is latched the bit toggles; otherwise it's a
 * momentary note. */
void controls_cell_press(uint8_t cell, float velocity_amp);

/* Cell release (momentary up-edge). Only meaningful when Hold is NOT latched
 * — releases the momentary note. Held cells are unaffected. */
void controls_cell_release(uint8_t cell);

/* Observability — used by LED renderer + UI. */
bool controls_hold_base (uint8_t cell);
bool controls_hold_shift(uint8_t cell);
bool controls_modifier_active(mod_id_t mod);

#endif
