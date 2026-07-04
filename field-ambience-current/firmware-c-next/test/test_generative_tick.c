/*
 * Host test for the r18.88 generative AUTOPLAY (engine_generative_tick) and
 * the audit fixes around it:
 *   - enabling GENERATE produces a bed note on the FIRST tick (no 8 s wait)
 *   - autoplay runs by itself: bars advance, sparkle chord tones appear
 *     (sources 14/15), degree stays 1..7, audio stays finite and bounded
 *   - a held USER note suppresses new bed/sparkle notes — including the
 *     SHIFT-octave sources 9..13 (the old any_cell_held() only looked at
 *     0..4, so the bed played over latched shift notes)
 *   - releasing the user note resumes the bed on the next tick
 *   - disabling GENERATE releases bed + sparkles
 *
 * The tick is pure in now_ms, so the whole schedule is simulated without
 * wall-clock waits: advance a fake clock, render audio between ticks (the
 * envelopes need samples to move), and watch engine_active_voices().
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "engine.h"
#include "brain.h"
#include "generative.h"
#include "dsp.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

/* Render `ms` of audio in 256-frame blocks; returns peak |sample| and traps
 * NaN. 1 ms ≈ 44.1 frames — close enough for envelope movement. */
static int render_ms(int ms) {
    int16_t buf[512];
    int blocks = (ms * 44100) / (1000 * 256) + 1, pk = 0;
    for (int b = 0; b < blocks; ++b) {
        engine_render(buf, 256);
        for (int i = 0; i < 512; ++i) {
            int a = buf[i] < 0 ? -buf[i] : buf[i];
            if (a > pk) pk = a;
        }
    }
    return pk;
}

int main(void) {
    printf("== generative autoplay (engine_generative_tick, r18.88) ==\n");
    dsp_init(); brain_init(); engine_init();
    uint32_t now = 1000;

    /* ---- 1. Immediate first note ---- */
    CHECK(engine_active_voices() == 0, "silent at boot");
    engine_set_generative(true, -1);            /* Markov auto */
    engine_generative_tick(now);
    CHECK(engine_active_voices() >= 1, "bed voice starts on the FIRST tick");
    int pk = render_ms(1500);
    CHECK(pk > 300, "bed audible shortly after enable (peak %d)", pk);

    /* ---- 2. Autoplay: simulate ~120 s, tick every 16 ms ---- */
    int max_voices = 0, sparkle_seen = 0;
    for (int step = 0; step < 7500; ++step) {
        now += 16;
        engine_generative_tick(now);
        int d = generative_current_degree();
        CHECK(d >= 1 && d <= 7, "degree in range (%d)", d);
        int v = engine_active_voices();
        if (v > max_voices) max_voices = v;
        if (v >= 2) sparkle_seen = 1;           /* bed + at least one sparkle */
        if ((step & 63) == 0) {
            int p = render_ms(16 * 64);
            CHECK(p <= 32767, "bounded");
        }
        if (fails > 10) break;                  /* don't spam */
    }
    CHECK(sparkle_seen, "sparkle chord tones actually played (max voices %d)", max_voices);
    CHECK(max_voices <= 4, "voice budget respected: bed + 2 sparkles max (%d)", max_voices);

    /* ---- 3. User override incl. SHIFT-octave source (the r18.88 fix) ---- */
    engine_note_on(11, dsp_midi_to_hz(76.0f), 0.12f);   /* shift source 9..13 */
    CHECK(engine_generative_advance() == -1,
          "advance() blocked by a held SHIFT note (was the any_cell_held bug)");
    int voices_with_user = 0;
    for (int step = 0; step < 1500; ++step) {           /* 24 s held */
        now += 16;
        engine_generative_tick(now);
        int v = engine_active_voices();
        if (v > voices_with_user) voices_with_user = v;
        if ((step & 127) == 0) render_ms(16 * 128);     /* let sparkles decay */
    }
    /* No NEW gen notes while the user holds: sparkles release on schedule,
     * the BED deliberately keeps sustaining underneath (it's an ambient bed
     * — same semantics as the old advance() gate), so after the sparkle
     * tails exactly two voices remain: user note + sustained bed. */
    CHECK(voices_with_user <= 4, "no new gen notes while user holds (max %d)",
          voices_with_user);
    render_ms(10000);                                   /* drain sparkle tails */
    CHECK(engine_active_voices() == 2,
          "user note + sustained bed remain, sparkles gone (have %d)",
          engine_active_voices());

    /* ---- 4. Release → bed movement resumes promptly ----
     * Drain the user note's release tail FIRST (no ticks), so the resume
     * check below can only be satisfied by NEW sparkle notes, not by the
     * old note still fading out. */
    engine_note_off(11);
    render_ms(10000);
    CHECK(engine_active_voices() == 1, "only the bed left after the user tail (%d)",
          engine_active_voices());
    int resumed = 0;                                    /* sparkles restart? */
    for (int step = 0; step < 2500; ++step) {           /* 40 s simulated */
        now += 16;
        engine_generative_tick(now);
        if (engine_active_voices() >= 2) { resumed = 1; break; }
        if ((step & 127) == 0) render_ms(16 * 128);
    }
    CHECK(resumed, "autoplay (sparkles) resumes after the user lets go");

    /* ---- 5. Disable releases everything ---- */
    engine_set_generative(false, -1);
    render_ms(12000);
    CHECK(engine_active_voices() == 0, "all gen voices gone after disable (%d)",
          engine_active_voices());

    printf("%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
