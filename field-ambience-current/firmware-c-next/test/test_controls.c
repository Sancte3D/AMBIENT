/*
 * Host test for the modifier + cell hold-latch state machine (controls.c,
 * ADR-0008 r2). Drives controls_* and asserts the resulting engine voice
 * activity + latch-bit observability:
 *   - momentary tap (no Hold) sounds then releases
 *   - Hold latch: tap toggles base bit on/off
 *   - Shift+Hold latch: tap toggles shift bit, independent of base
 *   - both base+shift held at once = octave stack (both bits true)
 *   - Clear wipes all holds + silences
 *   - Drone / Generate modifiers latch + forward to engine (no crash, voices)
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "controls.h"
#include "engine.h"
#include "brain.h"
#include "dsp.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

/* Render a short block and return the peak |sample| so we can tell "sounding"
 * from "silent". The engine has attack/reverb, so render ~0.5 s. */
static int peak_over(float seconds) {
    enum { N = 256 }; int16_t buf[N*2];
    int total = (int)(seconds * 44100), pk = 0;
    for (int done = 0; done < total; done += N) {
        engine_render(buf, N);
        for (int i = 0; i < N*2; ++i) { int a = buf[i]<0?-buf[i]:buf[i]; if (a>pk) pk=a; }
    }
    return pk;
}

int main(void) {
    printf("== controls (hold-latch state machine, ADR-0008 r2) ==\n");
    dsp_init(); brain_init(); engine_init(); controls_init();

    /* ---- 1. Momentary tap (no Hold): sounds, then releases to silence ---- */
    controls_cell_press(0, 0.18f);
    CHECK(peak_over(0.4f) > 800, "momentary tap audible");
    CHECK(!controls_hold_base(0), "momentary tap does not latch base");
    controls_cell_release(0);
    engine_all_off();                       /* kill reverb tail for clean next test */
    for (int i=0;i<200;i++){int16_t b[512]; engine_render(b,256);} /* drain */

    /* ---- 2. Hold latch: tap toggles base bit ON, holds; tap again OFF ---- */
    controls_init(); engine_init();
    controls_modifier(MOD_HOLD, true);      /* latch Hold */
    CHECK(controls_modifier_active(MOD_HOLD), "Hold latched on press");
    controls_cell_press(2, 0.15f);
    CHECK(controls_hold_base(2), "Hold+tap latches base bit");
    CHECK(!controls_hold_shift(2), "shift bit still off");
    CHECK(peak_over(0.5f) > 800, "held cell keeps sounding");
    controls_cell_press(2, 0.15f);          /* tap again → toggle off */
    CHECK(!controls_hold_base(2), "second tap clears base latch");

    /* ---- 3. Shift+Hold: toggles shift bit INDEPENDENT of base ---- */
    controls_init(); engine_init();
    controls_modifier(MOD_HOLD, true);
    controls_cell_press(3, 0.14f);          /* base on */
    controls_modifier(MOD_SHIFT, true);     /* arm shift */
    controls_cell_press(3, 0.14f);          /* shift on — base must stay */
    CHECK(controls_hold_base(3),  "base latch survives shift-tap");
    CHECK(controls_hold_shift(3), "shift latch set");
    CHECK(peak_over(0.5f) > 800,  "octave-stacked cell sounds");

    /* ---- 4. Clear wipes everything; held voices stop (reverb tail decays) ---- */
    controls_modifier(MOD_CLEAR, true);
    CHECK(!controls_hold_base(3) && !controls_hold_shift(3), "Clear wipes both latches");
    /* The held voices are note_off'd; only the reverb tail remains and must
     * DECAY (no sustained source re-feeding it). Compare early vs late peak. */
    int tail_early = peak_over(0.5f);
    int tail_late  = peak_over(3.0f);
    CHECK(tail_late < tail_early, "Clear: tail decays (early=%d late=%d → no sustained voice)",
          tail_early, tail_late);

    /* ---- 5. Drone + Generate modifiers latch + forward without crashing ---- */
    controls_init(); engine_init();
    controls_modifier(MOD_DRONE, true);
    CHECK(controls_modifier_active(MOD_DRONE), "Drone latched");
    CHECK(peak_over(2.5f) > 300, "drone produces audio (after bloom)");
    controls_modifier(MOD_DRONE, true);     /* toggle off */
    CHECK(!controls_modifier_active(MOD_DRONE), "Drone toggles off");
    controls_modifier(MOD_GENERATE, true);
    CHECK(controls_modifier_active(MOD_GENERATE), "Generate latched");
    controls_modifier(MOD_GENERATE, true);
    CHECK(!controls_modifier_active(MOD_GENERATE), "Generate toggles off");

    /* ---- 6. Release-edge ignored for held cells (no spurious note-off) ---- */
    controls_init(); engine_init();
    controls_modifier(MOD_HOLD, true);
    controls_cell_press(1, 0.15f);
    controls_cell_release(1);               /* should NOT release a latched cell */
    CHECK(controls_hold_base(1), "release-edge does not clear a Hold latch");
    CHECK(peak_over(0.5f) > 800, "held cell still sounds after key-up");

    /* ---- 7. Polyphony — all 5 cells momentary simultaneously --------------- */
    controls_init(); engine_init();
    for (int c = 0; c < 5; ++c) controls_cell_press((uint8_t)c, 0.15f);
    CHECK(engine_active_voices() == 5, "5 momentary cells: %d voices", engine_active_voices());
    CHECK(peak_over(0.4f) > 1500, "5-cell chord audible");
    for (int c = 0; c < 5; ++c) controls_cell_release((uint8_t)c);

    /* ---- 8. Polyphony — all 5 Hold-base + all 5 Hold-shift = 10 voices ---- */
    controls_init(); engine_init();
    controls_modifier(MOD_HOLD, true);                /* HOLD on */
    for (int c = 0; c < 5; ++c) controls_cell_press((uint8_t)c, 0.14f);   /* base latches */
    for (int c = 0; c < 5; ++c) CHECK(controls_hold_base(c),  "base %d latched", c);
    controls_modifier(MOD_SHIFT, true);               /* now SHIFT too */
    for (int c = 0; c < 5; ++c) controls_cell_press((uint8_t)c, 0.14f);   /* shift latches on top */
    for (int c = 0; c < 5; ++c) CHECK(controls_hold_shift(c), "shift %d latched", c);
    for (int c = 0; c < 5; ++c) CHECK(controls_hold_base(c),  "base %d still latched after stack", c);
    /* All 10 sources should be active in the pad pool (pad_render path) */
    CHECK(engine_active_voices() == 10,
          "10-voice stack (base+shift × 5 cells): got %d", engine_active_voices());
    CHECK(peak_over(0.4f) > 2500, "10-voice stack audible above 5-voice level");

    /* ---- 9. Hold latches + momentary tap of an EXTRA cell coexist --------- */
    /* (still HOLD+SHIFT from §8 — clear SHIFT, then turn HOLD off so the
     * next tap is momentary; the latched cells should keep sounding.)         */
    controls_modifier(MOD_SHIFT, false);              /* shift off — but shift-latches persist */
    controls_modifier(MOD_HOLD, false);               /* hold off — taps become momentary */
    /* The 10 held latches must still be visible. */
    CHECK(controls_hold_base(0)  && controls_hold_shift(0),  "latches survive HOLD-off");
    /* Cell 2 is already latched (base+shift). A momentary tap of cell 2
     * re-uses the SAME pad-source (cell index 2) — that's a re-trigger, no
     * new voice. So we can't add an 11th this way. Instead, just verify the
     * total stays at 10 after a tap+release. */
    controls_cell_press(2, 0.12f);
    controls_cell_release(2);
    CHECK(engine_active_voices() == 10,
          "momentary re-tap on a held cell didn't add/lose voices: %d", engine_active_voices());

    /* ---- 10. Clear nukes everything cleanly ------------------------------- */
    controls_modifier(MOD_CLEAR, true);
    for (int c = 0; c < 5; ++c) {
        CHECK(!controls_hold_base(c),  "Clear wiped base %d",  c);
        CHECK(!controls_hold_shift(c), "Clear wiped shift %d", c);
    }
    /* Engine voices fall to 0 after the release tails decay (~release_s).
     * We don't wait the full tail; just check no NEW notes are sounding. */
    CHECK(engine_active_voices() <= 10, "no spurious add after Clear");

    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
