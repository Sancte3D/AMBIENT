#include "ui_idle.h"

void ui_idle_init(ui_idle_t *s, uint32_t now, uint32_t initially_down) {
    *s = (ui_idle_t){.last_ms = now, .down = initially_down,
                     .suppressed = initially_down};
}

void ui_idle_begin_frame(ui_idle_t *s, uint32_t now) {
    if ((uint32_t)(now - s->wake_ms) >= UI_WAKE_GUARD_MS)
        s->wake_guard = false;
    if (s->down) s->last_ms = now;
    else if ((uint32_t)(now - s->last_ms) >= UI_IDLE_MS) s->quiet = true;
}

static bool activity(ui_idle_t *s, uint32_t now) {
    s->last_ms = now;
    if (s->quiet) {
        s->quiet = false;
        s->wake_guard = true;
        s->wake_ms = now;
    }
    return !s->wake_guard;
}

bool ui_idle_rotation(ui_idle_t *s, int delta, uint32_t now) {
    return delta != 0 && activity(s, now);
}

bool ui_idle_button(ui_idle_t *s, unsigned key, bool pressed, uint32_t now) {
    if (key >= 32u) return false;
    uint32_t bit = UINT32_C(1) << key;
    if (pressed) {
        if (s->down & bit) return false; /* duplicate press */
        s->down |= bit;
        if (!activity(s, now)) {
            s->suppressed |= bit;
            return false;
        }
        return true;
    }
    if (!(s->down & bit)) return false; /* no owned press */
    s->down &= ~bit;
    s->last_ms = now;
    bool deliver = !(s->suppressed & bit);
    s->suppressed &= ~bit;
    return deliver;
}
