/*
 * Cell-Velocity input model (ADR-0013) — see cells.h.
 *
 * Per-cell state machine, fed one normalised position sample at a time:
 *
 *   REST  ──(pos ≥ BAND_LO)──▶  ARMED          (start the band timer)
 *   ARMED ──(pos < BAND_LO)──▶  REST           (aborted press, no event)
 *   ARMED ──(pos ≥ BAND_HI)──▶  HELD + PRESS   (velocity = band / elapsed)
 *   HELD  ──(pos ≤ RELEASE)──▶  REST + RELEASE
 *
 * A single very fast sample can carry a cell straight from REST past BAND_HI;
 * the arming and trigger checks below cascade within one update() so that case
 * still fires a PRESS (at maximum velocity, since elapsed collapses to FAST).
 */

#include "cells.h"
#include <math.h>

typedef enum { ST_REST = 0, ST_ARMED, ST_HELD } cell_state_t;

typedef struct {
    cell_state_t state;
    uint32_t     t_band_lo;   /* time the stem crossed BAND_LO going down */
    float        pos;         /* last sampled position */
} cell_runtime_t;

static cell_runtime_t cells[CELL_COUNT];

void cells_init(void) {
    for (int i = 0; i < CELL_COUNT; ++i) {
        cells[i].state     = ST_REST;
        cells[i].t_band_lo = 0;
        cells[i].pos       = 0.0f;
    }
}

float cells_velocity_to_amp(float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    return CELL_AMP_MIN + (CELL_AMP_MAX - CELL_AMP_MIN) * powf(v, CELL_AMP_GAMMA);
}

/* Elapsed band-crossing time → normalised velocity (0..1). Shorter = faster. */
static float time_to_velocity(float elapsed_ms) {
    if (elapsed_ms <= CELL_T_FAST_MS) return 1.0f;
    if (elapsed_ms >= CELL_T_SLOW_MS) return 0.0f;
    return (CELL_T_SLOW_MS - elapsed_ms) / (CELL_T_SLOW_MS - CELL_T_FAST_MS);
}

cell_event_t cells_update(uint8_t cell, float pos, uint32_t now_ms) {
    cell_event_t ev = { CELL_EVENT_NONE, cell, 0.0f, 0.0f };
    if (cell >= CELL_COUNT) return ev;

    cell_runtime_t *c = &cells[cell];
    if (pos < 0.0f) pos = 0.0f;
    if (pos > 1.0f) pos = 1.0f;
    c->pos = pos;

    /* REST → ARMED on entering the band (records the start of the timer). */
    if (c->state == ST_REST) {
        if (pos >= CELL_VEL_BAND_LO) {
            c->state     = ST_ARMED;
            c->t_band_lo = now_ms;
        } else {
            return ev;
        }
    }

    /* ARMED: either reach the trigger (fire PRESS) or retreat (abort). */
    if (c->state == ST_ARMED) {
        if (pos >= CELL_VEL_BAND_HI) {
            float elapsed = (float)(now_ms - c->t_band_lo);
            if (elapsed < 0.0f) elapsed = 0.0f;
            float v   = time_to_velocity(elapsed);
            c->state  = ST_HELD;
            ev.kind     = CELL_EVENT_PRESS;
            ev.cell     = cell;
            ev.velocity = v;
            ev.amp      = cells_velocity_to_amp(v);
            return ev;
        }
        if (pos < CELL_VEL_BAND_LO) {
            c->state = ST_REST;
        }
        return ev;
    }

    /* HELD → REST on retreat past the release threshold (hysteresis). */
    if (c->state == ST_HELD) {
        if (pos <= CELL_POS_RELEASE) {
            c->state = ST_REST;
            ev.kind  = CELL_EVENT_RELEASE;
            ev.cell  = cell;
        }
    }
    return ev;
}

float cells_position(uint8_t cell) {
    return (cell < CELL_COUNT) ? cells[cell].pos : 0.0f;
}

bool cells_is_held(uint8_t cell) {
    return (cell < CELL_COUNT) && cells[cell].state == ST_HELD;
}
