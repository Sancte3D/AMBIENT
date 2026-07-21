/*
 * Host test for the r19.21 encoder push layer (knobs.c) + transient
 * overlay (overlay.c):
 *   - short vs long press classification (release before/after 500 ms)
 *   - DRIVE push = bypass toggle, rotate takes the value back
 *   - DRIVE long = reset to default (15 %)
 *   - BRIGHT short = neutral 0 Hz
 *   - VOLUME push = mute/unmute, rotate unmutes
 *   - VOLUME long = status overlay via callback
 *   - DISPLAY short fires the menu callback on release; a long press
 *     consumes the release (no menu action)
 *   - overlays appear on every action and expire after OVERLAY_HOLD_MS
 */
#include <stdio.h>
#include <string.h>
#include "knobs.h"
#include "overlay.h"
#include "params.h"
#include "engine.h"
#include "brain.h"
#include "dsp.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

static int s_menu_pushes = 0;
static void fake_menu_push(void) { ++s_menu_pushes; }
static void fake_status(char *l1, unsigned n1, char *l2, unsigned n2) {
    snprintf(l1, n1, "BAT 78%% +USB");
    snprintf(l2, n2, "SPEAKERS");
}

/* Press+release helper with a given hold duration. */
static void tap(uint8_t id, uint32_t *now, uint32_t hold_ms) {
    knobs_push(id, true, *now);
    *now += hold_ms;
    knobs_tick(*now);          /* long threshold fires here if crossed */
    knobs_push(id, false, *now);
}

int main(void) {
    printf("== knobs (encoder push layer, r19.21) + overlay ==\n");
    dsp_init(); brain_init(); engine_init(); params_init(); overlay_init();
    knobs_callbacks_t cb = { 0 };
    cb.display_push = fake_menu_push;
    cb.status_lines = fake_status;
    knobs_init(&cb);
    uint32_t now = 10000;

    /* ---- 1. defaults ---- */
    CHECK(!params_drive_bypassed(), "drive not bypassed at boot");
    CHECK(!params_muted(),          "not muted at boot");
    CHECK(!overlay_active(now),     "no overlay at boot");

    /* ---- 2. DRIVE short press = bypass toggle + overlay ---- */
    int d0 = params_drive_pct();
    tap(PARAM_ENC_DRIVE, &now, 80);                  /* short */
    CHECK(params_drive_bypassed(),  "DRIVE short press engages bypass");
    CHECK(params_drive_pct() == d0, "bypass keeps the stored value (%d)", params_drive_pct());
    CHECK(overlay_active(now),      "bypass shows an overlay");
    tap(PARAM_ENC_DRIVE, &now, 80);
    CHECK(!params_drive_bypassed(), "second short press restores A-value");

    /* ---- 3. rotate takes the value back out of bypass ---- */
    tap(PARAM_ENC_DRIVE, &now, 80);                  /* bypass ON again */
    CHECK(params_drive_bypassed(), "bypassed before the rotate");
    now += 400;
    knobs_rotate(PARAM_ENC_DRIVE, +1, now);
    CHECK(!params_drive_bypassed(), "rotating DRIVE exits bypass");
    CHECK(params_drive_pct() == d0 + 1, "rotate stepped from the stored value (%d)",
          params_drive_pct());

    /* ---- 4. DRIVE long press = reset to default, release consumed ---- */
    now += 1000;
    knobs_rotate(PARAM_ENC_DRIVE, +1, now);          /* move off default   */
    now += 1000;
    tap(PARAM_ENC_DRIVE, &now, KNOBS_LONG_MS + 100); /* long               */
    CHECK(params_drive_pct() == 15, "DRIVE long press resets to 15%% (got %d)",
          params_drive_pct());
    CHECK(!params_drive_bypassed(), "reset clears bypass");

    /* ---- 5. BRIGHT short = neutral ---- */
    now += 1000;
    knobs_rotate(PARAM_ENC_BRIGHT, +3, now);
    CHECK(params_bright_hz() > 0.0f, "brightness moved");
    tap(PARAM_ENC_BRIGHT, &now, 80);
    CHECK(params_bright_hz() == 0.0f, "BRIGHT short press = neutral 0 Hz");

    /* ---- 6. VOLUME mute/unmute + rotate unmutes ---- */
    now += 1000;
    int v0 = params_volume_pct();
    tap(PARAM_ENC_VOLUME, &now, 80);
    CHECK(params_muted(),             "VOLUME short press mutes");
    CHECK(params_volume_pct() == v0,  "mute keeps the stored volume");
    tap(PARAM_ENC_VOLUME, &now, 80);
    CHECK(!params_muted(),            "second press unmutes");
    tap(PARAM_ENC_VOLUME, &now, 80);  /* mute again */
    now += 400;
    knobs_rotate(PARAM_ENC_VOLUME, +1, now);
    CHECK(!params_muted(),            "rotating VOLUME unmutes");
    CHECK(params_volume_pct() == v0 + 1, "rotate stepped the volume (%d)",
          params_volume_pct());

    /* ---- 7. VOLUME long = status overlay via callback ---- */
    now += 1000;
    tap(PARAM_ENC_VOLUME, &now, KNOBS_LONG_MS + 50);
    CHECK(!params_muted(),        "long press did NOT toggle mute (consumed)");
    CHECK(overlay_active(now),    "status overlay shown");

    /* ---- 8. DISPLAY: short fires menu cb on release; long consumes ---- */
    now += 3000;
    int m0 = s_menu_pushes;
    tap(PARAM_ENC_DISPLAY, &now, 80);
    CHECK(s_menu_pushes == m0 + 1, "DISPLAY short press = menu push");
    tap(PARAM_ENC_DISPLAY, &now, KNOBS_LONG_MS + 100);
    CHECK(s_menu_pushes == m0 + 1, "DISPLAY long press does NOT menu-push (reserved)");

    /* ---- 9. overlay lifetime: active, then expires ---- */
    now += 5000;
    CHECK(!overlay_active(now), "overlays expired after idle");
    uint32_t g0 = overlay_gen();
    knobs_rotate(PARAM_ENC_DRIVE, +1, now);
    CHECK(overlay_gen() == g0 + 1, "rotate bumped the overlay generation");
    CHECK(overlay_active(now),                        "overlay active right after");
    CHECK(overlay_active(now + OVERLAY_HOLD_MS - 50), "still active before hold expires");
    CHECK(!overlay_active(now + OVERLAY_HOLD_MS + 50),"expired after hold");

    /* ---- 10. overlay renders without crashing (framebuffer smoke) ---- */
    knobs_rotate(PARAM_ENC_VOLUME, +1, now);
    overlay_render();
    CHECK(1, "overlay_render smoke");

    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
