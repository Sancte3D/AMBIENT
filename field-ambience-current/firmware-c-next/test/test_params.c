/*
 * Host test for encoder→engine parameter bindings (params.c).
 *   - defaults match engine_init
 *   - slow detents move 1 %/step; clamp at 0 and 100
 *   - fast spin (small dt) accelerates (×up to 8)
 *   - brightness moves in Hz and clamps to its range
 *   - DISPLAY encoder is ignored (menu owns it)
 *   - direction sign is honoured
 */
#include <stdio.h>
#include <stdint.h>
#include "params.h"
#include "engine.h"
#include "dsp.h"
#include "brain.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

int main(void) {
    printf("== params (encoder → engine bindings) ==\n");
    dsp_init(); brain_init(); engine_init(); params_init();

    /* ---- defaults ---- */
    CHECK(params_drive_pct() == 15, "drive default 15%% (got %d)", params_drive_pct());
    /* r19.20: SPEC boot rule — 30 % max at power-on (headphone-safe since
     * the TPA6132A2). Was 60 % before the phones jack existed. */
    CHECK(params_volume_pct() == 30, "volume default 30%% (got %d)", params_volume_pct());
    /* r19.45: brightness boots to world 0 (Alps) = +550 Hz, not 0. */
    CHECK(params_bright_hz() == 550.0f, "bright default = Alps +550 Hz (got %.0f)", params_bright_hz());

    /* ---- slow detents: 1 %/step. Space events 300 ms apart (no accel). ---- */
    uint32_t t = 1000;
    int start = params_drive_pct();
    for (int i = 0; i < 10; ++i) { params_encoder(PARAM_ENC_DRIVE, +1, t); t += 300; }
    CHECK(params_drive_pct() == start + 10, "10 slow detents = +10%% (got %d)", params_drive_pct());

    /* ---- direction ---- */
    for (int i = 0; i < 5; ++i) { params_encoder(PARAM_ENC_DRIVE, -1, t); t += 300; }
    CHECK(params_drive_pct() == start + 5, "5 down = -5%% (got %d)", params_drive_pct());

    /* ---- clamp low ---- */
    for (int i = 0; i < 60; ++i) { params_encoder(PARAM_ENC_DRIVE, -1, t); t += 300; }
    CHECK(params_drive_pct() == 0, "drive clamps at 0 (got %d)", params_drive_pct());

    /* ---- clamp high ---- */
    for (int i = 0; i < 150; ++i) { params_encoder(PARAM_ENC_DRIVE, +1, t); t += 300; }
    CHECK(params_drive_pct() == 100, "drive clamps at 100 (got %d)", params_drive_pct());

    /* ---- acceleration: fast spin moves much more than slow ---- */
    params_init();                                   /* reset to 15 */
    /* 10 slow steps (300 ms apart) */
    t = 5000; int v0 = params_volume_pct();
    for (int i = 0; i < 10; ++i) { params_encoder(PARAM_ENC_VOLUME, +1, t); t += 300; }
    int slow_delta = params_volume_pct() - v0;       /* should be +10 */
    params_init();
    /* 10 fast steps (10 ms apart → top tier ×8) */
    t = 5000; v0 = params_volume_pct();
    for (int i = 0; i < 10; ++i) { params_encoder(PARAM_ENC_VOLUME, +1, t); t += 10; }
    int fast_delta = params_volume_pct() - v0;       /* clamps at 100, but must exceed slow */
    CHECK(fast_delta > slow_delta, "fast spin accelerates (fast=%d > slow=%d)", fast_delta, slow_delta);

    /* ---- brightness in Hz + clamp ---- */
    params_init();
    t = 9000;
    for (int i = 0; i < 5; ++i) { params_encoder(PARAM_ENC_BRIGHT, +1, t); t += 300; }
    CHECK(params_bright_hz() == 650.0f, "5 bright detents from +550 base = +650 Hz (got %.0f)", params_bright_hz());
    for (int i = 0; i < 200; ++i) { params_encoder(PARAM_ENC_BRIGHT, +1, t); t += 300; }
    CHECK(params_bright_hz() <= 800.0f && params_bright_hz() >= 799.0f,
          "bright clamps at +800 Hz (got %.0f)", params_bright_hz());
    for (int i = 0; i < 400; ++i) { params_encoder(PARAM_ENC_BRIGHT, -1, t); t += 300; }
    CHECK(params_bright_hz() <= -599.0f && params_bright_hz() >= -600.0f,
          "bright clamps at -600 Hz (got %.0f)", params_bright_hz());

    /* ---- DISPLAY encoder ignored ---- */
    params_init();
    int d0 = params_drive_pct(), vv0 = params_volume_pct();
    for (int i = 0; i < 20; ++i) { params_encoder(PARAM_ENC_DISPLAY, +1, t); t += 50; }
    CHECK(params_drive_pct() == d0 && params_volume_pct() == vv0,
          "DISPLAY encoder changes no audio param");

    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
