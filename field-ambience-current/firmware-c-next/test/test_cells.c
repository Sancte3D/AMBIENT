/*
 * Host test for the cell-velocity input model (src/cells.c, ADR-0013).
 *
 * Drives the per-cell state machine with synthetic position ramps and asserts:
 *   - a slow press is soft, a fast press is loud, both inside [AMP_MIN,AMP_MAX]
 *   - velocity is monotonic in press speed
 *   - press → release sequencing with hysteresis (no chatter)
 *   - an aborted press (enters the band, retreats) produces no note
 *   - a re-press requires a release first
 *   - a single-sample slam still fires at max velocity
 *   - amp/velocity stay bounded for every input
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "cells.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

/* Press cell `c` by ramping position 0→1 so that the velocity band
 * (BAND_LO→BAND_HI) is crossed in `band_ms`. Returns the PRESS event seen
 * (kind=NONE if none fired). Advances `*t` to the end of the ramp. */
static cell_event_t press_ramp(uint8_t c, float band_ms, uint32_t *t) {
    cell_event_t got = { CELL_EVENT_NONE, c, 0, 0 };
    /* Position step per ms chosen so the band width is covered in band_ms. */
    const float band = CELL_VEL_BAND_HI - CELL_VEL_BAND_LO;
    const float dpos_per_ms = band / band_ms;
    float pos = 0.0f;
    for (int i = 0; i < 4000 && pos < 1.0f; ++i) {
        pos += dpos_per_ms;            /* 1 ms per sample */
        if (pos > 1.0f) pos = 1.0f;
        cell_event_t e = cells_update(c, pos, ++(*t));
        if (e.kind == CELL_EVENT_PRESS) { got = e; }
    }
    return got;
}

/* Release cell `c` by ramping position back to 0 over ~30 ms. */
static cell_event_t release_ramp(uint8_t c, uint32_t *t) {
    cell_event_t got = { CELL_EVENT_NONE, c, 0, 0 };
    float pos = cells_position(c);
    for (int i = 0; i < 200 && pos > 0.0f; ++i) {
        pos -= 0.05f;
        if (pos < 0.0f) pos = 0.0f;
        cell_event_t e = cells_update(c, pos, ++(*t));
        if (e.kind == CELL_EVENT_RELEASE) { got = e; }
    }
    return got;
}

int main(void) {
    printf("== cells (velocity model, ADR-0013) ==\n");
    cells_init();
    uint32_t t = 1000;

    /* ---- slow vs fast press ------------------------------------------- */
    cell_event_t slow = press_ramp(0, 60.0f, &t);
    CHECK(slow.kind == CELL_EVENT_PRESS, "slow press fired");
    CHECK(cells_is_held(0), "cell 0 held after press");
    cell_event_t rel0 = release_ramp(0, &t);
    CHECK(rel0.kind == CELL_EVENT_RELEASE, "slow press released");
    CHECK(!cells_is_held(0), "cell 0 not held after release");

    cell_event_t fast = press_ramp(0, 6.0f, &t);
    CHECK(fast.kind == CELL_EVENT_PRESS, "fast press fired");
    release_ramp(0, &t);

    CHECK(fast.velocity > slow.velocity + 0.2f,
          "fast louder than slow (v fast=%.3f slow=%.3f)", fast.velocity, slow.velocity);
    CHECK(fast.amp > slow.amp, "fast amp > slow amp (%.3f vs %.3f)", fast.amp, slow.amp);

    /* ---- bounds ------------------------------------------------------- */
    CHECK(slow.amp >= CELL_AMP_MIN - 1e-6f && fast.amp <= CELL_AMP_MAX + 1e-6f,
          "amps inside [MIN,MAX] (slow=%.3f fast=%.3f)", slow.amp, fast.amp);
    CHECK(slow.velocity >= 0.0f && fast.velocity <= 1.0f, "velocity bounded");

    /* ---- monotonic in speed ------------------------------------------ */
    float prev_amp = -1.0f;
    float speeds[] = { 60.0f, 30.0f, 15.0f, 8.0f, 4.0f };  /* faster each time */
    bool monotonic = true;
    for (unsigned i = 0; i < sizeof speeds / sizeof speeds[0]; ++i) {
        cell_event_t e = press_ramp(1, speeds[i], &t);
        release_ramp(1, &t);
        if (e.kind != CELL_EVENT_PRESS || e.amp < prev_amp - 1e-4f) monotonic = false;
        prev_amp = e.amp;
    }
    CHECK(monotonic, "amp monotonically increases with press speed");

    /* ---- aborted press: enter band, retreat, no note ------------------ */
    cells_init();
    t = 5000;
    cell_event_t none = { CELL_EVENT_NONE, 2, 0, 0 };
    /* up into the band but below trigger... */
    none = cells_update(2, 0.20f, ++t);     /* BAND_LO < 0.20 < BAND_HI */
    CHECK(none.kind == CELL_EVENT_NONE, "no event mid-band");
    none = cells_update(2, 0.40f, ++t);
    CHECK(none.kind == CELL_EVENT_NONE, "still no event below trigger");
    /* ...then retreat fully */
    none = cells_update(2, 0.05f, ++t);
    CHECK(none.kind == CELL_EVENT_NONE, "aborted press fires nothing");
    CHECK(!cells_is_held(2), "aborted press leaves cell idle");
    /* a real press afterwards must still work */
    cell_event_t after = press_ramp(2, 20.0f, &t);
    CHECK(after.kind == CELL_EVENT_PRESS, "press after abort works");
    release_ramp(2, &t);

    /* ---- no double-trigger without release --------------------------- */
    cells_init();
    t = 8000;
    cell_event_t p1 = press_ramp(3, 10.0f, &t);
    CHECK(p1.kind == CELL_EVENT_PRESS, "first press fires");
    /* hold near bottom-out: feed more high-position samples, expect silence */
    int extra = 0;
    for (int i = 0; i < 50; ++i)
        if (cells_update(3, 0.95f, ++t).kind != CELL_EVENT_NONE) ++extra;
    CHECK(extra == 0, "held cell does not re-trigger (%d spurious)", extra);
    cell_event_t r3 = release_ramp(3, &t);
    CHECK(r3.kind == CELL_EVENT_RELEASE, "release after hold");

    /* ---- single-sample slam = max velocity --------------------------- */
    cells_init();
    t = 9000;
    cell_event_t slam = cells_update(4, 1.0f, ++t);   /* REST → past BAND_HI in 1 step */
    CHECK(slam.kind == CELL_EVENT_PRESS, "single-sample slam fires");
    CHECK(slam.velocity > 0.99f, "slam is max velocity (%.3f)", slam.velocity);
    CHECK(fabsf(slam.amp - CELL_AMP_MAX) < 1e-4f, "slam amp = AMP_MAX (%.3f)", slam.amp);

    /* ---- mapping endpoints ------------------------------------------- */
    CHECK(fabsf(cells_velocity_to_amp(0.0f) - CELL_AMP_MIN) < 1e-6f, "v=0 → AMP_MIN");
    CHECK(fabsf(cells_velocity_to_amp(1.0f) - CELL_AMP_MAX) < 1e-6f, "v=1 → AMP_MAX");
    CHECK(cells_velocity_to_amp(2.0f) <= CELL_AMP_MAX + 1e-6f, "v clamps high");
    CHECK(cells_velocity_to_amp(-1.0f) >= CELL_AMP_MIN - 1e-6f, "v clamps low");

    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
