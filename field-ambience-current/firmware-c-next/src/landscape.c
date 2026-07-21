/*
 * landscape.c — Landscape cell mode. Behaviour: see landscape.h.
 *
 * Pure control-rate role state machine; no audio, no allocation. The three
 * sustained roles (Drone/Bed/Atmos) share one momentary-or-latch model — the
 * same shape controls.c uses for Note mode, so a Landscape layer feels like a
 * held key. Motif and Memory are edge events with no held state.
 */

#include "landscape.h"

/* Cell → role. Kept explicit so the mapping is one obvious table, not spread
 * through the switch. */
enum { ROLE_DRONE = 0, ROLE_BED = 1, ROLE_MOTIF = 2, ROLE_ATMOS = 3,
       ROLE_MEMORY = 4 };

static landscape_iface_t s_if;
static bool s_latched[5];      /* sustained roles: latched on?        */
static bool s_moment[5];       /* sustained roles: momentary-on now?  */

/* Call the right sustained-layer setter for a role. */
static void layer_set(uint8_t cell, bool on) {
    switch (cell) {
        case ROLE_DRONE: if (s_if.drone) s_if.drone(on);       break;
        case ROLE_BED:   if (s_if.bed)   s_if.bed(on, cell);   break;
        case ROLE_ATMOS: if (s_if.atmos) s_if.atmos(on);       break;
        default: break;
    }
}

static bool is_sustained(uint8_t cell) {
    return cell == ROLE_DRONE || cell == ROLE_BED || cell == ROLE_ATMOS;
}

void landscape_init(const landscape_iface_t *iface) {
    if (iface) s_if = *iface;
    for (int i = 0; i < 5; ++i) { s_latched[i] = false; s_moment[i] = false; }
}

bool landscape_latched(uint8_t cell) {
    return cell < 5 && s_latched[cell];
}

void landscape_press(uint8_t cell, bool hold, uint32_t now_ms) {
    if (cell >= 5) return;

    if (cell == ROLE_MOTIF) {                 /* one fragile impulse */
        if (s_if.motif) s_if.motif(cell);
        return;
    }
    if (cell == ROLE_MEMORY) {                /* toggle the gesture loop */
        if (s_if.memory_toggle) s_if.memory_toggle(now_ms);
        return;
    }

    /* Sustained role. */
    if (hold) {
        /* Latch toggle. If a momentary press of this role is currently
         * sounding, fold it into the latch so state and sound stay in sync. */
        bool now_on = !s_latched[cell];
        s_latched[cell] = now_on;
        s_moment[cell]  = false;
        layer_set(cell, now_on);
    } else {
        s_moment[cell] = true;
        layer_set(cell, true);
    }
}

void landscape_release(uint8_t cell, uint32_t now_ms) {
    (void)now_ms;
    if (cell >= 5 || !is_sustained(cell)) return;
    if (!s_moment[cell]) return;              /* was a latch press — ignore up */
    s_moment[cell] = false;
    if (!s_latched[cell]) layer_set(cell, false);
}

void landscape_all_off(uint32_t now_ms) {
    (void)now_ms;
    for (uint8_t c = 0; c < 5; ++c) {
        if (is_sustained(c) && (s_latched[c] || s_moment[c]))
            layer_set(c, false);
        s_latched[c] = false;
        s_moment[c]  = false;
    }
}
