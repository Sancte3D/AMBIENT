#ifndef FAM_UI_IDLE_H
#define FAM_UI_IDLE_H

#include <stdbool.h>
#include <stdint.h>

/* Main-owned human input gate. No engine/audio/parameter dependency.
 * A quiet screen consumes the waking gesture, including its release and
 * simultaneous inputs, including30ms of queued/debounced edges. Already forwarded releases survive.
 * Holding a physical button prevents idle. Audio/generative/LED/battery work
 * never counts as human activity. Unsigned elapsed time handles tick wrap. */
#define UI_IDLE_MS 900000u
#define UI_WAKE_GUARD_MS 30u
typedef struct {
    uint32_t last_ms;
    uint32_t wake_ms;
    uint32_t down;
    uint32_t suppressed;
    bool quiet;
    bool wake_guard;
} ui_idle_t;

void ui_idle_init(ui_idle_t *s, uint32_t now, uint32_t initially_down);
void ui_idle_begin_frame(ui_idle_t *s, uint32_t now);
bool ui_idle_button(ui_idle_t *s, unsigned key, bool pressed, uint32_t now);
bool ui_idle_rotation(ui_idle_t *s, int delta, uint32_t now);

#endif
