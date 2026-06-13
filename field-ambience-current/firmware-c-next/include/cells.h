#ifndef FAM_CELLS_H
#define FAM_CELLS_H

/*
 * Cell-Velocity input model (ADR-0013).
 *
 * The five cells are low-profile MAGNETIC switches (Gateron LP Magnetic
 * class). Under each one sits a linear Hall sensor (DRV5056A4, SOT-23) on an
 * STM32 ADC channel (PC0/PC1/PA4/PB0/PB1 — SPEC §5.6a). As the magnet stem
 * approaches the PCB the sensor voltage rises; the firmware normalises that to
 * a position `pos` in [0,1] (0 = rest / fully up, 1 = bottom-out).
 *
 * This module turns that continuous position stream into musical note events
 * with VELOCITY, exactly the way a real Hall keybed does it: it times how fast
 * the stem crosses a fixed velocity band (BAND_LO → BAND_HI). A fast press
 * crosses the band in a few ms (loud); a slow press takes much longer (soft).
 * The note-on fires at BAND_HI (the trigger point); the note-off fires once
 * the stem retreats below RELEASE (hysteresis so it can't chatter).
 *
 * It is pure logic — no SDK, no float ADC driver — so it is host-testable and
 * identical on the RP2040 bench (fed from a synthetic ramp) and on the STM32
 * target (fed from the real ADC). The velocity curve constants below are the
 * single source of truth and are mirrored verbatim in tools/display_sim.html
 * and SPEC §5.6a.
 */

#include <stdint.h>
#include <stdbool.h>

#define CELL_COUNT 5

/* ---- Velocity model tunables (mirror in display_sim.html + SPEC §5.6a) ---- */
/* Normalised stem positions (0 = rest, 1 = bottom-out). */
#define CELL_VEL_BAND_LO   0.15f   /* start timing the press here            */
#define CELL_VEL_BAND_HI   0.55f   /* trigger point: note-on fires here      */
#define CELL_POS_RELEASE   0.30f   /* note-off once it retreats below this   */
/* Time to cross the band (BAND_HI−BAND_LO) mapped to velocity 1.0 … 0.0.    */
#define CELL_T_FAST_MS     6.0f    /* ≤ this across the band → velocity 1.0   */
#define CELL_T_SLOW_MS     70.0f   /* ≥ this across the band → velocity 0.0   */
/* Velocity (0..1) → per-voice peak amplitude. Median press ≈ the old fixed
 * 0.12 the binary cells used, but now with real dynamic range, and small
 * enough to keep polyphony headroom under the soft-clipped master. */
#define CELL_AMP_MIN       0.05f
#define CELL_AMP_MAX        0.22f
#define CELL_AMP_GAMMA     0.8f    /* <1 expands soft end for fine control    */

typedef enum {
    CELL_EVENT_NONE = 0,
    CELL_EVENT_PRESS,     /* velocity + amp valid */
    CELL_EVENT_RELEASE,
} cell_event_kind_t;

typedef struct {
    cell_event_kind_t kind;
    uint8_t cell;         /* 0..CELL_COUNT-1 */
    float   velocity;     /* 0..1 normalised (PRESS only) */
    float   amp;          /* 0..1 mapped peak amplitude (PRESS only) */
} cell_event_t;

/* Reset all cell state. Call once at boot. */
void cells_init(void);

/* Feed one normalised position sample for `cell` at `now_ms`. Returns the
 * event produced by this sample (NONE / PRESS / RELEASE). Sampling rate is
 * free; 1 kHz on hardware (SPEC §5.6a). Robust to coarse sampling: a press
 * that jumps the whole band in one sample is treated as maximum velocity. */
cell_event_t cells_update(uint8_t cell, float pos_0_1, uint32_t now_ms);

/* Last sampled position (0..1) — for aftertouch / debug / the sim. */
float cells_position(uint8_t cell);

/* True while the cell's note is sounding (between PRESS and RELEASE). */
bool cells_is_held(uint8_t cell);

/* Map a normalised velocity (0..1) to a peak amplitude (0..1) with the curve
 * above. Exposed so the offline harness / sim can share the exact mapping. */
float cells_velocity_to_amp(float velocity_0_1);

#endif
