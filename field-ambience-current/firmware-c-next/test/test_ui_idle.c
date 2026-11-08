#include "ui_idle.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    for (unsigned wrap = 0; wrap < 2; ++wrap) {
        uint32_t t = wrap ? UINT32_MAX - 100u : 0;
        ui_idle_t s;
        ui_idle_init(&s, t, 0);
        ui_idle_begin_frame(&s, t + UI_IDLE_MS - 1);
        assert(!s.quiet);
        ui_idle_begin_frame(&s, t + UI_IDLE_MS);
        assert(s.quiet);
        assert(!ui_idle_rotation(&s, 0, t + UI_IDLE_MS));
        assert(s.quiet);
        assert(!ui_idle_button(&s, 32, true, t + UI_IDLE_MS));
        assert(s.quiet);
        assert(!ui_idle_button(&s, 0, true, t + UI_IDLE_MS));
        assert(!s.quiet && s.wake_guard);
        assert(!ui_idle_button(&s, 8, true, t + UI_IDLE_MS));
        assert(!ui_idle_rotation(&s, 1, t + UI_IDLE_MS));
        ui_idle_begin_frame(&s, t + UI_IDLE_MS + 10);
        assert(!ui_idle_button(&s, 0, false, t + UI_IDLE_MS + 10));
        assert(!ui_idle_button(&s, 8, false, t + UI_IDLE_MS + 10));
        assert(!s.down && !s.suppressed);
        assert(!ui_idle_button(&s, 16, true, t + UI_IDLE_MS + 20));
        assert(!ui_idle_button(&s, 16, false, t + UI_IDLE_MS + 25));
        ui_idle_begin_frame(&s, t + UI_IDLE_MS + UI_WAKE_GUARD_MS);
        assert(ui_idle_button(&s, 19, true, t + UI_IDLE_MS + UI_WAKE_GUARD_MS));
        assert(!ui_idle_button(&s, 19, true, t + UI_IDLE_MS + 30));
        ui_idle_begin_frame(&s, t + 3 * UI_IDLE_MS);
        assert(!s.quiet); /* held encoder must still get its release */
        assert(ui_idle_button(&s, 19, false, t + 3 * UI_IDLE_MS));
        ui_idle_begin_frame(&s, t + 4 * UI_IDLE_MS - 1);
        assert(!s.quiet);
        ui_idle_begin_frame(&s, t + 4 * UI_IDLE_MS);
        assert(s.quiet);
        assert(!ui_idle_rotation(&s, -1, t + 4 * UI_IDLE_MS));
        ui_idle_begin_frame(&s, t + 4 * UI_IDLE_MS + UI_WAKE_GUARD_MS);
        assert(ui_idle_rotation(&s, -1, t + 4 * UI_IDLE_MS + UI_WAKE_GUARD_MS));
        ui_idle_init(&s, t, 1u << 3);
        ui_idle_begin_frame(&s, t + UI_IDLE_MS);
        assert(!s.quiet);
        assert(!ui_idle_button(&s, 3, false, t + UI_IDLE_MS));
    }
    puts("ui idle: exact threshold, held/released/duplicate keys, wake batch and wrap pass");
}
