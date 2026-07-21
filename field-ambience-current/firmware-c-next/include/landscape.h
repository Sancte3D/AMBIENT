#ifndef FAM_LANDSCAPE_H
#define FAM_LANDSCAPE_H

/*
 * landscape.c — the "Landscape" cell mode (r19.27, cell-role architecture).
 *
 * A THIRD cell mode next to Note (r19.26 pentatonic) and Bloom (r19.23) — it
 * does not replace them. Where Note maps the five cells to five pitches,
 * Landscape maps them to the four sound LAYERS the engine already owns, plus
 * the memory loop — the real-gear model (Sonicware Ambient Ø layer roles,
 * SOMA Cosmos evolving memory, Torso S-4 few-but-effective controls),
 * scaled to our hardware (one shared reverb, ~6 voices, no sequencers):
 *
 *   cell 0  DRONE   — low stable pedal tone            (sustained, latchable)
 *   cell 1  BED     — broad slow pad swell             (sustained, latchable)
 *   cell 2  MOTIF   — one fragile pluck/glass impulse  (one-shot on press)
 *   cell 3  ATMOS   — air / noise / weather layer up   (sustained, latchable)
 *   cell 4  MEMORY  — record / replay the played gesture (toggle on press)
 *
 * Interaction (reusing the existing modifier): a plain tap makes the sustained
 * layers momentary (on while held, off on release); with HOLD the press
 * toggle-LATCHES the layer (stays until pressed again or CLEAR). Motif is
 * always a single impulse; Memory always toggles the gesture loop.
 *
 * Hardware-independent: every layer action goes through an injected interface
 * so this is host-testable and the engine keeps no new link dependency. The
 * pitches themselves still come from brain.c's clean pentatonic (r19.26) —
 * the caller supplies them; this module only owns the ROLE state machine.
 */

#include <stdint.h>
#include <stdbool.h>

/* Layer actions the host wires to real engine calls. `on` layers get a
 * boolean; motif/memory are edge events. `cell` is passed through so the
 * caller can pick the pentatonic pitch for the bed/motif. */
typedef struct {
    void (*drone)(bool on);
    void (*bed)(bool on, uint8_t cell);
    void (*motif)(uint8_t cell);
    void (*atmos)(bool on);
    void (*memory_toggle)(uint32_t now_ms);
} landscape_iface_t;

void landscape_init(const landscape_iface_t *iface);

/* Physical (or replayed) cell press/release. `hold` = the HOLD modifier state
 * at press time → latch instead of momentary (sustained roles only). */
void landscape_press  (uint8_t cell, bool hold, uint32_t now_ms);
void landscape_release(uint8_t cell, uint32_t now_ms);

/* CLEAR / mode-exit: turn every sustained layer off and drop all latches. */
void landscape_all_off(uint32_t now_ms);

/* Observability for UI/tests: is a sustained role currently latched on? */
bool landscape_latched(uint8_t cell);

#endif
