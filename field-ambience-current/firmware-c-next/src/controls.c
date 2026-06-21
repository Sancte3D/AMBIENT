/*
 * controls.c — modifier + cell hold-latch state machine (ADR-0008 r2).
 * See controls.h for the behavioural contract.
 *
 * Hardware-independent: calls the engine_* / brain_* API only, so the host
 * test (test/test_controls.c) exercises the full latch logic, and the STM32
 * button/encoder handlers (src/hal_h743/main_h743.c) just feed edges in.
 */

#include "controls.h"
#include "engine.h"
#include "brain.h"
#include "dsp.h"

#define SHIFT_SRC(c) ((uint8_t)((c) + 9))   /* shift-octave pad source */

static bool s_mod[MOD_COUNT];
static bool s_hold_base [CTRL_CELL_COUNT];
static bool s_hold_shift[CTRL_CELL_COUNT];

/* Momentary (non-hold) note tracking: which source is sounding for a cell
 * that was tapped without Hold latched. -1 = none. */
static int8_t s_moment_src[CTRL_CELL_COUNT];

void controls_init(void) {
    for (int i = 0; i < MOD_COUNT; ++i) s_mod[i] = false;
    for (int c = 0; c < CTRL_CELL_COUNT; ++c) {
        s_hold_base[c]  = false;
        s_hold_shift[c] = false;
        s_moment_src[c] = -1;
    }
}

static void clear_all_holds(void) {
    for (int c = 0; c < CTRL_CELL_COUNT; ++c) {
        if (s_hold_base[c])  { engine_note_off((uint8_t)c);          s_hold_base[c]  = false; }
        if (s_hold_shift[c]) { engine_note_off(SHIFT_SRC(c));        s_hold_shift[c] = false; }
        if (s_moment_src[c] >= 0) { engine_note_off((uint8_t)s_moment_src[c]); s_moment_src[c] = -1; }
    }
}

void controls_modifier(mod_id_t mod, bool pressed) {
    if (mod >= MOD_COUNT) return;
    switch (mod) {
        case MOD_SHIFT:
        case MOD_HOLD:
            if (pressed) s_mod[mod] = !s_mod[mod];   /* latch toggle on press */
            break;
        case MOD_DRONE:
            if (pressed) { s_mod[MOD_DRONE] = !s_mod[MOD_DRONE]; engine_set_drone(s_mod[MOD_DRONE]); }
            break;
        case MOD_GENERATE:
            if (pressed) { s_mod[MOD_GENERATE] = !s_mod[MOD_GENERATE];
                           engine_set_generative(s_mod[MOD_GENERATE], -1); }  /* -1 = Markov auto */
            break;
        case MOD_CLEAR:
            if (pressed) clear_all_holds();          /* momentary: clear held cells */
            break;
        default: break;
    }
}

void controls_cell_press(uint8_t cell, float velocity_amp) {
    if (cell >= CTRL_CELL_COUNT) return;
    int   root = brain_cell_root(cell);
    float hz   = dsp_midi_to_hz((float)root);
    float hz_s = dsp_midi_to_hz((float)(root + 12));

    if (s_mod[MOD_HOLD]) {
        /* Latching: toggle the branch the Shift state selects. The OTHER
         * branch is untouched (ADR-0008 r2 independent latches). */
        if (s_mod[MOD_SHIFT]) {
            s_hold_shift[cell] = !s_hold_shift[cell];
            if (s_hold_shift[cell]) engine_note_on(SHIFT_SRC(cell), hz_s, velocity_amp);
            else                    engine_note_off(SHIFT_SRC(cell));
        } else {
            s_hold_base[cell] = !s_hold_base[cell];
            if (s_hold_base[cell]) engine_note_on((uint8_t)cell, hz, velocity_amp);
            else                   engine_note_off((uint8_t)cell);
        }
    } else {
        /* Momentary: sounds until release. Shift selects which octave. */
        uint8_t src = s_mod[MOD_SHIFT] ? SHIFT_SRC(cell) : (uint8_t)cell;
        float   f   = s_mod[MOD_SHIFT] ? hz_s : hz;
        engine_note_on(src, f, velocity_amp);
        s_moment_src[cell] = (int8_t)src;
    }
}

void controls_cell_release(uint8_t cell) {
    if (cell >= CTRL_CELL_COUNT) return;
    /* Only momentary notes release on key-up; held latches persist. */
    if (s_moment_src[cell] >= 0) {
        engine_note_off((uint8_t)s_moment_src[cell]);
        s_moment_src[cell] = -1;
    }
}

bool controls_hold_base (uint8_t cell) { return cell < CTRL_CELL_COUNT && s_hold_base[cell]; }
bool controls_hold_shift(uint8_t cell) { return cell < CTRL_CELL_COUNT && s_hold_shift[cell]; }
bool controls_modifier_active(mod_id_t mod) { return mod < MOD_COUNT && s_mod[mod]; }
