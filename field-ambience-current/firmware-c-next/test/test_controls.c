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
    controls_modifier(MOD_SHIFT, false);    /* r19.20: SHIFT is momentary —
                                             * release it so §4 is a PLAIN clear */

    /* ---- 4. Clear wipes everything; held voices stop (reverb tail decays) ---- */
    controls_modifier(MOD_CLEAR, true);
    controls_modifier(MOD_CLEAR, false);
    CHECK(!controls_hold_base(3) && !controls_hold_shift(3), "Clear wipes both latches");
    CHECK(!controls_modifier_active(MOD_HOLD), "r19.20: plain Clear turns HOLD off");
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
    /* r19.41: threshold recalibrated 300→250 — the master-effects DREAM
     * chain voices the mix ~5.7 dB below the legacy chain (world level trim
     * + dark tone + tape age). Measured: drone peak 540 in BYPASS, 280 in
     * DREAM; the assertion's purpose (drone audibly present) still holds. */
    CHECK(peak_over(2.5f) > 250, "drone produces audio (after bloom)");
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
    controls_modifier(MOD_SHIFT, false);              /* shift RELEASED (momentary, r19.20) */
    controls_modifier(MOD_HOLD, true);                /* toggle HOLD off — taps momentary
                                                       * (r19.20: the old ',false' here was a
                                                       * no-op and HOLD silently stayed on) */
    CHECK(!controls_modifier_active(MOD_HOLD), "HOLD toggled off for the momentary tap");
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
    /* r18.88 AUDIT-FIX regression check: the release above used to
     * note_off the shared source and silently kill the LATCHED voice (the
     * latch bit stayed true = state desync). It only showed after the
     * release tail — so drain several seconds and recount. */
    (void)peak_over(6.0f);
    CHECK(engine_active_voices() == 10,
          "latched voice SURVIVES the momentary release tail (r18.88): %d",
          engine_active_voices());

    /* ---- 10. Clear nukes everything cleanly ------------------------------- */
    controls_modifier(MOD_CLEAR, true);
    for (int c = 0; c < 5; ++c) {
        CHECK(!controls_hold_base(c),  "Clear wiped base %d",  c);
        CHECK(!controls_hold_shift(c), "Clear wiped shift %d", c);
    }
    /* Engine voices fall to 0 after the release tails decay (~release_s).
     * We don't wait the full tail; just check no NEW notes are sounding. */
    CHECK(engine_active_voices() <= 10, "no spurious add after Clear");

    /* ---- 11. r18.88: key/world change re-pitches latched voices ---------- */
    controls_init(); engine_init();
    controls_modifier(MOD_HOLD, true);
    controls_cell_press(1, 0.15f);
    CHECK(controls_hold_base(1), "cell 1 latched for the key-change test");
    engine_set_key(69);                       /* move the whole world to A */
    controls_refresh_held_pitches();
    CHECK(controls_hold_base(1), "latch bit survives the re-pitch");
    CHECK(engine_active_voices() == 1,
          "re-pitch re-blooms the SAME source, no voice leak: %d",
          engine_active_voices());
    CHECK(peak_over(0.6f) > 800, "re-pitched latched voice audible");

    /* ---- 12. r19.20: SHIFT is momentary ---------------------------------- */
    controls_init(); engine_init();
    controls_modifier(MOD_SHIFT, true);
    CHECK(controls_modifier_active(MOD_SHIFT),  "SHIFT active while held");
    controls_modifier(MOD_SHIFT, false);
    CHECK(!controls_modifier_active(MOD_SHIFT), "SHIFT inactive after release (momentary)");

    /* ---- 13. r19.20: CLEAR full stop vs SHIFT+CLEAR flush ----------------- */
    controls_init(); engine_init();
    controls_modifier(MOD_DRONE, true);
    controls_modifier(MOD_GENERATE, true);
    controls_modifier(MOD_HOLD, true);
    controls_cell_press(0, 0.15f);                       /* latch a voice */
    /* SHIFT+CLEAR: voices flushed, ALL modes keep running */
    controls_modifier(MOD_SHIFT, true);
    controls_modifier(MOD_CLEAR, true);
    controls_modifier(MOD_CLEAR, false);
    controls_modifier(MOD_SHIFT, false);
    CHECK(!controls_hold_base(0),                       "flush wiped the latch");
    CHECK(controls_modifier_active(MOD_DRONE),          "flush keeps DRONE running");
    CHECK(controls_modifier_active(MOD_GENERATE),       "flush keeps GENERATE running");
    CHECK(controls_modifier_active(MOD_HOLD),           "flush keeps HOLD armed");
    /* plain CLEAR: everything off */
    controls_cell_press(1, 0.15f);                       /* latch again */
    controls_modifier(MOD_CLEAR, true);
    controls_modifier(MOD_CLEAR, false);
    CHECK(!controls_hold_base(1),                       "full stop wiped the latch");
    CHECK(!controls_modifier_active(MOD_DRONE),         "full stop turns DRONE off");
    CHECK(!controls_modifier_active(MOD_GENERATE),      "full stop turns GENERATE off");
    CHECK(!controls_modifier_active(MOD_HOLD),          "full stop turns HOLD off");
    /* the full stop must actually END the sound: either the tail is still
     * decaying, or we are already at the noise floor (silence). */
    int fs_early = peak_over(0.5f);
    int fs_late  = peak_over(3.0f);
    CHECK(fs_late < fs_early || fs_late < 100,
          "full stop: audio decays/silent (early=%d late=%d)", fs_early, fs_late);

    /* ---- 14. r19.20: physical-presence tracking --------------------------- */
    controls_init(); engine_init();
    controls_modifier(MOD_HOLD, true);
    controls_cell_press(4, 0.15f);                       /* latch ON, key DOWN */
    CHECK(controls_any_cell_down(),  "key physically down while pressed");
    controls_cell_release(4);                            /* latch stays, key UP */
    CHECK(!controls_any_cell_down(), "latched cell does NOT count as key-down");
    CHECK(controls_hold_base(4),     "latch survived the release");

    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
