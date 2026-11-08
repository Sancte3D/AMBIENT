#ifndef FAM_CELL_ROUTER_H
#define FAM_CELL_ROUTER_H

#include <stdbool.h>
#include <stdint.h>

/* Control-thread only. Zero initialization means no owned presses.
 * Modes 0..2 correspond to Note/Harmony/Land; Generate never changes routing.
 * Remember the recipient of each down edge until its matching up edge.
 * Existing physical/gesture events still share the same five source IDs. */
typedef struct { uint8_t owner[5]; } cell_router_t;
typedef void (*cell_dispatch_t)(int mode, uint8_t cell, bool pressed, uint32_t now);

static inline void cell_router_release(cell_router_t *r, uint8_t cell,
                                       uint32_t now, cell_dispatch_t dispatch) {
    if (cell >= 5 || !r->owner[cell]) return;
    int mode = r->owner[cell] - 1;
    r->owner[cell] = 0; /* clear before callbacks, which can stop gesture playback */
    dispatch(mode, cell, false, now);
}

static inline void cell_router_event(cell_router_t *r, int mode, uint8_t cell,
                                     bool pressed, uint32_t now, cell_dispatch_t dispatch) {
    if (cell >= 5) return;
    if (!pressed) { cell_router_release(r, cell, now, dispatch); return; }
    if (mode < 0 || mode > 2) return;
    cell_router_release(r, cell, now, dispatch);
    r->owner[cell] = (uint8_t)(mode + 1);
    dispatch(mode, cell, true, now);
}

static inline void cell_router_release_all(cell_router_t *r, uint32_t now,
                                           cell_dispatch_t dispatch) {
    for (uint8_t cell = 0; cell < 5; ++cell)
        cell_router_release(r, cell, now, dispatch);
}
#endif
